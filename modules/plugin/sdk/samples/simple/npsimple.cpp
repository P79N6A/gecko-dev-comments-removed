






















































#include <stdio.h>
#include <string.h>
#include "nsplugin.h"
#include "nsIServiceManager.h"
#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsMemory.h"
#include "nsString.h"
#include "simpleCID.h"

#include "nsISimplePluginInstance.h"
#include "nsIScriptablePlugin.h"




#ifdef _WINDOWS 
#include <windows.h>
#endif 



#ifdef XP_UNIX
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <gtk/gtk.h>
#include <gdksuperwin.h>
#include <gtkmozbox.h>
#endif 

#ifdef XP_UNIX

gboolean draw (GtkWidget *widget, GdkEventExpose *event, gpointer data);

#endif














 




#if defined(XP_WIN) 
typedef struct _PlatformInstance
{
    HWND		fhWnd;
    WNDPROC		fDefaultWindowProc;
} PlatformInstance;





#elif defined(XP_UNIX)
typedef struct _PlatformInstance
{
    Window 		       window;
    GtkWidget         *moz_box;
    GdkSuperWin       *superwin;
    GtkWidget         *label;
    Display *		   display;
    uint32 		       x, y;
    uint32 		       width, height;
} PlatformInstance;





#elif defined(XP_MAC)
typedef struct _PlatformInstance
{
    int			placeholder;
} PlatformInstance;





#else
typedef struct _PlatformInstance
{
    int			placeholder;
} PlatformInstance;
#endif 



static NS_DEFINE_CID(kSimplePluginCID, NS_SIMPLEPLUGIN_CID);
static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);

#define PLUGIN_MIME_TYPE "application/x-simple"

static const char kPluginName[] = "Simple Sample Plug-in";
static const char kPluginDescription[] = "Demonstrates a simple plug-in.";

static const char* kMimeTypes[] = {
    PLUGIN_MIME_TYPE
};

static const char* kMimeDescriptions[] = {
    "Simple Sample Plug-in"
};

static const char* kFileExtensions[] = {
    "smp"
};

static const PRInt32 kNumMimeTypes = sizeof(kMimeTypes) / sizeof(*kMimeTypes);










class SimplePluginInstance : 
    public nsIPluginInstance, 
    public nsIScriptablePlugin,
    public nsISimplePluginInstance {

public:
    
    
    static NS_METHOD
    Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    static NS_METHOD
    RegisterSelf(nsIComponentManager* aCompMgr,
                 nsIFile* aPath,
                 const char* aRegistryLocation,
                 const char* aComponentType,
                 const nsModuleComponentInfo *info);

    static NS_METHOD
    UnregisterSelf(nsIComponentManager* aCompMgr,
                   nsIFile* aPath,
                   const char* aRegistryLocation,
                   const nsModuleComponentInfo *info);


    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTABLEPLUGIN
    NS_DECL_NSISIMPLEPLUGININSTANCE

    
    

    
    
    
    
    
    
    NS_IMETHOD
    HandleEvent(nsPluginEvent* event, PRBool* handled);

    
    

    NS_IMETHOD
    Initialize(nsIPluginInstancePeer* peer);

    
    NS_IMETHOD
    GetPeer(nsIPluginInstancePeer* *result);

    
    NS_IMETHOD
    Start(void);

    
    
    
    
    
    
    
    
    

    NS_IMETHOD
    Stop(void);

    NS_IMETHOD
    Destroy(void);

    
    NS_IMETHOD
    SetWindow(nsPluginWindow* window);

    NS_IMETHOD
    NewStream(nsIPluginStreamListener** listener);

    
    NS_IMETHOD
    Print(nsPluginPrint* platformPrint);

    NS_IMETHOD
    GetValue(nsPluginInstanceVariable variable, void *value);


    
    

    SimplePluginInstance(void);
    virtual ~SimplePluginInstance(void);

    void            PlatformNew(void);
    nsresult        PlatformDestroy(void);
    nsresult    	PlatformSetWindow(nsPluginWindow* window);
    PRInt16         PlatformHandleEvent(nsPluginEvent* event);

    void SetMode(nsPluginMode mode) { fMode = mode; }

#ifdef XP_UNIX
    NS_IMETHOD Repaint(void);
#endif

#ifdef XP_WIN
    static LRESULT CALLBACK 
    PluginWindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif

    char*                       fText;

protected:
    nsIPluginInstancePeer*      fPeer;
    nsPluginWindow*             fWindow;
    nsPluginMode                fMode;
    PlatformInstance            fPlatform;
};





class SimplePluginStreamListener : public nsIPluginStreamListener {
public:

    NS_DECL_ISUPPORTS

    
    

    






    NS_IMETHOD
    OnStartBinding(nsIPluginStreamInfo* pluginInfo);

    









    NS_IMETHOD
    OnDataAvailable(nsIPluginStreamInfo* pluginInfo, 
                                            nsIInputStream* input, 
                                            PRUint32 length);

    NS_IMETHOD
    OnFileAvailable(nsIPluginStreamInfo* pluginInfo, const char* fileName);

    










    NS_IMETHOD
    OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status);

    NS_IMETHOD
    OnNotify(const char* url, nsresult status);

    NS_IMETHOD
    GetStreamType(nsPluginStreamType *result);

    
    

    SimplePluginStreamListener(SimplePluginInstance* inst, const char* url);
    virtual ~SimplePluginStreamListener(void);

protected:
    const char*                 fMessageName;
};






static const nsModuleComponentInfo gComponentInfo[] = {
    { "Simple Plugin",
      NS_SIMPLEPLUGIN_CID,
      NS_INLINE_PLUGIN_CONTRACTID_PREFIX PLUGIN_MIME_TYPE,
      SimplePluginInstance::Create,
      SimplePluginInstance::RegisterSelf,
      SimplePluginInstance::UnregisterSelf },
};

NS_IMPL_NSGETMODULE(SimplePlugin, gComponentInfo)






NS_METHOD
SimplePluginInstance::GetScriptablePeer(void **aScriptablePeer)
{
   
   

   
   
   
   
   
   
   

   *aScriptablePeer = static_cast<nsISimplePluginInstance *>(this);
   NS_ADDREF(static_cast<nsISimplePluginInstance *>(*aScriptablePeer));
   return NS_OK;
}

NS_METHOD
SimplePluginInstance::GetScriptableInterface(nsIID **aScriptableInterface)
{
  *aScriptableInterface = (nsIID *)nsMemory::Alloc(sizeof(nsIID));
  NS_ENSURE_TRUE(*aScriptableInterface, NS_ERROR_OUT_OF_MEMORY);

  **aScriptableInterface = NS_GET_IID(nsISimplePluginInstance);

  return NS_OK;
}





NS_METHOD
SimplePluginInstance::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    SimplePluginInstance* plugin = new SimplePluginInstance();
    if (! plugin)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    NS_ADDREF(plugin);
    rv = plugin->QueryInterface(aIID, aResult);
    NS_RELEASE(plugin);
    return rv;
}

NS_METHOD
SimplePluginInstance::RegisterSelf(nsIComponentManager* aCompMgr,
                                   nsIFile* aPath,
                                   const char* aRegistryLocation,
                                   const char* aComponentType,
                                   const nsModuleComponentInfo *info)
{
    nsresult rv;

    nsIServiceManager *svcMgr;
    rv = aCompMgr->QueryInterface(NS_GET_IID(nsIServiceManager),
                                  reinterpret_cast<void**>(&svcMgr));
    if (NS_FAILED(rv))
        return rv;

    nsIPluginManager* pm;
    rv = svcMgr->GetService(kPluginManagerCID,
                            NS_GET_IID(nsIPluginManager),
                            reinterpret_cast<void**>(&pm));
    NS_RELEASE(svcMgr);

    if (NS_SUCCEEDED(rv)) {
        rv = pm->RegisterPlugin(kSimplePluginCID,
                                kPluginName,
                                kPluginDescription,
                                kMimeTypes,
                                kMimeDescriptions,
                                kFileExtensions,
                                kNumMimeTypes);

        NS_RELEASE(pm);
    }

    return rv;
}


NS_METHOD
SimplePluginInstance::UnregisterSelf(nsIComponentManager* aCompMgr,
                                     nsIFile* aPath,
                                     const char* aRegistryLocation,
                                     const nsModuleComponentInfo *info)
{
    nsresult rv;

    nsIServiceManager *svcMgr;
    rv = aCompMgr->QueryInterface(NS_GET_IID(nsIServiceManager),
                                  reinterpret_cast<void**>(&svcMgr));
    if (NS_FAILED(rv))
        return rv;

    nsIPluginManager* pm;
    rv = svcMgr->GetService(kPluginManagerCID,
                            NS_GET_IID(nsIPluginManager),
                            reinterpret_cast<void**>(&pm));
    NS_RELEASE(svcMgr);

    if (NS_SUCCEEDED(rv)) {
        rv = pm->UnregisterPlugin(kSimplePluginCID);
        NS_RELEASE(pm);
    }

    return rv;
}


SimplePluginInstance::SimplePluginInstance(void)
    : fText(NULL), fPeer(NULL), fWindow(NULL), fMode(nsPluginMode_Embedded)
{
    static const char text[] = "Hello World!";
    fText = (char*) nsMemory::Clone(text, sizeof(text));

#ifdef XP_UNIX
    fPlatform.moz_box = nsnull;
    fPlatform.superwin = nsnull;
    fPlatform.label = nsnull;
#endif

}

SimplePluginInstance::~SimplePluginInstance(void)
{
    if(fText)
        nsMemory::Free(fText);
    PlatformDestroy(); 
}




NS_IMPL_ISUPPORTS3(SimplePluginInstance, nsIPluginInstance, nsISimplePluginInstance, nsIScriptablePlugin)












NS_METHOD
SimplePluginInstance::Initialize(nsIPluginInstancePeer* peer)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::Initialize\n");
#endif
    
    NS_ASSERTION(peer != NULL, "null peer");

    fPeer = peer;
    nsIPluginTagInfo* taginfo;
    const char* const* names = nsnull;
    const char* const* values = nsnull;
    PRUint16 count = 0;
    nsresult result;

    peer->AddRef();
    result = peer->GetMode(&fMode);
    if (NS_FAILED(result)) return result;

   result = peer->QueryInterface(NS_GET_IID(nsIPluginTagInfo), (void **)&taginfo);

    if (NS_SUCCEEDED(result))
    {
        taginfo->GetAttributes(count, names, values);
        NS_IF_RELEASE(taginfo);
    }

#ifdef NS_DEBUG
    printf("Attribute count = %d\n", count);

    for (int i = 0; i < count; i++)
    {
        printf("plugin param=%s, value=%s\n", names[i], values[i]);
    }
#endif

    PlatformNew(); 	
    return NS_OK;
}

NS_METHOD
SimplePluginInstance::GetPeer(nsIPluginInstancePeer* *result)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::GetPeer\n");
#endif

    fPeer->AddRef();
    *result = fPeer;
    return NS_OK;
}

NS_METHOD
SimplePluginInstance::Start(void)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::Start\n");
#endif

    return NS_OK;
}

NS_METHOD
SimplePluginInstance::Stop(void)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::Stop\n");
#endif

    return NS_OK;
}

NS_METHOD
SimplePluginInstance::Destroy(void)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::Destroy\n");
#endif

    return NS_OK;
}















NS_METHOD
SimplePluginInstance::SetWindow(nsPluginWindow* window)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::SetWindow\n");
#endif

    nsresult result;

    






    result = PlatformSetWindow(window);
    fWindow = window;
    return result;
}

NS_METHOD
SimplePluginInstance::NewStream(nsIPluginStreamListener** listener)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::NewStream\n");
#endif

    if(listener != NULL)
    {
        SimplePluginStreamListener* sl = 
                new SimplePluginStreamListener(this, "http://www.mozilla.org");
        if(!sl)
            return NS_ERROR_UNEXPECTED;
        sl->AddRef();
        *listener = sl;
    }
    
    return NS_OK;
}



NS_IMETHODIMP SimplePluginInstance::GetText(char * *aText)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::GetText\n");
#endif
    
    if(!fText)
    {
        *aText = NULL;
        return NS_OK;        
    }
    char* ptr = *aText = (char*) nsMemory::Clone(fText, strlen(fText)+1);
    return ptr ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
NS_IMETHODIMP SimplePluginInstance::SetText(const char * aText)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::SetText\n");
#endif

    if(fText)
    {
        nsMemory::Free(fText);
        fText = NULL;
    }

    if(aText)
    {
        fText = (char*) nsMemory::Clone(aText, strlen(aText)+1);
        if(!fText)
            return NS_ERROR_OUT_OF_MEMORY;

#ifdef XP_WIN
        if(fPlatform.fhWnd) {
            InvalidateRect( fPlatform.fhWnd, NULL, TRUE );
            UpdateWindow( fPlatform.fhWnd );
        }
#endif

#ifdef XP_UNIX
        
        Repaint();
#endif

    }

    return NS_OK;
}





NS_METHOD
SimplePluginInstance::Print(nsPluginPrint* printInfo)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::Print\n");
#endif

    if (printInfo == NULL)
        return NS_ERROR_FAILURE;

    if (printInfo->mode == nsPluginMode_Full) {
        
















        
        printInfo->print.fullPrint.pluginPrinted = PR_FALSE;
    }
    else {	
        










    }
    return NS_OK;
}











NS_METHOD
SimplePluginInstance::HandleEvent(nsPluginEvent* event, PRBool* handled)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::HandleEvent\n");
#endif

    *handled = (PRBool)PlatformHandleEvent(event);
    return NS_OK;
}

NS_METHOD
SimplePluginInstance::GetValue(nsPluginInstanceVariable variable, void *value)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::GetValue\n");
#endif

    return NS_ERROR_FAILURE;
}





SimplePluginStreamListener::SimplePluginStreamListener(SimplePluginInstance* inst,
                                                       const char* msgName)
    : fMessageName(msgName)
{
    char msg[256];
    sprintf(msg, "### Creating SimplePluginStreamListener for %s\n", fMessageName);
}

SimplePluginStreamListener::~SimplePluginStreamListener(void)
{
    char msg[256];
    sprintf(msg, "### Destroying SimplePluginStreamListener for %s\n", fMessageName);
}




NS_IMPL_ISUPPORTS1(SimplePluginStreamListener, nsIPluginStreamListener)

NS_METHOD
SimplePluginStreamListener::OnStartBinding(nsIPluginStreamInfo* pluginInfo)
{
#ifdef NS_DEBUG
    printf("SimplePluginStreamListener::OnStartBinding\n");
#endif

    char msg[256];
    sprintf(msg, "### Opening plugin stream for %s\n", fMessageName);
    return NS_OK;
}

NS_METHOD
SimplePluginStreamListener::OnDataAvailable(nsIPluginStreamInfo* pluginInfo, 
                                            nsIInputStream* input, 
                                            PRUint32 length)
{
#ifdef NS_DEBUG
    printf("SimplePluginStreamListener::OnDataAvailable\n");
#endif

    char* buffer = new char[length];
    if (buffer) {
        PRUint32 amountRead = 0;
        nsresult rslt = input->Read(buffer, length, &amountRead);
        if (rslt == NS_OK) {
            char msg[256];
            sprintf(msg, "### Received %d bytes for %s\n", length, fMessageName);
        }
        delete buffer;
    }
    return NS_OK;
}

NS_METHOD
SimplePluginStreamListener::OnFileAvailable(nsIPluginStreamInfo* pluginInfo, 
                                            const char* fileName)
{
#ifdef NS_DEBUG
    printf("SimplePluginStreamListener::OnFileAvailable\n");
#endif

    char msg[256];
    sprintf(msg, "### File available for %s: %s\n", fMessageName, fileName);
    return NS_OK;
}

NS_METHOD
SimplePluginStreamListener::OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status )
{
#ifdef NS_DEBUG
    printf("SimplePluginStreamListener::OnStopBinding\n");
#endif

    char msg[256];
    sprintf(msg, "### Closing plugin stream for %s\n", fMessageName);
    return NS_OK;
}

NS_METHOD
SimplePluginStreamListener::OnNotify(const char* url, nsresult status)
{
#ifdef NS_DEBUG
    printf("SimplePluginStreamListener::OnNotify\n");
#endif

    return NS_OK;
}

NS_METHOD
SimplePluginStreamListener::GetStreamType(nsPluginStreamType *result)
{
#ifdef NS_DEBUG
    printf("SimplePluginStreamListener::GetStreamType\n");
#endif

    *result = nsPluginStreamType_Normal;
    return NS_OK;
}











#if defined(XP_UNIX)







void
SimplePluginInstance::PlatformNew(void)
{
    fPlatform.window = 0;
}







nsresult
SimplePluginInstance::PlatformDestroy(void)
{
    gtk_widget_destroy(fPlatform.moz_box);
    fPlatform.moz_box = 0;
    return NS_OK;
}







nsresult
SimplePluginInstance::PlatformSetWindow(nsPluginWindow* window)
{
#ifdef NS_DEBUG
    printf("SimplePluginInstance::PlatformSetWindow\n");
#endif

    if (window == NULL || window->window == NULL)
        return NS_ERROR_NULL_POINTER;

    GdkWindow *win = gdk_window_lookup((XID)window->window);

    if ( fPlatform.superwin && fPlatform.superwin->bin_window == win )
        return NS_OK;
    
    
    
    fPlatform.superwin = gdk_superwin_new(win, 0, 0, window->width, window->height);

    
    if (fPlatform.label)
        gtk_widget_destroy(fPlatform.label);
    if (fPlatform.moz_box)
        gtk_widget_destroy(fPlatform.moz_box);

    
    fPlatform.moz_box = gtk_mozbox_new(fPlatform.superwin->bin_window);
    fPlatform.label = gtk_label_new(fText);
    gtk_container_add(GTK_CONTAINER(fPlatform.moz_box), fPlatform.label);

    
    gtk_widget_set_usize(fPlatform.label, window->width, window->height);

    
    gtk_signal_connect (GTK_OBJECT(fPlatform.label), "expose_event",
                        GTK_SIGNAL_FUNC(draw), this);

    gtk_widget_show(fPlatform.label);
    gtk_widget_show(fPlatform.moz_box);

    return NS_OK;
}







int16
SimplePluginInstance::PlatformHandleEvent(nsPluginEvent* event)
{
    
    return 0;
}

NS_IMETHODIMP
SimplePluginInstance::Repaint(void)
{
#ifdef DEBUG
    printf("SimplePluginInstance::Repaint()\n");
#endif
    
    if ( !fPlatform.moz_box || !fPlatform.label )
        return NS_ERROR_FAILURE;

    
    gtk_label_set_text(GTK_LABEL(fPlatform.label), fText);

    
    gtk_widget_show(fPlatform.label);
    gtk_widget_show(fPlatform.moz_box);

    return NS_OK;
}

gboolean draw (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    SimplePluginInstance * pthis = (SimplePluginInstance *)data;

    pthis->Repaint();
    return TRUE;
}





#elif defined(XP_WIN)
const char* gInstanceLookupString = "instance->pdata";







void
SimplePluginInstance::PlatformNew(void)
{
    fPlatform.fhWnd = NULL;
    fPlatform.fDefaultWindowProc = NULL;
}







nsresult
SimplePluginInstance::PlatformDestroy(void)
{
    if( fWindow != NULL ) { 

        SetWindowLong( fPlatform.fhWnd, GWL_WNDPROC, (LONG)fPlatform.fDefaultWindowProc);
        fPlatform.fDefaultWindowProc = NULL;
        fPlatform.fhWnd = NULL;
    }

    return NS_OK;
}







nsresult
SimplePluginInstance::PlatformSetWindow(nsPluginWindow* window)
{
    if( fWindow != NULL ) 


    {
        if( (window == NULL) || ( window->window == NULL ) ) {
            

            SetWindowLong( fPlatform.fhWnd, GWL_WNDPROC, (LONG)fPlatform.fDefaultWindowProc);
            fPlatform.fDefaultWindowProc = NULL;
            fPlatform.fhWnd = NULL;
            return NS_OK;
        }

        else if ( fPlatform.fhWnd == (HWND) window->window ) {
            
            return NS_OK;
        }
        else {
            

            SetWindowLong( fPlatform.fhWnd, GWL_WNDPROC, (LONG)fPlatform.fDefaultWindowProc);
            fPlatform.fDefaultWindowProc = NULL;
            fPlatform.fhWnd = NULL;
        }
    }
    else if( (window == NULL) || ( window->window == NULL ) ) {
        

        return NS_OK;
    }

    


    fPlatform.fDefaultWindowProc =
        (WNDPROC)SetWindowLong( (HWND)window->window,
                                GWL_WNDPROC, (LONG)SimplePluginInstance::PluginWindowProc);
    fPlatform.fhWnd = (HWND) window->window;
    SetProp(fPlatform.fhWnd, gInstanceLookupString, (HANDLE)this);

    InvalidateRect( fPlatform.fhWnd, NULL, TRUE );
    UpdateWindow( fPlatform.fhWnd );
    return NS_OK;
}







PRInt16
SimplePluginInstance::PlatformHandleEvent(nsPluginEvent* event)
{
    

    return 0;
}







LRESULT CALLBACK 
SimplePluginInstance::PluginWindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    SimplePluginInstance* inst = (SimplePluginInstance*) GetProp(hWnd, gInstanceLookupString);

    switch( Msg ) {
      case WM_PAINT: {
          PAINTSTRUCT paintStruct;
          HDC hdc;

          hdc = BeginPaint( hWnd, &paintStruct );

          if(paintStruct.fErase)
            FillRect(hdc, &paintStruct.rcPaint, 
                     (HBRUSH) GetStockObject(WHITE_BRUSH)); 

          if(inst->fText) {
            RECT rcWnd;
            GetWindowRect(hWnd, &rcWnd);
            SetTextAlign(hdc, TA_CENTER);
            TextOut(hdc, (rcWnd.right-rcWnd.left)/2, (rcWnd.bottom-rcWnd.top)/2, inst->fText, strlen(inst->fText));
          }

          EndPaint( hWnd, &paintStruct );
          break;
      }
      default: {
          CallWindowProc(inst->fPlatform.fDefaultWindowProc, hWnd, Msg, wParam, lParam);
      }
    }
    return 0;
}







#elif defined(XP_MAC)

PRBool	StartDraw(nsPluginWindow* window);
void 	EndDraw(nsPluginWindow* window);
void 	DoDraw(SimplePluginInstance* This);

CGrafPort 		gSavePort;
CGrafPtr		gOldPort;







void
SimplePluginInstance::PlatformNew(void)
{
}







nsresult
SimplePluginInstance::PlatformDestroy(void)
{
    return NS_OK;
}







nsresult
SimplePluginInstance::PlatformSetWindow(nsPluginWindow* window)
{
    fWindow = window;
    if( StartDraw( window ) ) {
        DoDraw(This);
        EndDraw( window );
    }
    return NS_OK;
}







int16
SimplePluginInstance::PlatformHandleEvent(nsPluginEvent* event)
{
    int16 eventHandled = FALSE;
	
    EventRecord* ev = (EventRecord*) event;
    if (This != NULL && event != NULL)
    {
        switch (ev->what)
        {
            


          case updateEvt:
            if( StartDraw( fWindow ) ) {
                DoDraw(This);
                EndDraw( fWindow );
            }
            eventHandled = true;
            break;
          default:
            break;
        }
    }
    return eventHandled;
}





PRBool
SimplePluginInstance::StartDraw(nsPluginWindow* window)
{
    NP_Port* port;
    Rect clipRect;
    RGBColor  col;
	
    if (window == NULL)
        return FALSE;
    port = (NP_Port*) window->window;
    if (window->clipRect.left < window->clipRect.right)
    {
	
        GetPort((GrafPtr*)&gOldPort);
        SetPort((GrafPtr)port->port);
	
        gSavePort.portRect = port->port->portRect;
        gSavePort.txFont = port->port->txFont;
        gSavePort.txFace = port->port->txFace;
        gSavePort.txMode = port->port->txMode;
        gSavePort.rgbFgColor = port->port->rgbFgColor;
        gSavePort.rgbBkColor = port->port->rgbBkColor;
        GetClip(gSavePort.clipRgn);
	
        clipRect.top = window->clipRect.top + port->porty;
        clipRect.left = window->clipRect.left + port->portx;
        clipRect.bottom = window->clipRect.bottom + port->porty;
        clipRect.right = window->clipRect.right + port->portx;
        SetOrigin(port->portx,port->porty);
        ClipRect(&clipRect);
        clipRect.top = clipRect.left = 0;
        TextSize(12);
        TextFont(geneva);
        TextMode(srcCopy);
        col.red = col.green = col.blue = 0;
        RGBForeColor(&col);
        col.red = col.green = col.blue = 65000;
        RGBBackColor(&col);
        return TRUE;
    }
    else
        return FALSE;
}





void
SimplePluginInstance::EndDraw(nsPluginWindow* window)
{
    CGrafPtr myPort;
    NP_Port* port = (NP_Port*) window->window;
    SetOrigin(gSavePort.portRect.left, gSavePort.portRect.top);
    SetClip(gSavePort.clipRgn);
    GetPort((GrafPtr*)&myPort);
    myPort->txFont = gSavePort.txFont;
    myPort->txFace = gSavePort.txFace;
    myPort->txMode = gSavePort.txMode;
    RGBForeColor(&gSavePort.rgbFgColor);
    RGBBackColor(&gSavePort.rgbBkColor);
    SetPort((GrafPtr)gOldPort);
}





void
SimplePluginInstance::DoDraw(void)
{
    Rect drawRect;
    drawRect.top = 0;
    drawRect.left = 0;
    drawRect.bottom = drawRect.top + fWindow->height;
    drawRect.right = drawRect.left + fWindow->width;
    EraseRect( &drawRect );
    MoveTo( 2, 12 );
    DrawString("\pHello, World!");
}







#else

void
SimplePluginInstance::PlatformNew(void)
{
}

nsresult
SimplePluginInstance::PlatformDestroy(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
SimplePluginInstance::PlatformSetWindow(nsPluginWindow* window)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

int16
SimplePluginInstance::PlatformHandleEvent(nsPluginEvent* event)
{
    int16 eventHandled = FALSE;
    return eventHandled;
}

#endif 


