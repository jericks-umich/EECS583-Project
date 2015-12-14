#ifndef __AE_PERF_HPP__
#define __AE_PERF_HPP__

#include <linux/perf_event.h>
#include <stdint.h>

struct SampledRecord
{
    uint64_t time;
    void* ip;
    void* data;
};

struct CountedRecord
{
    uint64_t time;
    uint64_t value;
};

class PerfBase
{
protected:
    PerfBase(int sample_period, const char* event_path);
    PerfBase(int sample_period, int evt, int type = PERF_TYPE_RAW);
    PerfBase(int sample_period, const char* driver_name, const char* event_name);
    ~PerfBase();

public:
    bool enable();
    bool disable();

protected:
    bool open_base(int pid, int cpu);
private:
    void init_base(int sample_period, int evt, int type);
    bool read_type_file(const char* driver_name, int* out);
    bool read_event_data(const char* driver_name, const char* evtname, int* evtid, int* scale);
 
protected:
    struct perf_event_attr _attr;
    int _pid;
    int _fd;
    int _scale;
};

class PerfSampled: public PerfBase
{
public:
    PerfSampled(int buf_bits, int sample_period, const char* event_path);
    PerfSampled(int buf_bits, int sample_period, int evt, int type = PERF_TYPE_RAW);
    PerfSampled(int buf_bits, int sample_period, const char* driver_name, const char* event_name);
    ~PerfSampled();

public:
    bool open(int pid = -1, int cpu = -1, bool enable = true);
    bool start_iterate();
    bool next(SampledRecord* dr);
    void abort_iterate();
    uint64_t get_exit_time() const; 

private:
    void init_sampled(int buf_bits);

private:
    unsigned int _page_size, _buf_size, _buf_mask;
    struct perf_event_mmap_page *_mpage;
    char *_data, *_data_end;
    uint64_t _head, _tail; 
    int _lost;
    uint64_t _exit_time;
};


class PerfCounted: public PerfBase
{
public:
    PerfCounted(const char* event_path);
    PerfCounted(int evt, int type = PERF_TYPE_RAW);
    PerfCounted(const char* driver_name, const char* event_name);
    ~PerfCounted();

public:
    bool open(int pid = -1, int cpu = -1, bool enable = true);
    bool get_value(CountedRecord* dr, bool withtime = true);

    static uint64_t cur_time();
};



#endif // __AE_PERF_HPP__

