








































#if defined(_RCLOCK_H)
#else
#define _RCLOCK_H

#include "rcbase.h"

#include <prlock.h>

class PR_IMPLEMENT(RCLock): public RCBase
{
public:
    RCLock();
    virtual ~RCLock();

    void Acquire();                 
    void Release();                 

    friend class RCCondition;

private:
    RCLock(const RCLock&);          
    void operator=(const RCLock&);  

    PRLock *lock;
};  








class PR_IMPLEMENT(RCEnter)
{
public:
    ~RCEnter();                     
    RCEnter(RCLock*);               

private:
    RCLock *lock;

    RCEnter();
    RCEnter(const RCEnter&);
    void operator=(const RCEnter&);

    void *operator new(PRSize) { return NULL; }
    void operator delete(void*) { }
};  


inline RCEnter::RCEnter(RCLock* ml) { lock = ml; lock->Acquire(); }
inline RCEnter::~RCEnter() { lock->Release(); lock = NULL; }

#endif 


