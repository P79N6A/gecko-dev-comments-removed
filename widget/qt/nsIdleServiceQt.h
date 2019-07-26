






#ifndef nsIdleServiceQt_h__
#define nsIdleServiceQt_h__

#include "nsIdleService.h"

#if defined(MOZ_X11)
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
    NS_DECL_ISUPPORTS_INHERITED

    bool PollIdleTime(uint32_t* aIdleTime);

    static already_AddRefed<nsIdleServiceQt> GetInstance()
    {
        nsRefPtr<nsIdleServiceQt> idleService =
            nsIdleService::GetInstance().downcast<nsIdleServiceQt>();
        if (!idleService) {
            idleService = new nsIdleServiceQt();
        }
        
        return idleService.forget();
    }

private:
#if defined(MOZ_X11)
    XScreenSaverInfo* mXssInfo;
#endif

protected:
    nsIdleServiceQt();
    virtual ~nsIdleServiceQt();
    bool UsePollMode();
};

#endif 
