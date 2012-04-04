#include <gx/gx.h>

#ifdef __LINUX__
#include <unistd.h>
static long gx_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

#else
#include <sys/types.h>
#include <sys/sysctl.h>
static long gx_cpu_count() {
    long    cpu_count;
    int     mib[4];
    size_t  len = sizeof(cpu_count); 

    mib[0] = CTL_HW;       // Set the mib for hw.ncpu 
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    sysctl(mib, 2, &cpu_count, &len, NULL, 0);

    if(cpu_count < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &cpu_count, &len, NULL, 0);
        if(cpu_count < 1) return 1;
    }
    return cpu_count;
}
#endif
