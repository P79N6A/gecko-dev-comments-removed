




















































#if defined(_RCCOND_H)
#else
#define _RCCOND_H

#include "rclock.h"
#include "rcbase.h"
#include "rcinrval.h"

struct PRCondVar;

class PR_IMPLEMENT(RCCondition): public RCBase
{
public:
    RCCondition(RCLock*);           
    virtual ~RCCondition();

    virtual PRStatus Wait();        

    virtual PRStatus Notify();      
    virtual PRStatus Broadcast();   

    virtual PRStatus SetTimeout(const RCInterval&);
                                    

private:
    PRCondVar *cv;
    RCInterval timeout;

    RCCondition();
    RCCondition(const RCCondition&);
    void operator=(const RCCondition&);

public:
    RCInterval GetTimeout() const;
};  

inline RCCondition::RCCondition(): RCBase() { }
inline RCCondition::RCCondition(const RCCondition&): RCBase() { }
inline void RCCondition::operator=(const RCCondition&) { }

#endif 


