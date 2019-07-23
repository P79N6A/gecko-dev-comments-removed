







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif


static _sigsig sigArray[_SIGCOUNT];


static void defaultSighandler(int inSignal)
{
    
    extern void mozce_abort(void);
    mozce_abort();
}


MOZCE_SHUNT_API int mozce_raise(int inSignal)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_raise called\n");
#endif

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


MOZCE_SHUNT_API _sigsig mozce_signal(int inSignal, _sigsig inFunc)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_signal called\n");
#endif

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

