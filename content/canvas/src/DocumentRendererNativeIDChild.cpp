



































#ifdef MOZ_WIDGET_QT
#include <QX11Info>
#define DISPLAY QX11Info::display
#endif

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdkx.h>
#define DISPLAY GDK_DISPLAY
#endif

#include "base/basictypes.h"

#include "gfxImageSurface.h"
#include "gfxPattern.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsCSSParser.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsColor.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsLayoutUtils.h"

#include "mozilla/ipc/DocumentRendererNativeIDChild.h"

#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif

using namespace mozilla::ipc;

DocumentRendererNativeIDChild::DocumentRendererNativeIDChild()
{}

DocumentRendererNativeIDChild::~DocumentRendererNativeIDChild()
{}

static void
FlushLayoutForTree(nsIDOMWindow* aWindow)
{
    nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aWindow);
    if (!piWin)
        return;

    
    
    

    nsCOMPtr<nsIDOMDocument> domDoc;
    aWindow->GetDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    if (doc) {
        doc->FlushPendingNotifications(Flush_Layout);
    }

    nsCOMPtr<nsIDocShellTreeNode> node =
        do_QueryInterface(piWin->GetDocShell());
    if (node) {
        PRInt32 i = 0, i_end;
        node->GetChildCount(&i_end);
        for (; i < i_end; ++i) {
            nsCOMPtr<nsIDocShellTreeItem> item;
            node->GetChildAt(i, getter_AddRefs(item));
            nsCOMPtr<nsIDOMWindow> win = do_GetInterface(item);
            if (win) {
                FlushLayoutForTree(win);
            }
        }
    }
}

bool
DocumentRendererNativeIDChild::RenderDocument(nsIDOMWindow* window, const PRInt32& x,
                                      const PRInt32& y, const PRInt32& w,
                                      const PRInt32& h, const nsString& aBGColor,
                                      const PRUint32& flags, const PRBool& flush,
                                      const gfxMatrix& aMatrix,
                                      const PRInt32& nativeID)
{
    if (!nativeID)
        return false;

    if (flush)
        FlushLayoutForTree(window);

    nsCOMPtr<nsPresContext> presContext;
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(window);
    if (win) {
        nsIDocShell* docshell = win->GetDocShell();
        if (docshell) {
            docshell->GetPresContext(getter_AddRefs(presContext));
        }
    }
    if (!presContext)
        return false;

    nscolor bgColor;
    nsCSSParser parser;
    nsresult rv = parser.ParseColorString(PromiseFlatString(aBGColor), nsnull, 0, &bgColor);
    if (NS_FAILED(rv))
        return false;

    nsIPresShell* presShell = presContext->PresShell();

    nsRect r(x, y, w, h);

    
    nsRefPtr<gfxASurface> surf;
#ifdef MOZ_X11
    
    
    Display* dpy = DISPLAY();
    int depth = DefaultDepth(dpy, DefaultScreen(dpy));
    XVisualInfo vinfo;
    int foundVisual = XMatchVisualInfo(dpy,
                                       DefaultScreen(dpy),
                                       depth,
                                       TrueColor,
                                       &vinfo);
    if (!foundVisual)
        return false;

    surf = new gfxXlibSurface(dpy, nativeID, vinfo.visual);
#else
    NS_ERROR("NativeID handler not implemented for your platform");
#endif

    nsRefPtr<gfxContext> ctx = new gfxContext(surf);
    ctx->SetMatrix(aMatrix);

    presShell->RenderDocument(r, flags, bgColor, ctx);
#ifdef MOZ_X11
    
    
    
    
    
    XSync(dpy, False);
#endif
    return true;
}
