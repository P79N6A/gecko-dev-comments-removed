







#include "base/thread.h"
#include "chrome/common/x11_util.h"
#include "chrome/common/x11_util_internal.h"

#include <string.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "base/logging.h"
#include "base/gfx/size.h"

namespace x11_util {

Display* GetXDisplay() {
  static Display* display = NULL;

  if (!display)
    display = gdk_x11_get_default_xdisplay();

  return display;
}

static bool DoQuerySharedMemorySupport(Display* dpy) {
  int dummy;
  Bool pixmaps_supported;
  
  if (!XShmQueryVersion(dpy, &dummy, &dummy, &pixmaps_supported))
    return false;
  
  
  
  if (!pixmaps_supported)
    return false;

  
  int shmkey = shmget(IPC_PRIVATE, 1, 0666);
  if (shmkey == -1)
    return false;
  void* address = shmat(shmkey, NULL, 0);
  
  shmctl(shmkey, IPC_RMID, NULL);

  XShmSegmentInfo shminfo;
  memset(&shminfo, 0, sizeof(shminfo));
  shminfo.shmid = shmkey;

  gdk_error_trap_push();
  bool result = XShmAttach(dpy, &shminfo);
  XSync(dpy, False);
  if (gdk_error_trap_pop())
    result = false;
  shmdt(address);
  if (!result)
    return false;

  XShmDetach(dpy, &shminfo);
  return true;
}

bool QuerySharedMemorySupport(Display* dpy) {
  static bool shared_memory_support = false;
  static bool shared_memory_support_cached = false;

  if (shared_memory_support_cached)
    return shared_memory_support;

  shared_memory_support = DoQuerySharedMemorySupport(dpy);
  shared_memory_support_cached = true;

  return shared_memory_support;
}

bool QueryRenderSupport(Display* dpy) {
  static bool render_supported = false;
  static bool render_supported_cached = false;

  if (render_supported_cached)
    return render_supported;

  
  
  int dummy;
  render_supported = XRenderQueryExtension(dpy, &dummy, &dummy);
  render_supported_cached = true;

  return render_supported;
}

int GetDefaultScreen(Display* display) {
  return XDefaultScreen(display);
}

XID GetX11RootWindow() {
  return GDK_WINDOW_XID(gdk_get_default_root_window());
}

XID GetX11WindowFromGtkWidget(GtkWidget* widget) {
  return GDK_WINDOW_XID(widget->window);
}

XID GetX11WindowFromGdkWindow(GdkWindow* window) {
  return GDK_WINDOW_XID(window);
}

void* GetVisualFromGtkWidget(GtkWidget* widget) {
  return GDK_VISUAL_XVISUAL(gtk_widget_get_visual(widget));
}

int BitsPerPixelForPixmapDepth(Display* dpy, int depth) {
  int count;
  XPixmapFormatValues* formats = XListPixmapFormats(dpy, &count);
  if (!formats)
    return -1;

  int bits_per_pixel = -1;
  for (int i = 0; i < count; ++i) {
    if (formats[i].depth == depth) {
      bits_per_pixel = formats[i].bits_per_pixel;
      break;
    }
  }

  XFree(formats);
  return bits_per_pixel;
}

XRenderPictFormat* GetRenderVisualFormat(Display* dpy, Visual* visual) {
  static XRenderPictFormat* pictformat = NULL;
  if (pictformat)
    return pictformat;

  DCHECK(QueryRenderSupport(dpy));

  pictformat = XRenderFindVisualFormat(dpy, visual);
  CHECK(pictformat) << "XRENDER does not support default visual";

  return pictformat;
}

XRenderPictFormat* GetRenderARGB32Format(Display* dpy) {
  static XRenderPictFormat* pictformat = NULL;
  if (pictformat)
    return pictformat;

  
  XRenderPictFormat templ;
  templ.depth = 32;
  templ.type = PictTypeDirect;
  templ.direct.red = 16;
  templ.direct.green = 8;
  templ.direct.blue = 0;
  templ.direct.redMask = 0xff;
  templ.direct.greenMask = 0xff;
  templ.direct.blueMask = 0xff;
  templ.direct.alphaMask = 0;

  static const unsigned long kMask =
    PictFormatType | PictFormatDepth |
    PictFormatRed | PictFormatRedMask |
    PictFormatGreen | PictFormatGreenMask |
    PictFormatBlue | PictFormatBlueMask |
    PictFormatAlphaMask;

  pictformat = XRenderFindFormat(dpy, kMask, &templ, 0 );

  if (!pictformat) {
    
    
    pictformat = XRenderFindStandardFormat(dpy, PictStandardARGB32);
    CHECK(pictformat) << "XRENDER ARGB32 not supported.";
  }

  return pictformat;
}

XID AttachSharedMemory(Display* display, int shared_memory_key) {
  DCHECK(QuerySharedMemorySupport(display));

  XShmSegmentInfo shminfo;
  memset(&shminfo, 0, sizeof(shminfo));
  shminfo.shmid = shared_memory_key;

  
  
  
  if (!XShmAttach(display, &shminfo))
    NOTREACHED();

  return shminfo.shmseg;
}

void DetachSharedMemory(Display* display, XID shmseg) {
  DCHECK(QuerySharedMemorySupport(display));

  XShmSegmentInfo shminfo;
  memset(&shminfo, 0, sizeof(shminfo));
  shminfo.shmseg = shmseg;

  if (!XShmDetach(display, &shminfo))
    NOTREACHED();
}

XID CreatePictureFromSkiaPixmap(Display* display, XID pixmap) {
  XID picture = XRenderCreatePicture(
      display, pixmap, GetRenderARGB32Format(display), 0, NULL);

  return picture;
}

void FreePicture(Display* display, XID picture) {
  XRenderFreePicture(display, picture);
}

void FreePixmap(Display* display, XID pixmap) {
  XFreePixmap(display, pixmap);
}


Display* GetSecondaryDisplay() {
  static Display* display = NULL;
  if (!display) {
    display = XOpenDisplay(NULL);
    CHECK(display);
  }

  return display;
}


bool GetWindowGeometry(int* x, int* y, unsigned* width, unsigned* height,
                       XID window) {
  Window root_window, child_window;
  unsigned border_width, depth;
  int temp;

  if (!XGetGeometry(GetSecondaryDisplay(), window, &root_window, &temp, &temp,
                    width, height, &border_width, &depth))
    return false;
  if (!XTranslateCoordinates(GetSecondaryDisplay(), window, root_window,
                             0, 0 , x, y ,
                             &child_window))
    return false;

  return true;
}


bool GetWindowParent(XID* parent_window, bool* parent_is_root, XID window) {
  XID root_window, *children;
  unsigned num_children;

  Status s = XQueryTree(GetSecondaryDisplay(), window, &root_window,
                        parent_window, &children, &num_children);
  if (!s)
    return false;

  if (children)
    XFree(children);

  *parent_is_root = root_window == *parent_window;
  return true;
}

}  
