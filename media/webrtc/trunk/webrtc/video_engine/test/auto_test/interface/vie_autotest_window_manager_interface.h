













#include "vie_autotest_defines.h"

#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_WINDOW_MANAGER_INTERFACE_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_WINDOW_MANAGER_INTERFACE_H_

class ViEAutoTestWindowManagerInterface
{
public:
    virtual int CreateWindows(AutoTestRect window1Size,
                              AutoTestRect window2Size, void* window1Title,
                              void* window2Title) = 0;
    virtual int TerminateWindows() = 0;
    virtual void* GetWindow1() = 0;
    virtual void* GetWindow2() = 0;
    virtual bool SetTopmostWindow() = 0;
    virtual ~ViEAutoTestWindowManagerInterface() {}
};

#endif  
