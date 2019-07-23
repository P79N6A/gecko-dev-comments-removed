







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif


static _sigsig sigArray[_SIGCOUNT];


static void defaultSighandler(int inSignal)
{
    
    extern void abort(void);
    abort();
}


MOZCE_SHUNT_API int raise(int inSignal)
{
    WINCE_LOG_API_CALL("raise called\n");

    void (*handler)(int inSignal) = defaultSighandler;

    if(inSignal >= 0 && inSignal < _SIGCOUNT)
    {
        if(NULL != sigArray[inSignal])
        {
            handler = sigArray[inSignal];
        }
    }

    handler(inSignal);
    return 0;
}


MOZCE_SHUNT_API _sigsig signal(int inSignal, _sigsig inFunc)
{
    WINCE_LOG_API_CALL("signal called\n");

    void (*retval)(int inSignal) = defaultSighandler;

    if(inSignal >= 0 && inSignal < _SIGCOUNT)
    {
        if(NULL != sigArray[inSignal])
        {
            retval = sigArray[inSignal];
        }
        sigArray[inSignal] = inFunc;
    }

    return retval;
}


#if 0
{
#endif
} 

