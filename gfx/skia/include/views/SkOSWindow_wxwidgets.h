






#ifndef SkOSWindow_wxwidgets_DEFINED
#define SkOSWindow_wxwidgets_DEFINED

#include "SkWindow.h"
#include "wx/frame.h"

class SkOSWindow: public SkWindow
{
public:
    SkOSWindow();
    SkOSWindow(const wxString& title, int x, int y, int width, int height);
    ~SkOSWindow();
    
    wxFrame* getWXFrame() const { return fFrame; }
    
    void updateSize();
    
protected:
    virtual void onHandleInval(const SkIRect&);
    virtual void onAddMenu(const SkOSMenu*);
    
private:
    wxFrame* fFrame;
    typedef SkWindow INHERITED;
    
};

#endifpedef SkWindow INHERITED;
