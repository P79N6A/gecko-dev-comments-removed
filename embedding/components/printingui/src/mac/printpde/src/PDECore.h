


















































 
#ifndef __PDECORE__
#define __PDECORE__

#include <Carbon/Carbon.h>








typedef struct
{
    ControlRef       userPane;
    EventHandlerRef  helpHandler;
    EventHandlerUPP  helpHandlerUPP;
    void*            customContext;
    Boolean          initialized;

} MyContextBlock;

typedef MyContextBlock* MyContext;


#endif
