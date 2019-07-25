




#include "jsperf.h"
#include "jsutil.h"

using namespace js;






#include <linux/perf_event.h>
#include <new>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>






static int
sys_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                    int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

namespace {

using JS::PerfMeasurement;
typedef PerfMeasurement::EventMask EventMask;


struct Impl
{
    
    int f_cpu_cycles;
    int f_instructions;
    int f_cache_references;
    int f_cache_misses;
    int f_branch_instructions;
    int f_branch_misses;
    int f_bus_cycles;
    int f_page_faults;
    int f_major_page_faults;
    int f_context_switches;
    int f_cpu_migrations;

    
    int group_leader;

    
    bool running;

    Impl();
    ~Impl();

    EventMask init(EventMask toMeasure);
    void start();
    void stop(PerfMeasurement* counters);
};



static const struct
{
    EventMask bit;
    uint32_t type;
    uint32_t config;
    uint64_t PerfMeasurement::* counter;
    int Impl::* fd;
} kSlots[PerfMeasurement::NUM_MEASURABLE_EVENTS] = {
#define HW(mask, constant, fieldname)                                   \
    { PerfMeasurement::mask, PERF_TYPE_HARDWARE, PERF_COUNT_HW_##constant, \
      &PerfMeasurement::fieldname, &Impl::f_##fieldname }
#define SW(mask, constant, fieldname)                                   \
    { PerfMeasurement::mask, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_##constant, \
      &PerfMeasurement::fieldname, &Impl::f_##fieldname }

    HW(CPU_CYCLES,          CPU_CYCLES,          cpu_cycles),
    HW(INSTRUCTIONS,        INSTRUCTIONS,        instructions),
    HW(CACHE_REFERENCES,    CACHE_REFERENCES,    cache_references),
    HW(CACHE_MISSES,        CACHE_MISSES,        cache_misses),
    HW(BRANCH_INSTRUCTIONS, BRANCH_INSTRUCTIONS, branch_instructions),
    HW(BRANCH_MISSES,       BRANCH_MISSES,       branch_misses),
    HW(BUS_CYCLES,          BUS_CYCLES,          bus_cycles),
    SW(PAGE_FAULTS,         PAGE_FAULTS,         page_faults),
    SW(MAJOR_PAGE_FAULTS,   PAGE_FAULTS_MAJ,     major_page_faults),
    SW(CONTEXT_SWITCHES,    CONTEXT_SWITCHES,    context_switches),
    SW(CPU_MIGRATIONS,      CPU_MIGRATIONS,      cpu_migrations),

#undef HW
#undef SW
};

Impl::Impl()
  : f_cpu_cycles(-1),
    f_instructions(-1),
    f_cache_references(-1),
    f_cache_misses(-1),
    f_branch_instructions(-1),
    f_branch_misses(-1),
    f_bus_cycles(-1),
    f_page_faults(-1),
    f_major_page_faults(-1),
    f_context_switches(-1),
    f_cpu_migrations(-1),
    group_leader(-1),
    running(false)
{
}

Impl::~Impl()
{
    
    
    
    for (int i = 0; i < PerfMeasurement::NUM_MEASURABLE_EVENTS; i++) {
        int fd = this->*(kSlots[i].fd);
        if (fd != -1 && fd != group_leader)
            close(fd);
    }

    if (group_leader != -1)
        close(group_leader);
}

EventMask
Impl::init(EventMask toMeasure)
{
    JS_ASSERT(group_leader == -1);
    if (!toMeasure)
        return EventMask(0);

    EventMask measured = EventMask(0);
    struct perf_event_attr attr;
    for (int i = 0; i < PerfMeasurement::NUM_MEASURABLE_EVENTS; i++) {
        if (!(toMeasure & kSlots[i].bit))
            continue;

        memset(&attr, 0, sizeof(attr));
        attr.size = sizeof(attr);

        
        
        
        attr.type = kSlots[i].type;
        attr.config = kSlots[i].config;

        
        
        
        if (group_leader == -1)
            attr.disabled = 1;

        
        
        
        
        
        attr.mmap = 1;
        attr.comm = 1;

        int fd = sys_perf_event_open(&attr,
                                     0 ,
                                     -1 ,
                                     group_leader,
                                     0 );
        if (fd == -1)
            continue;

        measured = EventMask(measured | kSlots[i].bit);
        this->*(kSlots[i].fd) = fd;
        if (group_leader == -1)
            group_leader = fd;
    }
    return measured;
}

void
Impl::start()
{
    if (running || group_leader == -1)
        return;

    running = true;
    ioctl(group_leader, PERF_EVENT_IOC_ENABLE, 0);
}

void
Impl::stop(PerfMeasurement* counters)
{
    
    
    unsigned char buf[1024];

    if (!running || group_leader == -1)
        return;

    ioctl(group_leader, PERF_EVENT_IOC_DISABLE, 0);
    running = false;

    
    for (int i = 0; i < PerfMeasurement::NUM_MEASURABLE_EVENTS; i++) {
        int fd = this->*(kSlots[i].fd);
        if (fd == -1)
            continue;

        if (read(fd, buf, sizeof(buf)) == sizeof(uint64_t)) {
            uint64_t cur;
            memcpy(&cur, buf, sizeof(uint64_t));
            counters->*(kSlots[i].counter) += cur;
        }

        
        
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    }
}

} 


namespace JS {

#define initCtr(flag) ((eventsMeasured & flag) ? 0 : -1)

PerfMeasurement::PerfMeasurement(PerfMeasurement::EventMask toMeasure)
  : impl(js_new<Impl>()),
    eventsMeasured(impl ? static_cast<Impl*>(impl)->init(toMeasure)
                   : EventMask(0)),
    cpu_cycles(initCtr(CPU_CYCLES)),
    instructions(initCtr(INSTRUCTIONS)),
    cache_references(initCtr(CACHE_REFERENCES)),
    cache_misses(initCtr(CACHE_MISSES)),
    branch_instructions(initCtr(BRANCH_INSTRUCTIONS)),
    branch_misses(initCtr(BRANCH_MISSES)),
    bus_cycles(initCtr(BUS_CYCLES)),
    page_faults(initCtr(PAGE_FAULTS)),
    major_page_faults(initCtr(MAJOR_PAGE_FAULTS)),
    context_switches(initCtr(CONTEXT_SWITCHES)),
    cpu_migrations(initCtr(CPU_MIGRATIONS))
{
}

#undef initCtr

PerfMeasurement::~PerfMeasurement()
{
    js_delete(static_cast<Impl*>(impl));
}

void
PerfMeasurement::start()
{
    if (impl)
        static_cast<Impl*>(impl)->start();
}

void
PerfMeasurement::stop()
{
    if (impl)
        static_cast<Impl*>(impl)->stop(this);
}

void
PerfMeasurement::reset()
{
    for (int i = 0; i < NUM_MEASURABLE_EVENTS; i++) {
        if (eventsMeasured & kSlots[i].bit)
            this->*(kSlots[i].counter) = 0;
        else
            this->*(kSlots[i].counter) = -1;
    }
}

bool
PerfMeasurement::canMeasureSomething()
{
    
    
    
    
    
    
    
    

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.type = PERF_TYPE_MAX;

    int fd = sys_perf_event_open(&attr, 0, -1, -1, 0);
    if (fd >= 0) {
        close(fd);
        return true;
    } else {
        return errno != ENOSYS;
    }
}

} 
