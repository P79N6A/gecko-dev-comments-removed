





#ifndef SIMPLETHREAD_H
#define SIMPLETHREAD_H

#include "mutex.h"

class U_EXPORT SimpleThread
{
public:
    SimpleThread();
    virtual  ~SimpleThread();
    int32_t   start(void);        
    UBool     isRunning();        

    virtual void run(void) = 0;   
                                  
    void *fImplementation;

public:
    static void sleep(int32_t millis); 
    static void errorFunc();      
                                  
};

#endif

