#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "Perf.hpp"

#define EXPECTED_SIZE (sizeof(perf_event_header) + 3 * sizeof(void*))
static inline void mb() { asm volatile ("":::"memory"); }

PerfBase::PerfBase(int sample_period, int evt, int type)
{
    _scale = 1;
    init_base(sample_period, evt, type);
}

PerfBase::PerfBase(int sample_period, const char* driver_name, const char* event_name)
{
    int type = 0, evt = 0;
    if(!read_type_file(driver_name, &type)
        || !read_event_data(driver_name, event_name, &evt, &_scale))
    {
        exit(1);
    }

    init_base(sample_period, evt, type);
}

PerfBase::PerfBase(int sample_period, const char* event_path)
{
    if(event_path[0] == '0' && event_path[1] == 'x')
    {
        char* end = NULL;
        int event_id = strtol(event_path, &end, 16);
        if(end == NULL || *end != 0)
        {
            printf("Invalid event: '%s'\n", event_path);
            exit(1);
        }
        init_base(sample_period, event_id, PERF_TYPE_RAW);
        return;
    }
    if(event_path[0] == '/')
    {
        const char* next = strchr(event_path + 1, '/');
        if(next == NULL || next == event_path + 1)
        {
            printf("Invalid event path: %s\n", event_path);
            exit(1);
        }
        char* driver = strndup(event_path + 1, next - event_path - 1);
        int type = 0, evt = 0;

//        printf("Driver: '%s' Event: '%s'\n", driver, next + 1);
        if(!read_type_file(driver, &type)
            || !read_event_data(driver, next + 1, &evt, &_scale))
        {
            exit(1);
        }
    
        init_base(sample_period, evt, type);
        free(driver);
        return;
    }
    printf("Path type not handled yet: %s\n", event_path);
    exit(1);
}

void PerfBase::init_base(int sample_period, int evt, int type)
{
    _fd = -1;
    memset(&_attr, 0, sizeof(_attr));
    _attr.type = type;
    _attr.size = sizeof(_attr);
    _attr.sample_period = sample_period;
    switch(type)
    {
    case PERF_TYPE_RAW:
        _attr.sample_type = PERF_SAMPLE_ADDR | PERF_SAMPLE_IP | PERF_SAMPLE_TIME;
        _attr.exclude_kernel = 1;
        _attr.precise_ip = 2;
        _attr.mmap = 1;
        break;
    case PERF_TYPE_HW_CACHE:
        _attr.sample_type = 263;
        _attr.exclude_kernel = 1;
        _attr.exclude_guest = 1;
        _attr.mmap = 1;
        break;
    }

    _attr.config = evt;
    _attr.disabled = 1;
    _attr.read_format = 0;
}

PerfBase::~PerfBase()
{
    if (_fd != -1)
        close(_fd);
}

bool PerfBase::open_base(int pid, int cpu)
{
    _attr.enable_on_exec = (_pid >= 0);

    _fd = syscall(__NR_perf_event_open, &_attr, pid, cpu, -1, PERF_FLAG_FD_CLOEXEC);
    if (_fd < 0)
        return false;
    return true;
}
 
bool PerfBase::enable()
{
    return ioctl(_fd, PERF_EVENT_IOC_ENABLE, 0) == 0;
}

bool PerfBase::disable()
{
    return ioctl(_fd, PERF_EVENT_IOC_DISABLE, 0) == 0;
}

bool PerfBase::read_type_file(const char* driver_name, int* out)
{
    char name[NAME_MAX], buffer[60], *end;
    sprintf(name, "/sys/bus/event_source/devices/%s/type", driver_name);
    FILE* f = fopen(name, "r");
    if(f == NULL || fgets(buffer, sizeof(buffer), f) == NULL)
    {
        printf("Failed opening or reading '%s'\n", name);
        if(f != NULL) fclose(f);
        return false;
    }

    end = NULL;
    *out = strtol(buffer, &end, 10);
    if(end == NULL || *end >= 32)
    {
        printf("Invalid file format: %s\n", name);
        return false;
    }

    return true;
}

bool PerfBase::read_event_data(const char* driver_name, const char* event_name, int* evtid, int* scale) /* add per-pkg */
{
    char name[NAME_MAX], buffer[100], *pos, *end;
    sprintf(name, "/sys/bus/event_source/devices/%s/events/%s", driver_name, event_name);
    FILE* f = fopen(name, "r");
    if(f == NULL || fgets(buffer, sizeof(buffer), f) == NULL)
    {
        printf("Failed opening or reading '%s'\n", name);
        if(f != NULL) fclose(f);
        return false;
    }
    fclose(f);

    pos = buffer;
    for(;;)
    {
        if(strncmp(pos, "event=0x", 8) != 0)
        {
            pos = strchr(pos, ',');
            if(pos == NULL)
                return false;
            pos += 1;
            continue;
        }
 
        end = NULL;
        *evtid = strtol(pos + 8, &end, 16);
        if(end == NULL || (*end >= 32 && *end != ','))
        {
            printf("Invalid file format: %s\n", name);
            return false;
        }
        break;
    }

    sprintf(name, "/sys/bus/event_source/devices/%s/events/%s.scale", driver_name, event_name);
    f = fopen(name, "r");
    if(f == NULL || fgets(buffer, sizeof(buffer), f) == NULL)
    {
        printf("Failed opening or reading '%s'\n", name);
        if(f != NULL) fclose(f);
        return false;
    }
    fclose(f);

    end = NULL;
    *scale = strtol(buffer, &end, 10);
    if(end == NULL || (*end != 0 && *end != ','))
    {
        printf("Invalid file format: %s\n", name);
        return false;
    }

    return true;
}

PerfSampled::PerfSampled(int buf_bits, int sample_period, int evt, int type) : PerfBase(sample_period, evt, type)
{
    init_sampled(buf_bits);
}
 
PerfSampled::PerfSampled(int buf_bits, int sample_period, const char* driver_name, const char* event_name) : PerfBase(sample_period, driver_name, event_name)
{
    init_sampled(buf_bits);
}
 
PerfSampled::PerfSampled(int buf_bits, int sample_period, const char* event_path) : PerfBase(sample_period, event_path)
{
    init_sampled(buf_bits);
}

PerfSampled::~PerfSampled()
{
    if (_mpage != (struct perf_event_mmap_page *)-1L)
        munmap(_mpage, _page_size + _buf_size);
}

    
bool PerfSampled::open(int pid, int cpu, bool do_enable)
{
    if(!open_base(pid, cpu))
        return false;

    _mpage = (struct perf_event_mmap_page*)mmap(NULL,  _page_size + _buf_size, PROT_READ|PROT_WRITE, MAP_SHARED, _fd, 0);
    if (_mpage == (struct perf_event_mmap_page *)-1L)
        return false;
    _data = ((char*)_mpage) + _page_size;
    _data_end = _data + _buf_size;
    _buf_mask = _buf_size - 1;
/*    if(_mpage->data_size != _buf_size)
        return false;
    if(_mpage->data_offset != _page_size)
        return false;*/
    if(do_enable)
        return enable();

    return true;
}

bool PerfSampled::start_iterate()
{
    _head = _mpage->data_head;
    _tail = _mpage->data_tail;
    mb();
    return true;
}
            
uint64_t PerfSampled::get_exit_time() const
{
    return _exit_time;
}
 
bool PerfSampled::next(SampledRecord* dr)
{
//    printf("Tail=%p, _head=%p\n", (void*)_tail, (void*)_head);    
    while(_tail != _head)
    { 
        struct perf_event_header* hdr = (struct perf_event_header*)(_data + (_tail & _buf_mask));
        size_t size = hdr->size;
        _tail += size;
        char* base = (char*)(hdr + 1);
        if(base >= _data_end) base = (char*)_data;

//        printf("Record: %i\n", hdr->type);

        if(hdr->type != PERF_RECORD_SAMPLE)
        {
            if(hdr->type == PERF_RECORD_EXIT)
            {
                for(int i = 0; i < 4; ++ i)
                {
                    base += sizeof(uint32_t);
                    if(base >= _data_end) base = (char*)_data;
                }
                _exit_time = *(uint64_t*)base;
                printf("Exit time is: %lu\n", _exit_time);
            }
            //printf("Skipping type: %i\n", hdr->type);
            continue;
        }

        if(size != EXPECTED_SIZE)
            break;
    
        dr->ip = *((void**)base);
        base += sizeof(uint64_t);
        if(base >= _data_end) base = (char*)_data;

        dr->time = *((uint64_t*)base);
        base += sizeof(void*);
        if(base >= _data_end) base = (char*)_data;

        dr->data = *((void**)base);

        return true;
    }
    abort_iterate();
    return false;
}   
    
void PerfSampled::abort_iterate()
{
    _mpage->data_tail = _head;
    mb();
}

void PerfSampled::init_sampled(int buf_bits)
{
    _exit_time = 0;
    _lost = 0;
    _head = _tail = 0;
    _page_size = sysconf(_SC_PAGESIZE);
    _buf_size = (1U << buf_bits) * _page_size;
}

PerfCounted::PerfCounted(int evt, int type) : PerfBase(0, evt, type)
{
}
 
PerfCounted::PerfCounted(const char* driver_name, const char* event_name) : PerfBase(0, driver_name, event_name)
{
}
 
PerfCounted::PerfCounted(const char* event_path) : PerfBase(0, event_path)
{
}

PerfCounted::~PerfCounted()
{
}

bool PerfCounted::open(int pid, int cpu, bool do_enable)
{
    if(!open_base(pid, cpu))
        return false;

    if(do_enable)
        return enable();

    return true;
}

bool PerfCounted::get_value(CountedRecord* dr, bool withtime)
{
    uint64_t data;
    if(read(_fd, &data, sizeof(data)) != sizeof(data))
    {
        printf("Failed reading data (%i)\n", errno);
        return false;
    }
    if(withtime)
        dr->time = cur_time();
    else
        dr->time = 0;
    dr->value = data * _scale;
    return true;
}

uint64_t PerfCounted::cur_time()
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000L + (uint64_t)ts.tv_nsec;
}


