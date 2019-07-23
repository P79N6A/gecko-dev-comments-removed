





































#include "TabChild.h"

#include "nsIWebBrowser.h"
#include "nsEmbedCID.h"
#include "nsComponentManagerUtils.h"
#include "nsIBaseWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsThreadUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "mozilla/ipc/DocumentRendererChild.h"

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

using namespace mozilla::dom;

TabChild::TabChild()
{
    printf("creating %d!\n", NS_IsMainThread());

#ifdef MOZ_WIDGET_GTK2
    gtk_init(NULL, NULL);
#endif

    mWebNav = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);
    if (!mWebNav) {
        NS_ERROR("Couldn't create a nsWebBrowser?");
        return;
    }

    nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(mWebNav));
    docShellItem->SetItemType(nsIDocShellTreeItem::typeContentWrapper);
}

bool
TabChild::RecvcreateWidget(const MagicWindowHandle& parentWidget)
{
    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebNav);
    if (!baseWindow) {
        NS_ERROR("mWebNav doesn't QI to nsIBaseWindow");
        return true;
    }

#ifdef MOZ_WIDGET_GTK2
    GtkWidget* win = gtk_plug_new((GdkNativeWindow)parentWidget);
    gtk_widget_show(win);
#elif defined(XP_WIN)
    HWND win = parentWidget;
#else
#error You lose!
#endif

    baseWindow->InitWindow(win, 0, 0, 0, 0, 0);
    baseWindow->Create();
    baseWindow->SetVisibility(PR_TRUE);

    return true;
}

bool
TabChild::RecvdestroyWidget()
{
    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebNav);
    if (baseWindow)
        baseWindow->Destroy();

    return true;
}

TabChild::~TabChild()
{
    
}

bool
TabChild::RecvloadURL(const nsCString& uri)
{
    printf("loading %s, %d\n", uri.get(), NS_IsMainThread());

    nsresult rv = mWebNav->LoadURI(NS_ConvertUTF8toUTF16(uri).get(),
                                   nsIWebNavigation::LOAD_FLAGS_NONE,
                                   NULL, NULL, NULL);
    if (NS_FAILED(rv)) {
        NS_WARNING("mWebNav->LoadURI failed. Eating exception, what else can I do?");
    }
    return true;
}

bool
TabChild::Recvmove(const PRUint32& x,
                     const PRUint32& y,
                     const PRUint32& width,
                     const PRUint32& height)
{
    printf("[TabChild] MOVE to (x,y)=(%ud, %ud), (w,h)= (%ud, %ud)\n",
           x, y, width, height);

    nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(mWebNav);
    baseWin->SetPositionAndSize(x, y, width, height, PR_TRUE);
    return true;
}

mozilla::ipc::PDocumentRendererChild*
TabChild::AllocPDocumentRenderer(
        const PRInt32& x,
        const PRInt32& y,
        const PRInt32& w,
        const PRInt32& h,
        const nsString& bgcolor,
        const PRUint32& flags,
        const bool& flush)
{
    return new mozilla::ipc::DocumentRendererChild();
}

bool
TabChild::DeallocPDocumentRenderer(
        mozilla::ipc::PDocumentRendererChild* __a,
        const PRUint32& w,
        const PRUint32& h,
        const nsCString& data)
{
    delete __a;
    return true;
}

bool
TabChild::RecvPDocumentRendererConstructor(
        mozilla::ipc::PDocumentRendererChild *__a,
        const PRInt32& aX,
        const PRInt32& aY,
        const PRInt32& aW,
        const PRInt32& aH,
        const nsString& bgcolor,
        const PRUint32& flags,
        const bool& flush)
{
    mozilla::ipc::DocumentRendererChild *render = 
        static_cast<mozilla::ipc::DocumentRendererChild *>(__a);

    nsCOMPtr<nsIWebBrowser> browser = do_QueryInterface(mWebNav);
    if (!browser)
        return true; 
    nsCOMPtr<nsIDOMWindow> window;
    if (NS_FAILED(browser->GetContentDOMWindow(getter_AddRefs(window))) ||
        !window)
    {
        return true; 
    }

    PRUint32 width, height;
    nsCString data;
    bool ret = render->RenderDocument(window, aX, aY, aW, aH, bgcolor, flags, flush,
                                      width, height, data);
    if (!ret)
        return true; 

    return SendPDocumentRendererDestructor(__a, width, height, data);
}
