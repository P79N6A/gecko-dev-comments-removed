








#ifndef SkOSWindow_Android_DEFINED
#define SkOSWindow_Android_DEFINED

#include "SkWindow.h"

class SkIRect;

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void*) {}
    ~SkOSWindow() {}
    bool attachGL() { return true; }
    void detachGL() {}
    void presentGL() {}

    virtual void onPDFSaved(const char title[], const char desc[],
        const char path[]);

protected:
    
    virtual void onHandleInval(const SkIRect&);
    virtual void onSetTitle(const char title[]);

private:
    typedef SkWindow INHERITED;
};

#endif

