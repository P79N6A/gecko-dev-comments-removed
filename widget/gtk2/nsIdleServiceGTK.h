






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
    NS_DECL_ISUPPORTS_INHERITED

    bool PollIdleTime(uint32_t* aIdleTime);

    static already_AddRefed<nsIdleServiceGTK> GetInstance()
    {
        nsIdleServiceGTK* idleService =
            static_cast<nsIdleServiceGTK*>(nsIdleService::GetInstance().get());
        if (!idleService) {
            idleService = new nsIdleServiceGTK();
            NS_ADDREF(idleService);
        }

        return idleService;
    }

private:
    ~nsIdleServiceGTK();
    XScreenSaverInfo* mXssInfo;

protected:
    nsIdleServiceGTK();
    bool UsePollMode();
};

#endif 
