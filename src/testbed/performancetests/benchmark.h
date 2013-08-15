#ifndef LIBGEODECOMP_TESTBED_PERFORMANCE_TESTS_BENCHMARK_H
#define LIBGEODECOMP_TESTBED_PERFORMANCE_TESTS_BENCHMARK_H

#include <libgeodecomp/misc/coord.h>

class Benchmark
{
public:
    virtual std::string order() = 0;
    virtual std::string family() = 0;
    virtual std::string species() = 0;
    virtual double performance(const LibGeoDecomp::Coord<3>& dim) = 0;
    virtual std::string unit() = 0;
    virtual std::string device() = 0;

};

#endif