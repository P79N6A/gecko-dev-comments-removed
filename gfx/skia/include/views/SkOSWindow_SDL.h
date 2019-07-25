








#ifndef SkOSWindow_SDL_DEFINED
#define SkOSWindow_SDL_DEFINED

#include "SDL.h"
#include "SkWindow.h"

class SkGLCanvas;

class SkOSWindow : public SkWindow {
public:
    SkOSWindow(void* screen);
    virtual ~SkOSWindow();

    static bool PostEvent(SkEvent* evt, SkEventSinkID, SkMSec delay);

    void handleSDLEvent(const SDL_Event& event);

protected:
    
    virtual void onHandleInval(const SkIRect&);
    
    virtual void onAddMenu(const SkOSMenu*);
    virtual void onSetTitle(const char[]);

private:
    SDL_Surface* fScreen;
    SDL_Surface* fSurface;
    SkGLCanvas* fGLCanvas;

    void doDraw();

    typedef SkWindow INHERITED;
};

#endif

