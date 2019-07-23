









































#if defined(_RCMONITOR_H)
#else
#define _RCMONITOR_H

#include "rcbase.h"
#include "rcinrval.h"

struct PRMonitor;

class PR_IMPLEMENT(RCMonitor): public RCBase
{
public:
    RCMonitor();                    
    virtual ~RCMonitor();

    virtual void Enter();           
    virtual void Exit();

    virtual void Notify();          
    virtual void NotifyAll();       

    virtual void Wait();            

    virtual void SetTimeout(const RCInterval& timeout);

private:
    PRMonitor *monitor;
    RCInterval timeout;

public:
    RCInterval GetTimeout() const;  

};  

#endif 


