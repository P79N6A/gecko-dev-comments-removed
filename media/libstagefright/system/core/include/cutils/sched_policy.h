















#ifndef __CUTILS_SCHED_POLICY_H
#define __CUTILS_SCHED_POLICY_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    SP_DEFAULT    = -1,
    SP_BACKGROUND = 0,
    SP_FOREGROUND = 1,
    SP_SYSTEM     = 2,  
    SP_AUDIO_APP  = 3,
    SP_AUDIO_SYS  = 4,
    SP_CNT,
    SP_MAX        = SP_CNT - 1,
    SP_SYSTEM_DEFAULT = SP_FOREGROUND,
} SchedPolicy;







extern int set_sched_policy(int tid, SchedPolicy policy);





extern int get_sched_policy(int tid, SchedPolicy *policy);





extern const char *get_sched_policy_name(SchedPolicy policy);

#ifdef __cplusplus
}
#endif

#endif 
