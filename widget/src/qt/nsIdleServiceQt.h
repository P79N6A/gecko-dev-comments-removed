






































#ifndef nsIdleServiceQt_h__
#define nsIdleServiceQt_h__

#include "nsIdleService.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
    Window window;              
    int state;                  
    int kind;                   
    unsigned long til_or_since; 
    unsigned long idle;         
    unsigned long event_mask;   
} XScreenSaverInfo;

class nsIdleServiceQt : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS
    nsIdleServiceQt();

    NS_IMETHOD GetIdleTime(PRUint32* idleTime);
private:
    ~nsIdleServiceQt();
    XScreenSaverInfo* mXssInfo;
};

#endif 
