






































#if defined(_RCTHREAD_H)
#else
#define _RCTHREAD_H

#include "rcbase.h"

#include <prthread.h>

class RCInterval;

class PR_IMPLEMENT(RCThreadPrivateData)
{
public:
    RCThreadPrivateData();
    RCThreadPrivateData(const RCThreadPrivateData&);

    virtual ~RCThreadPrivateData();

    virtual void Release() = 0;

};  

class PR_IMPLEMENT(RCThread): public RCBase
{
public:

    typedef enum 
    {
        local = PR_LOCAL_THREAD, global = PR_GLOBAL_THREAD
    } Scope;

    typedef enum
    {
        joinable = PR_JOINABLE_THREAD, unjoinable = PR_UNJOINABLE_THREAD
    } State;

    typedef enum
    {
        first = PR_PRIORITY_FIRST,
        low = PR_PRIORITY_LOW,
        normal = PR_PRIORITY_NORMAL,
        high = PR_PRIORITY_HIGH,
        urgent = PR_PRIORITY_URGENT,
        last = PR_PRIORITY_LAST
    } Priority;

    


    RCThread(Scope scope, State state, PRUint32 stackSize=0);

    



    virtual PRStatus Start();

    




    virtual PRStatus Join();
    
    



   
    virtual void SetPriority(Priority newPriority);


    



    virtual PRStatus Interrupt();
    
    




    static void ClearInterrupt();
    
    


    static PRThread *Self();
    Scope GetScope() const;
    State GetState() const;
    Priority GetPriority() const;

    


    static PRStatus NewPrivateIndex(PRUintn* index);

    


    static RCThreadPrivateData* GetPrivateData(PRUintn index);

    


    static PRStatus SetPrivateData(PRUintn index);

    


    static PRStatus SetPrivateData(PRUintn index, RCThreadPrivateData* data);

    


    static PRStatus Sleep(const RCInterval& ticks);

    friend void nas_Root(void*);
    friend class RCPrimordialThread;
protected:

    




    virtual ~RCThread();

private:

    



    virtual void RootFunction() = 0;

    PRThread *identity;

    
    enum {ex_unstarted, ex_started} execution;

    
    RCThread();
    RCThread(const RCThread&);
    
    
    void operator=(const RCThread&);

public:
    static RCPrimordialThread *WrapPrimordialThread();    

 };
 



class PR_IMPLEMENT(RCPrimordialThread): public RCThread
{
public:
    






    static PRStatus Cleanup();

    



    static PRStatus SetVirtualProcessors(PRIntn count=10);

friend class RCThread;
private:
    




    RCPrimordialThread();
    ~RCPrimordialThread();
    void RootFunction();
};  

 #endif 
