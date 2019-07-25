









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_LINUX_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_LINUX_H_

#include "vie_autotest_window_manager_interface.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>



class ViEAutoTestWindowManager: public ViEAutoTestWindowManagerInterface
{
public:
    ViEAutoTestWindowManager();
    virtual ~ViEAutoTestWindowManager();
    virtual void* GetWindow1();
    virtual void* GetWindow2();
    virtual int TerminateWindows();
    virtual int CreateWindows(AutoTestRect window1Size,
                              AutoTestRect window2Size, void* window1Title,
                              void* window2Title);
    virtual bool SetTopmostWindow();

private:
    int ViECreateWindow(Window *outWindow, Display **outDisplay, int xpos,
                        int ypos, int width, int height, char* title);
    int ViEDestroyWindow(Window *window, Display *display);

    Window _hwnd1;
    Window _hwnd2;
    Display* _hdsp1;
    Display* _hdsp2;
};

#endif  
