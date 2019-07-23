








































#ifndef jsd_lock_h___
#define jsd_lock_h___











typedef struct JSDStaticLock JSDStaticLock;

extern void*
jsd_CreateLock();

extern void
jsd_Lock(JSDStaticLock* lock);

extern void
jsd_Unlock(JSDStaticLock* lock);

#ifdef DEBUG
extern JSBool
jsd_IsLocked(JSDStaticLock* lock);
#endif 

extern void*
jsd_CurrentThread();

#endif 
