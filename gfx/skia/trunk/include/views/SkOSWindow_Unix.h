






#ifndef SkOSWindow_Unix_DEFINED
#define SkOSWindow_Unix_DEFINED

#include <GL/glx.h>
#include <X11/Xlib.h>

#include "SkWindow.h"

class SkEvent;

struct SkUnixWindow {
  Display* fDisplay;
  Window fWin;
  size_t fOSWin;
  GC fGc;
  GLXContext fGLContext;
};

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void*);
    ~SkOSWindow();

    void* getHWND() const { return (void*)fUnixWindow.fWin; }
    void* getDisplay() const { return (void*)fUnixWindow.fDisplay; }
    void* getUnixWindow() const { return (void*)&fUnixWindow; }
    void loop();

    enum SkBackEndTypes {
        kNone_BackEndType,
        kNativeGL_BackEndType,
    };

    struct AttachmentInfo {
        int fSampleCount;
        int fStencilBits;
    };

    bool attach(SkBackEndTypes attachType, int msaaSampleCount, AttachmentInfo*);
    void detach();
    void present();

    int getMSAASampleCount() const { return fMSAASampleCount; }

    

protected:
    
    virtual void onSetTitle(const char title[]) SK_OVERRIDE;

private:
    enum NextXEventResult {
        kContinue_NextXEventResult,
        kQuitRequest_NextXEventResult,
        kPaintRequest_NextXEventResult
    };

    NextXEventResult nextXEvent();
    void doPaint();
    void mapWindowAndWait();

    void closeWindow();
    void initWindow(int newMSAASampleCount, AttachmentInfo* info);

    SkUnixWindow fUnixWindow;

    
    XVisualInfo* fVi;
    
    int fMSAASampleCount;

    typedef SkWindow INHERITED;
};

#endif
