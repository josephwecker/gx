#include <gx/gx.h>

static long _gx_cpu_count = 0;  // Cache it

#ifdef __LINUX__
#include <unistd.h>
static long gx_cpu_count() {
    if(_gx_cpu_count) return _gx_cpu_count;
    _gx_cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    return _gx_cpu_count;
}

#else
#include <sys/types.h>
#include <sys/sysctl.h>
static long gx_cpu_count() {
    long    cpu_count;
    int     mib[4];
    size_t  len = sizeof(cpu_count);

    if(_gx_cpu_count) return _gx_cpu_count;

    mib[0] = CTL_HW;       // Set the mib for hw.ncpu
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    sysctl(mib, 2, &cpu_count, &len, NULL, 0);

    if(cpu_count < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &cpu_count, &len, NULL, 0);
        if(cpu_count < 1) return 1;
    }
    _gx_cpu_count = cpu_count;
    return cpu_count;
}
#endif
