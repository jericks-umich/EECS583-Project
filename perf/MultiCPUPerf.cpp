#include "MultiCPUPerf.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define MAX_CPUS 128

MultiCPUPerf::MultiCPUPerf(int buf_bits, int sample_period, int evt, int type) :
        _buf_bits(buf_bits), _sample_period(sample_period), _evt(evt), _type(type), _perfs(NULL),_count(0)
{
}

MultiCPUPerf::~MultiCPUPerf()
{
    if(_perfs != NULL)
    {
        for(int i = 0; i < _count; ++ i)
            delete _perfs[i];
        delete _perfs;
    }
}

bool MultiCPUPerf::open(int pid)
{
    char name[1024];
    struct stat buf;
    for(int i = 0; i < MAX_CPUS; ++ i)
    {
        memset(&buf, 0, sizeof(buf));
        sprintf(name, "/sys/devices/system/cpu/cpu%i", i);

        if(stat(name, &buf) >= 0)
            ++ _count;
    }
    _perfs = new Perf*[_count];
    int pos = 0;
    for(int i = 0; i < MAX_CPUS; ++ i)
    {
        memset(&buf, 0, sizeof(buf));
        sprintf(name, "/sys/devices/system/cpu/cpu%i", i);

        if(stat(name, &buf) >= 0)
        {
            _perfs[pos] = new Perf(_buf_bits, _sample_period, _evt, _type);
            if(!_perfs[pos]->open(pid, i, false))
                return false;
            //printf("Opened %i\n", i);
            ++ pos;
            if(pos == _count)
                break;
        }
    }
    return true;
}

bool MultiCPUPerf::enable()
{
    for(int i = 0; i < _count; ++ i)
    {
        printf("Enabling %i\n", i);
        if(!_perfs[i]->enable())
        {
            for(--i; i >= 0; -- i)
                _perfs[i]->disable();
            return false;
        }
    }
    return true;
}

bool MultiCPUPerf::disable()
{
    for(int i = 0; i < _count; ++ i)
        _perfs[i]->disable();
    return true;
}

bool MultiCPUPerf::start_iterate()
{
    if(_count == 0)
        return false;
    _iterator_pos = 0;
    return _perfs[_iterator_pos]->start_iterate();
}

bool MultiCPUPerf::next(DataRecord* dr)
{
    for(;;)
    {
        //printf("Getting next from %i\n", _iterator_pos);
        if(_perfs[_iterator_pos]->next(dr))
            return true;

        ++ _iterator_pos;
        if(_iterator_pos == _count)
            return false;

        if(!_perfs[_iterator_pos]->start_iterate())
            return false;
    }
}

void MultiCPUPerf::abort_iterate()
{
    if(_iterator_pos >= _count)
        return;
    _perfs[_iterator_pos]->abort_iterate();
}



