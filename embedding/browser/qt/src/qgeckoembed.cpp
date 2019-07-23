






































#include "qgeckoembed.h"

#include "EmbedWindow.h"
#include "EmbedProgress.h"
#include "EmbedStream.h"
#include "EmbedEventListener.h"
#include "EmbedContentListener.h"
#include "EmbedWindowCreator.h"
#include "qgeckoglobals.h"

#include "nsIAppShell.h"
#include <nsIDocShell.h>
#include <nsIWebProgress.h>
#include <nsIWebNavigation.h>
#include <nsIWebBrowser.h>
#include <nsISHistory.h>
#include <nsIWebBrowserChrome.h>
#include "nsIWidget.h"
#include "nsCRT.h"
#include <nsIWindowWatcher.h>
#include <nsILocalFile.h>
#include <nsEmbedAPI.h>
#include <nsWidgetsCID.h>
#include <nsIDOMUIEvent.h>
#include <nsIInterfaceRequestor.h>
#include <nsIComponentManager.h>
#include <nsIFocusController.h>
#include <nsProfileDirServiceProvider.h>
#include <nsIGenericFactory.h>
#include <nsIComponentRegistrar.h>
#include <nsVoidArray.h>
#include <nsIDOMDocument.h>
#include <nsIDOMBarProp.h>
#include <nsIDOMWindow.h>
#include <nsIDOMEventReceiver.h>
#include <nsCOMPtr.h>
#include <nsPIDOMWindow.h>

#include "prenv.h"

#include <qlayout.h>

class QGeckoEmbedPrivate
{
public:
    QGeckoEmbedPrivate(QGeckoEmbed *qq);
    ~QGeckoEmbedPrivate();

    QGeckoEmbed *q;

    QWidget *mMainWidget;

    
    EmbedWindow                   *window;
    nsCOMPtr<nsISupports>          windowGuard;
    EmbedProgress                 *progress;
    nsCOMPtr<nsISupports>          progressGuard;
    EmbedContentListener          *contentListener;
    nsCOMPtr<nsISupports>          contentListenerGuard;
    EmbedEventListener            *eventListener;
    nsCOMPtr<nsISupports>          eventListenerGuard;
    EmbedStream                   *stream;
    nsCOMPtr<nsISupports>          streamGuard;

    nsCOMPtr<nsIWebNavigation>     navigation;
    nsCOMPtr<nsISHistory>          sessionHistory;

    
    nsCOMPtr<nsIDOMEventReceiver>  eventReceiver;

    
    PRUint32                       chromeMask;

    bool isChrome;
    bool chromeLoaded;
    bool listenersAttached;

    void initGUI();
    void init();
    void ApplyChromeMask();
};


QGeckoEmbedPrivate::QGeckoEmbedPrivate(QGeckoEmbed *qq)
    : q(qq),
      mMainWidget(0),
      chromeMask(nsIWebBrowserChrome::CHROME_ALL),
      isChrome(FALSE),
      chromeLoaded(FALSE),
      listenersAttached(FALSE)
{
    initGUI();
    init();
}

QGeckoEmbedPrivate::~QGeckoEmbedPrivate()
{
    QGeckoGlobals::removeEngine(q);
    QGeckoGlobals::popStartup();
}

void
QGeckoEmbedPrivate::init()
{
    QGeckoGlobals::initializeGlobalObjects();
    QGeckoGlobals::pushStartup();
    QGeckoGlobals::addEngine(q);

    
    
    
    window = new EmbedWindow();
    windowGuard = NS_STATIC_CAST(nsIWebBrowserChrome *, window);
    window->Init(q);
    
    
    
    progress = new EmbedProgress(q);
    progressGuard = NS_STATIC_CAST(nsIWebProgressListener *,
                                   progress);

    
    
    
    contentListener = new EmbedContentListener(q);
    contentListenerGuard = NS_STATIC_CAST(nsISupports*,
                                          NS_STATIC_CAST(nsIURIContentListener*, contentListener));

    
    
    eventListener = new EmbedEventListener(q);
    eventListenerGuard =
        NS_STATIC_CAST(nsISupports *, NS_STATIC_CAST(nsIDOMKeyListener *,
                                                     eventListener));

    
    static int initialized = PR_FALSE;
    
    if (!initialized) {
        
        nsCOMPtr<nsIWindowCreator> windowCreator = new EmbedWindowCreator();

        
        nsCOMPtr<nsIWindowWatcher> watcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID);
        if (watcher)
            watcher->SetWindowCreator(windowCreator);
        initialized = PR_TRUE;
    }

    
    nsCOMPtr<nsIWebBrowser> webBrowser;
    window->GetWebBrowser(getter_AddRefs(webBrowser));

    
    navigation = do_QueryInterface(webBrowser);

    
    
    
    sessionHistory = do_CreateInstance(NS_SHISTORY_CONTRACTID);
    navigation->SetSessionHistory(sessionHistory);

    
    window->CreateWindow();

    
    nsCOMPtr<nsISupportsWeakReference> supportsWeak;
    supportsWeak = do_QueryInterface(progressGuard);
    nsCOMPtr<nsIWeakReference> weakRef;
    supportsWeak->GetWeakReference(getter_AddRefs(weakRef));
    webBrowser->AddWebBrowserListener(weakRef,
                                      NS_GET_IID(nsIWebProgressListener));

    
    webBrowser->SetParentURIContentListener(contentListener);

    
    nsCOMPtr<nsIWidget> qtWidget;
    window->mBaseWindow->GetMainWidget(getter_AddRefs(qtWidget));
    
    mMainWidget = NS_STATIC_CAST(QWidget*, qtWidget->GetNativeData(NS_NATIVE_WINDOW));

    
    ApplyChromeMask();
}

void
QGeckoEmbedPrivate::initGUI()
{
    QBoxLayout *l = new QHBoxLayout(q);
    l->setAutoAdd(TRUE);
}

void
QGeckoEmbedPrivate::ApplyChromeMask()
{
   if (window) {
      nsCOMPtr<nsIWebBrowser> webBrowser;
      window->GetWebBrowser(getter_AddRefs(webBrowser));

      nsCOMPtr<nsIDOMWindow> domWindow;
      webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
      if (domWindow) {
          nsCOMPtr<nsIDOMBarProp> scrollbars;
          domWindow->GetScrollbars(getter_AddRefs(scrollbars));
          if (scrollbars) {

              scrollbars->SetVisible(
                  chromeMask & nsIWebBrowserChrome::CHROME_SCROLLBARS ?
                  PR_TRUE : PR_FALSE);
          }
      }
   }
}




QGeckoEmbed::QGeckoEmbed(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    d = new QGeckoEmbedPrivate(this);
}

QGeckoEmbed::~QGeckoEmbed()
{
    delete d;
}


bool
QGeckoEmbed::canGoBack() const
{
    PRBool retval = PR_FALSE;
    if (d->navigation)
        d->navigation->GetCanGoBack(&retval);
    return retval;
}

bool
QGeckoEmbed::canGoForward() const
{
    PRBool retval = PR_FALSE;
    if (d->navigation)
        d->navigation->GetCanGoForward(&retval);
    return retval;
}

void
QGeckoEmbed::loadURL(const QString &url)
{
    if (!url.isEmpty()) {
        d->navigation->LoadURI((const PRUnichar*)url.ucs2(),
                               nsIWebNavigation::LOAD_FLAGS_NONE, 
                               nsnull,                            
                               nsnull,                            
                               nsnull);
    }
}

void
QGeckoEmbed::stopLoad()
{
    if (d->navigation)
        d->navigation->Stop(nsIWebNavigation::STOP_NETWORK);
}

void
QGeckoEmbed::goForward()
{
    if (d->navigation)
        d->navigation->GoForward();
}

void
QGeckoEmbed::goBack()
{
    if (d->navigation)
        d->navigation->GoBack();
}

void
QGeckoEmbed::renderData(const QCString &data, const QString &baseURI,
                            const QString &mimeType)
{
    openStream(baseURI, mimeType);
    appendData(data);
    closeStream();
}

int
QGeckoEmbed::openStream(const QString &baseURI, const QString &mimeType)
{
    nsresult rv;

    if (!d->stream) {
        d->stream = new EmbedStream();
        d->streamGuard = do_QueryInterface(d->stream);
        d->stream->InitOwner(this);
        rv = d->stream->Init();
        if (NS_FAILED(rv))
            return rv;
    }

    rv = d->stream->OpenStream(baseURI, mimeType);
    return rv;
}

int
QGeckoEmbed::appendData(const QCString &data)
{
    if (!d->stream)
        return NS_ERROR_FAILURE;

    
    
    contentStateChanged();

    return d->stream->AppendToStream(data, data.length());
}

int
QGeckoEmbed::closeStream()
{
    nsresult rv;

    if (!d->stream)
        return NS_ERROR_FAILURE;
    rv = d->stream->CloseStream();

    
    d->stream = 0;
    d->streamGuard = 0;

    return rv;
}

void
QGeckoEmbed::reload(ReloadFlags flags)
{
    int qeckoFlags = 0;
    switch(flags) {
    case Normal:
        qeckoFlags = 0;
        break;
    case BypassCache:
        qeckoFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE;
        break;
    case BypassProxy:
        qeckoFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
        break;
    case BypassProxyAndCache:
        qeckoFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE |
                     nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
        break;
    case CharsetChange:
        qeckoFlags = nsIWebNavigation::LOAD_FLAGS_CHARSET_CHANGE;
        break;
    default:
        qeckoFlags = 0;
        break;
    }


    nsCOMPtr<nsIWebNavigation> wn;

    if (d->sessionHistory) {
        wn = do_QueryInterface(d->sessionHistory);
    }
    if (!wn)
        wn = d->navigation;

    if (wn)
        wn->Reload(qeckoFlags);
}

bool
QGeckoEmbed::domKeyDownEvent(nsIDOMKeyEvent *keyEvent)
{
    emit domKeyDown(keyEvent);
    return false;
}

bool
QGeckoEmbed::domKeyPressEvent(nsIDOMKeyEvent *keyEvent)
{
    emit domKeyPress(keyEvent);
    return false;
}

bool
QGeckoEmbed::domKeyUpEvent(nsIDOMKeyEvent *keyEvent)
{
    emit domKeyUp(keyEvent);
    return false;
}

bool
QGeckoEmbed::domMouseDownEvent(nsIDOMMouseEvent *mouseEvent)
{
    emit domMouseDown(mouseEvent);
    return false;
}

bool
QGeckoEmbed::domMouseUpEvent(nsIDOMMouseEvent *mouseEvent)
{
    emit domMouseUp(mouseEvent);
    return false;
}

bool
QGeckoEmbed::domMouseClickEvent(nsIDOMMouseEvent *mouseEvent)
{
    emit domMouseClick(mouseEvent);
    return false;
}

bool
QGeckoEmbed::domMouseDblClickEvent(nsIDOMMouseEvent *mouseEvent)
{
    emit domMouseDblClick(mouseEvent);
    return false;
}

bool
QGeckoEmbed::domMouseOverEvent(nsIDOMMouseEvent *mouseEvent)
{
    emit domMouseOver(mouseEvent);
    return false;
}

bool
QGeckoEmbed::domMouseOutEvent(nsIDOMMouseEvent *mouseEvent)
{
    emit domMouseOut(mouseEvent);
    return false;
}

bool
QGeckoEmbed::domActivateEvent(nsIDOMUIEvent *event)
{
    emit domActivate(event);
    return false;
}

bool
QGeckoEmbed::domFocusInEvent(nsIDOMUIEvent *event)
{
    emit domFocusIn(event);
    return false;
}

bool
QGeckoEmbed::domFocusOutEvent(nsIDOMUIEvent *event)
{
    emit domFocusOut(event);
    return false;
}

void
QGeckoEmbed::emitScriptStatus(const QString &str)
{
    emit jsStatusMessage(str);
}

void
QGeckoEmbed::emitLinkStatus(const QString &str)
{
    emit linkMessage(str);
}

int
QGeckoEmbed::chromeMask() const
{
    return d->chromeMask;
}

void
QGeckoEmbed::setChromeMask(int mask)
{
    d->chromeMask = mask;

    d->ApplyChromeMask();
}

void
QGeckoEmbed::resizeEvent(QResizeEvent *e)
{
    d->window->SetDimensions(nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER,
                              0, 0, e->size().width(), e->size().height());
}

nsIDOMDocument*
QGeckoEmbed::document() const
{
    nsIDOMDocument *doc = 0;

    nsCOMPtr<nsIDOMWindow> window;
    nsCOMPtr<nsIWebBrowser> webBrowser;

    d->window->GetWebBrowser(getter_AddRefs(webBrowser));

    webBrowser->GetContentDOMWindow(getter_AddRefs(window));
    if (window) {
        window->GetDocument(&doc);
    }

    return doc;
}

void
QGeckoEmbed::contentStateChanged()
{
    
    if (d->listenersAttached && !d->isChrome)
        return;

    setupListener();

    if (!d->eventReceiver)
        return;

    attachListeners();
}

void
QGeckoEmbed::contentFinishedLoading()
{
    if (d->isChrome) {
        
        d->chromeLoaded = PR_TRUE;

        
        nsCOMPtr<nsIWebBrowser> webBrowser;
        d->window->GetWebBrowser(getter_AddRefs(webBrowser));

        
        nsCOMPtr<nsIDOMWindow> domWindow;
        webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
        if (!domWindow) {
            NS_WARNING("no dom window in content finished loading\n");
            return;
        }

        
        domWindow->SizeToContent();

        
        
        PRBool visibility;
        d->window->GetVisibility(&visibility);
        if (visibility)
            d->window->SetVisibility(PR_TRUE);
    }
}

void
QGeckoEmbed::setupListener()
{
    if (d->eventReceiver)
        return;

    nsCOMPtr<nsPIDOMWindow> piWin;
    GetPIDOMWindow(getter_AddRefs(piWin));

    if (!piWin)
        return;

    d->eventReceiver = do_QueryInterface(piWin->GetChromeEventHandler());
}

void
QGeckoEmbed::attachListeners()
{
    if (!d->eventReceiver || d->listenersAttached)
        return;

    nsIDOMEventListener *eventListener =
        NS_STATIC_CAST(nsIDOMEventListener *,
                       NS_STATIC_CAST(nsIDOMKeyListener *, d->eventListener));

    
    nsresult rv;
    rv = d->eventReceiver->AddEventListenerByIID(eventListener,
                                                 NS_GET_IID(nsIDOMKeyListener));
    if (NS_FAILED(rv)) {
        NS_WARNING("Failed to add key listener\n");
        return;
    }

    rv = d->eventReceiver->AddEventListenerByIID(eventListener,
                                                 NS_GET_IID(nsIDOMMouseListener));
    if (NS_FAILED(rv)) {
        NS_WARNING("Failed to add mouse listener\n");
        return;
    }

    rv = d->eventReceiver->AddEventListenerByIID(eventListener,
                                                NS_GET_IID(nsIDOMUIListener));
    if (NS_FAILED(rv)) {
        NS_WARNING("Failed to add UI listener\n");
        return;
    }

    
    d->listenersAttached = PR_TRUE;
}

EmbedWindow * QGeckoEmbed::window() const
{
    return d->window;
}


int QGeckoEmbed::GetPIDOMWindow(nsPIDOMWindow **aPIWin)
{
    *aPIWin = nsnull;

    
    nsCOMPtr<nsIWebBrowser> webBrowser;
    d->window->GetWebBrowser(getter_AddRefs(webBrowser));

    
    nsCOMPtr<nsIDOMWindow> domWindow;
    webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    if (!domWindow)
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsPIDOMWindow> domWindowPrivate = do_QueryInterface(domWindow);
    
    *aPIWin = domWindowPrivate->GetPrivateRoot();

    if (*aPIWin) {
        NS_ADDREF(*aPIWin);
        return NS_OK;
    }

    return NS_ERROR_FAILURE;

}

void QGeckoEmbed::setIsChrome(bool isChrome)
{
    d->isChrome = isChrome;
}

bool QGeckoEmbed::isChrome() const
{
    return d->isChrome;
}

bool QGeckoEmbed::chromeLoaded() const
{
    return d->chromeLoaded;
}

QString QGeckoEmbed::url() const
{
    nsCOMPtr<nsIURI> uri;
    d->navigation->GetCurrentURI(getter_AddRefs(uri));
    nsCAutoString acstring;
    uri->GetSpec(acstring);

    return QString::fromUtf8(acstring.get());
}

QString QGeckoEmbed::resolvedUrl(const QString &relativepath) const
{
    nsCOMPtr<nsIURI> uri;
    d->navigation->GetCurrentURI(getter_AddRefs(uri));
    nsCAutoString rel;
    rel.Assign(relativepath.utf8().data());
    nsCAutoString resolved;
    uri->Resolve(rel, resolved);

    return QString::fromUtf8(resolved.get());
}

void QGeckoEmbed::initialize(const char *aDir, const char *aName)
{
    QGeckoGlobals::setProfilePath(aDir, aName);
}

