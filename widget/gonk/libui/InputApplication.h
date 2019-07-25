















#ifndef _UI_INPUT_APPLICATION_H
#define _UI_INPUT_APPLICATION_H

#include "Input.h"

#include <utils/RefBase.h>
#include "Timers.h"
#include "String8.h"

namespace android {




struct InputApplicationInfo {
    String8 name;
    nsecs_t dispatchingTimeout;
};








class InputApplicationHandle : public RefBase {
public:
    inline const InputApplicationInfo* getInfo() const {
        return mInfo;
    }

    inline String8 getName() const {
        return mInfo ? mInfo->name : String8("<invalid>");
    }

    inline nsecs_t getDispatchingTimeout(nsecs_t defaultValue) const {
        return mInfo ? mInfo->dispatchingTimeout : defaultValue;
    }

    








    virtual bool updateInfo() = 0;

    



    void releaseInfo();

protected:
    InputApplicationHandle();
    virtual ~InputApplicationHandle();

    InputApplicationInfo* mInfo;
};

} 

#endif 
