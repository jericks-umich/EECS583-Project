#ifndef __AE_MULTI_CPU_PERF_HPP
#define __AE_MULTI_CPU_PERF_HPP

#include "Perf.hpp"

class MultiCPUPerf
{
public:
    MultiCPUPerf(int buf_bits, int sample_period, int evt, int type = PERF_TYPE_RAW);
    ~MultiCPUPerf();

    int count() const { return _count; }
public:
    bool open(int pid);
    bool enable();
    bool disable();

public:
    bool start_iterate();
    bool next(DataRecord* dr);
    void abort_iterate();

private:
    int _buf_bits, _sample_period, _evt, _type;
    Perf** _perfs;
    int _count;
    int _iterator_pos;
};

#endif // __AE_MULTI_CPU_PERF_HPP
