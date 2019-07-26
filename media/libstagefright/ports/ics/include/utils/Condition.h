















#ifndef _LIBS_UTILS_CONDITION_H
#define _LIBS_UTILS_CONDITION_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/Timers.h>


namespace android {










class Condition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    enum WakeUpType {
        WAKE_UP_ONE = 0,
        WAKE_UP_ALL = 1
    };

    Condition();
    Condition(int type);
    ~Condition();
    
    status_t wait(Mutex& mutex);
    
    status_t waitRelative(Mutex& mutex, nsecs_t reltime);
    
    void signal();
    
    void signal(WakeUpType type) {
        if (type == WAKE_UP_ONE) {
            signal();
        } else {
            broadcast();
        }
    }
    
    void broadcast();
};



inline Condition::Condition() {
}
inline Condition::Condition(int type) {
}
inline Condition::~Condition() {
}
inline status_t Condition::wait(Mutex& mutex) {
    return OK;
}
inline status_t Condition::waitRelative(Mutex& mutex, nsecs_t reltime) {
    return OK;
}
inline void Condition::signal() {
}
inline void Condition::broadcast() {
}


}; 


#endif
