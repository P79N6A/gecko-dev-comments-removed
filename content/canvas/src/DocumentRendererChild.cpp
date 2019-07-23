



































#include "mozilla/ipc/DocumentRendererChild.h"
#include "gfxImageSurface.h"
#include "gfxPattern.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsICSSParser.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsColor.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsLayoutUtils.h"

using namespace mozilla::ipc;

DocumentRendererChild::DocumentRendererChild()
{}

DocumentRendererChild::~DocumentRendererChild()
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
DocumentRendererChild::RenderDocument(nsIDOMWindow *window, const PRInt32& x, const PRInt32& y, const PRInt32& w, const PRInt32& h,
                                      const nsString& aBGColor, const PRUint32& flags, const PRBool& flush, 
                                      PRUint32& _width, PRUint32& _height, nsCString& data)
{
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
    nsCOMPtr<nsICSSParser> parser = do_CreateInstance("@mozilla.org/content/css-parser;1");
    nsresult rv = parser->ParseColorString(PromiseFlatString(aBGColor),
                                           nsnull, 0, &bgColor);
    if (NS_FAILED(rv))
        return false;

    nsIPresShell* presShell = presContext->PresShell();

    nsRect r(x, y, w, h);

    _width = nsPresContext::AppUnitsToIntCSSPixels(w);
    _height = nsPresContext::AppUnitsToIntCSSPixels(h);

    
    data.SetLength(_width * _height * 4);
    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(reinterpret_cast<PRUint8*>(const_cast<char*>(data.get())),
                                                         gfxIntSize(_width, _height),
                                                         4 * _width, gfxASurface::ImageFormatARGB32);
    nsRefPtr<gfxContext> ctx = new gfxContext(surf);

    PRBool oldDisableValue = nsLayoutUtils::sDisableGetUsedXAssertions;
    nsLayoutUtils::sDisableGetUsedXAssertions = oldDisableValue || !flush;
    presShell->RenderDocument(r, flags, bgColor, ctx);
    nsLayoutUtils::sDisableGetUsedXAssertions = oldDisableValue;

    return true;
}
