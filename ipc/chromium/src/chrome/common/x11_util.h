



#ifndef CHROME_COMMON_X11_UTIL_H_
#define CHROME_COMMON_X11_UTIL_H_







typedef struct _GdkDrawable GdkWindow;
typedef struct _GtkWidget GtkWidget;
typedef unsigned long XID;
typedef struct _XDisplay Display;

namespace base {
class Thread;
}

namespace gfx {
class Size;
}

namespace x11_util {







Display* GetXDisplay();

bool QuerySharedMemorySupport(Display* dpy);

bool QueryRenderSupport(Display* dpy);

int GetDefaultScreen(Display* display);




XID GetX11RootWindow();

XID GetX11WindowFromGtkWidget(GtkWidget*);
XID GetX11WindowFromGdkWindow(GdkWindow*);


void* GetVisualFromGtkWidget(GtkWidget*);

int BitsPerPixelForPixmapDepth(Display*, int depth);



XID AttachSharedMemory(Display* display, int shared_memory_support);
void DetachSharedMemory(Display* display, XID shmseg);



XID CreatePictureFromSkiaPixmap(Display* display, XID pixmap);

void FreePicture(Display* display, XID picture);
void FreePixmap(Display* display, XID pixmap);






Display* GetSecondaryDisplay();








bool GetWindowGeometry(int* x, int* y, unsigned* width, unsigned* height,
                       XID window);





bool GetWindowParent(XID* parent_window, bool* parent_is_root, XID window);

}  

#endif  
