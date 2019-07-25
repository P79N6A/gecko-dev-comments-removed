






































#ifndef nsIdleServiceQt_h__
#define nsIdleServiceQt_h__

#include "nsIdleService.h"

#ifndef MOZ_PLATFORM_MAEMO
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
#endif

class nsIdleServiceQt : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS
    nsIdleServiceQt();

    bool PollIdleTime(PRUint32* aIdleTime);

private:
    ~nsIdleServiceQt();
#ifndef MOZ_PLATFORM_MAEMO
    XScreenSaverInfo* mXssInfo;
#endif

protected:
    bool UsePollMode();
};

#endif 
