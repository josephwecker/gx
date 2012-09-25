/**
 * Some basic abstractions in order to pull in current process information.
 * Will wrap around getrusage, getrlimit, on linux some /proc/self/stat*
 * information, various ioctl calls, and, on mac, proc_pidinfo etc.
 *
 * Ideally the interface this will expose will only do the minimal amount of
 * work absolutely necessary-  so maybe something like an explicit refresh
 * command, and then specific commands for each kind of data you may want to
 * get back. If that function sees it hasn't done the right syscall/parsing
 * etc. since it last got marked "dirty" (by the refresh command)- it will go
 * through the work, but otherwise will used the cache values (under the
 * assumption that the user's program intends one snapshot and is now cycling
 * through various properties).
 *
 * @todo  Possibly opposite behavior from above- assume always refresh unless
 *        user temporarily sets a do-not-refresh flag.
 *
 * @todo  Cache chunks as appropriate
 * @todo  For linux, use /proc/self/status and parse, instead of stat+statm,
 *        because it has a lot more- like high-water-marks etc.
 * @todo  Unions for various things parsed out so they have a "workable" type
 *        as well as the type suited for scanf/parsing.
 * @todo  Pretty much everything... almost all the main functionality.
 *
 * @todo  Values as % of limit from processes perspective (e.g., open-files,
 *        children, ...) and percentage of system limit (e.g.  cpu, ram,
 *        nic-capacity, etc.)
 *
 */
#ifndef GX_SYSTEM_H
#define GX_SYSTEM_H
#include <gx/gx.h>



typedef struct _gx_sysinfo {
    // @todo  much more
    long long unsigned int  ram_in_use__bytes;
    long long unsigned int  ram_high_water_mark__bytes;
    // @todo  much more
} _gx_sysinfo;



// ASSUMES LINUX >= 2.6.22 !!!
// This was from /proc/self/stat, which we probably won't end up using in the
// end. In fact, the whole structure may be depricated, but I need it for
// reference for a bit.
typedef struct _gx_proc_stat_sysinfo {
    int                     pid;
    char                    name[1024];
    char                    state;
    int                     parent_pid;
    int                     process_group_id;
    int                     session_id;
    int                     controlling_terminal;
    int                     foreground_group_id;
    unsigned int            process_flags;

    long unsigned int       total_minor_faults;
    long unsigned int       total_children_minor_faults;
    long unsigned int       total_major_faults;
    long unsigned int       total_children_major_faults;

    long unsigned int       total_user_ticks;   // Divide by sysconf(_SC_CLK_TCK)
    long unsigned int       total_system_ticks; // Divide
    long int                total_children_user_ticks;
    long int                total_children_system_ticks;

    long int                priority;
    long int                nice;
    long int                num_threads;

    long int                old_itrealvalue;    // always 0- depricated
    long long unsigned int  start_time_in_jsb;  // jsb = jiffies since boot

    long unsigned int       vm_size_bytes;
    long int                rss_pages; // resident set size- current hdw ram usage
    long unsigned int       rss_soft_limit_bytes;

    long unsigned int       code_start_address;
    long unsigned int       code_end_address;
    long unsigned int       stack_bottom;
    long unsigned int       current_stack_pointer;
    long unsigned int       current_instruction_pointer;

    long unsigned int       old_signals_bitmap;
    long unsigned int       old_blocked_signals_bitmap;
    long unsigned int       old_ignored_signals_bitmap;
    long unsigned int       old_caught_signals_bitmap;

    long unsigned int       waiting_channel_syscall_address;

    long unsigned int       old_total_pages_swapped;
    long unsigned int       old_total_children_pages_swapped;

    int                     exit_signal; // signal we'll send to parent
    unsigned int            rt_scheduling_priority;
    unsigned int            scheduling_policy; // SCHED_* constants in linux/sched.h

    long long unsigned int  total_io_delays_ticks;
    long unsigned int       total_guest_ticks; // already included in total_user_ticks
    long int                total_children_guest_ticks; // already in total_children_user_ticks

    long long int           mem_total_pages;
    long long int           mem_rss_pages;
    long long int           mem_shared_pages;
    long long int           mem_text_pages;
    long long int           mem_data_and_stack_pages;
} _gx_proc_stat_sysinfo;



static optional _gx_proc_stat_sysinfo _gx_current_sysinfo;
static optional int         _gx_sys_dirty          =  1;

#ifdef __LINUX__
  #define gp(FLD) _gx_current_sysinfo.FLD
  #define g(FLD) &(gp(FLD))
  static void _gx_sys_refresh_meminfo() {
      FILE *fproc_stat, *fproc_statm;
      _N(fproc_stat  = fopen("/proc/self/stat",  "r")) _warning();
      _N(fproc_statm = fopen("/proc/self/statm", "r")) _warning();

      _Z(fscanf(fproc_stat,"%d %s %c %d %d %d %d %d %u %lu "
                            "%lu %lu %lu %lu %lu %ld %ld %ld %ld %ld "
                            "%ld %llu %lu %ld %lu %lu %lu %lu %lu %lu "
                            "%lu %lu %lu %lu %lu %lu %lu %d %u %u "
                            "%llu %lu %ld",
                  g(pid), gp(name), g(state), g(parent_pid),
                  g(process_group_id), g(session_id), g(controlling_terminal),
                  g(foreground_group_id), g(process_flags),
                  g(total_minor_faults), g(total_children_minor_faults),
                  g(total_major_faults), g(total_children_major_faults),
                  g(total_user_ticks), g(total_system_ticks),
                  g(total_children_user_ticks), g(total_children_system_ticks),
                  g(priority), g(nice), g(num_threads), g(old_itrealvalue),
                  g(start_time_in_jsb), g(vm_size_bytes), g(rss_pages),
                  g(rss_soft_limit_bytes), g(code_start_address),
                  g(code_end_address), g(stack_bottom),
                  g(current_stack_pointer), g(current_instruction_pointer),
                  g(old_signals_bitmap), g(old_blocked_signals_bitmap),
                  g(old_ignored_signals_bitmap), g(old_caught_signals_bitmap),
                  g(waiting_channel_syscall_address),
                  g(old_total_pages_swapped),
                  g(old_total_children_pages_swapped), g(exit_signal),
                  g(rt_scheduling_priority), g(scheduling_policy),
                  g(total_io_delays_ticks), g(total_guest_ticks),
                  g(total_children_guest_ticks))) _warning();
      _Z(fscanf(fproc_statm, "%lld %lld %lld %lld 0 %lld 0",
                  g(mem_total_pages), g(mem_rss_pages), g(mem_shared_pages),
                  g(mem_text_pages), g(mem_data_and_stack_pages))) _warning();
      _gx_sys_dirty = 0;
  }
#else

#endif

/// Mark current information as dirty so that it fetches the latest.
static inline void gx_sys_refresh() {_gx_sys_dirty = 1;}

static inline long long int gx_sys_ram_pages_in_use() {
    if(_gx_sys_dirty) _gx_sys_refresh_meminfo();
    return _gx_current_sysinfo.mem_rss_pages;
}

static inline long long int gx_sys_ram_bytes_in_use() {
    if(_gx_sys_dirty) _gx_sys_refresh_meminfo();
    return gx_sys_ram_pages_in_use() * pagesize();
}


#if 0
#ifdef __LINUX__
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/ethtool.h>

static inline int gx_nic_capacity_BitsPerSec() {
    int sock;
    struct ifreq ifr;
    struct ethtool_cmd edata;
    int rc;

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    ifr.ifr_data = &edata;
    edata.cmd = ETHTOOL_GSET;

    // A bug in Linux's SIOCGIFCONF doesn't return the network interface card
    // names correctly when they are bonded together, so instead we just
    // brute-force loop. Obviously this fails when the interfaces are named
    // something than eth0++
    // I'm sure there's a cleaner solution if it comes to it.
    int i=0, unfound=0;
    long int total_mbits=0;
    while(unfound < 3) {
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth%d", i++);
        rc = ioctl(sock, SIOCETHTOOL, &ifr);
        if(rc < 0) unfound ++;
        else {
            fprintf(stderr, "DEBUG: %s - %d Mbps\n",ifr.ifr_name, edata.speed);
            total_mbits += edata.speed;
        }
    }
    printf("%ld Mb/s\n%ld Bytes/second\n", total_mbits, total_mbits * 1000 >> 3);
    return (0);
}
#else

#endif
#endif

#endif
