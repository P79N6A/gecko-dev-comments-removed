















#define LOG_TAG "InputWindow"

#include "InputWindow.h"

#include <cutils/log.h>

namespace android {



bool InputWindowInfo::touchableRegionContainsPoint(int32_t x, int32_t y) const {
#ifdef HAVE_ANDROID_OS
    return touchableRegion.contains(x, y);
#else
    return false;
#endif
}

bool InputWindowInfo::frameContainsPoint(int32_t x, int32_t y) const {
    return x >= frameLeft && x <= frameRight
            && y >= frameTop && y <= frameBottom;
}

bool InputWindowInfo::isTrustedOverlay() const {
    return layoutParamsType == TYPE_INPUT_METHOD
            || layoutParamsType == TYPE_INPUT_METHOD_DIALOG
            || layoutParamsType == TYPE_SECURE_SYSTEM_OVERLAY;
}

bool InputWindowInfo::supportsSplitTouch() const {
    return layoutParamsFlags & FLAG_SPLIT_TOUCH;
}




InputWindowHandle::InputWindowHandle(const sp<InputApplicationHandle>& inputApplicationHandle) :
    inputApplicationHandle(inputApplicationHandle), mInfo(NULL) {
}

InputWindowHandle::~InputWindowHandle() {
    delete mInfo;
}

void InputWindowHandle::releaseInfo() {
    if (mInfo) {
        delete mInfo;
        mInfo = NULL;
    }
}

} 
