







































#ifndef nsIdleServiceGTK_h__
#define nsIdleServiceGTK_h__

#include "nsIdleService.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk/gdkx.h>

typedef struct {
    Window window;              
    int state;                  
    int kind;                   
    unsigned long til_or_since; 
    unsigned long idle;         
    unsigned long event_mask;   
} XScreenSaverInfo;

class nsIdleServiceGTK : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS
    nsIdleServiceGTK();

    NS_IMETHOD GetIdleTime(PRUint32* idleTime);
private:
    ~nsIdleServiceGTK();
    XScreenSaverInfo* mXssInfo;
};

#endif 
