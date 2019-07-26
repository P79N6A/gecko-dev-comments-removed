






#ifndef SkOSWindow_iOS_DEFINED
#define SkOSWindow_iOS_DEFINED

#include "SkWindow.h"

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void* hwnd);
    ~SkOSWindow();
    void*   getHWND() const { return fHWND; }

    enum SkBackEndTypes {
        kNone_BackEndType,
        kNativeGL_BackEndType,
    };

    struct AttachmentInfo {
        int fSampleCount;
        int fStencilBits;
    };

    void    detach();
    bool    attach(SkBackEndTypes attachType, int msaaSampleCount, AttachmentInfo*);
    void    present();

protected:
    
    virtual bool onEvent(const SkEvent& evt);
    
    virtual void onHandleInval(const SkIRect&);
    
    virtual void onAddMenu(const SkOSMenu*);
    virtual void onUpdateMenu(SkOSMenu*);
    virtual void onSetTitle(const char[]);

private:
    void*   fHWND;
    bool    fInvalEventIsPending;
    void*   fNotifier;
    typedef SkWindow INHERITED;
};

#endif
