








#ifndef SkOSWindow_Android_DEFINED
#define SkOSWindow_Android_DEFINED

#include "SkWindow.h"

class SkIRect;

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void*) {}
    ~SkOSWindow() {}

    enum SkBackEndTypes {
        kNone_BackEndType,
        kNativeGL_BackEndType,
    };

    bool attach(SkBackEndTypes , int ) {
        return true;
    }
    void detach() {}
    void present() {}

    virtual void onPDFSaved(const char title[], const char desc[],
        const char path[]);

protected:
    
    virtual void onHandleInval(const SkIRect&);
    virtual void onSetTitle(const char title[]);

private:
    typedef SkWindow INHERITED;
};

#endif

