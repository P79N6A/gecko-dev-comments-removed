








#ifndef SkOSWindow_Unix_DEFINED
#define SkOSWindow_Unix_DEFINED

#include "SkWindow.h"
#include <X11/Xlib.h>
#include <GL/glx.h>

class SkBitmap;
class SkEvent;

struct SkUnixWindow {
  Display* fDisplay;
  Window fWin;
  size_t fOSWin;
  GC fGc;
  GLXContext fGLContext;
  bool fGLCreated;
};

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void*);
    ~SkOSWindow();

    void* getHWND() const { return (void*)fUnixWindow.fWin; }
    void* getDisplay() const { return (void*)fUnixWindow.fDisplay; }
    void* getUnixWindow() const { return (void*)&fUnixWindow; }
    void loop();
    void post_linuxevent();
    bool attachGL();
    void detachGL();
    void presentGL();

    

    

protected:
    
    virtual bool onEvent(const SkEvent&);
    virtual void onHandleInval(const SkIRect&);
    virtual bool onHandleChar(SkUnichar);
    virtual bool onHandleKey(SkKey);
    virtual bool onHandleKeyUp(SkKey);
    virtual void onSetTitle(const char title[]);

private:
    SkUnixWindow  fUnixWindow;
    bool fGLAttached;

    
    XVisualInfo* fVi;

    void    doPaint();
    void    mapWindowAndWait();

    typedef SkWindow INHERITED;
};

#endif

