















#define LOG_TAG "InputApplication"

#include "InputApplication.h"

#include "cutils_log.h"

namespace android {



InputApplicationHandle::InputApplicationHandle() :
    mInfo(NULL) {
}

InputApplicationHandle::~InputApplicationHandle() {
    delete mInfo;
}

void InputApplicationHandle::releaseInfo() {
    if (mInfo) {
        delete mInfo;
        mInfo = NULL;
    }
}

} 
