











































#include "nsplugin.h"
#include "nsIServiceManager.h"
#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsIAllocator.h"

#include "nsIEventsPluginInstance.h"

#include <stdio.h> 
#include <string.h> 

#if defined (XP_WIN)

#include <windows.h>

#elif defined (XP_UNIX)

#include <gdk/gdkprivate.h> 
#include <gtk/gtk.h> 
#include <gdk/gdkkeysyms.h> 
#include <gtkmozbox.h> 

#endif 

#define EVENTSPLUGIN_DEBUG




#define NS_EVENTSAMPLEPLUGIN_CID { 0xcb2ef72a, 0x856a, 0x4818, { 0x8e, 0x72, 0x34, 0x39, 0x39, 0x5e, 0x33, 0x5f } }

#if defined(XP_UNIX)
typedef struct _PlatformInstance {
	GtkWidget *moz_box;
	Display * display;
	uint32 x, y;
	uint32 width, height;
}
PlatformInstance;

typedef GtkWidget* WinID;

#endif 

#if defined(XP_WIN)
typedef struct _PlatformInstance
{
	WNDPROC fOldChildWindowProc; 
	WNDPROC fParentWindowProc; 
} PlatformInstance;


static const char* gInstanceLookupString = "instance->pdata";

typedef HWND WinID;

#endif 

static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);
static NS_DEFINE_CID(kEventsPluginCID, NS_EVENTSAMPLEPLUGIN_CID);

const char *kPluginName = "Events Sample Plug-in";
const char *kPluginDescription = "Sample plugin that demonstrates events, focus and keystrokes.";
#define PLUGIN_MIME_TYPE "application/x-events-sample-plugin"

static const char* kMimeTypes[] = {
    PLUGIN_MIME_TYPE
};

static const char* kMimeDescriptions[] = {
    "Event Sample Plug-in"
};

static const char* kFileExtensions[] = {
    "smpev"
};

static const PRInt32 kNumMimeTypes = sizeof(kMimeTypes) / sizeof(*kMimeTypes);




class EventsPluginInstance :
	public nsIPluginInstance,
	public nsIEventsSampleInstance {
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
	NS_DECL_NSIEVENTSSAMPLEINSTANCE

	
	

	NS_IMETHOD HandleEvent(nsPluginEvent* event, PRBool* handled);

	
	

	NS_IMETHOD Initialize(nsIPluginInstancePeer *peer);

	
	NS_IMETHOD GetPeer(nsIPluginInstancePeer **result);

	NS_IMETHOD Start(void);

	NS_IMETHOD Stop(void);

	NS_IMETHOD Destroy(void);

	NS_IMETHOD SetWindow(nsPluginWindow* window);

	NS_IMETHOD NewStream(nsIPluginStreamListener** listener);

	NS_IMETHOD Print(nsPluginPrint* platformPrint);

	NS_IMETHOD GetValue(nsPluginInstanceVariable variable, void *value);

	
	

	EventsPluginInstance();
	virtual ~EventsPluginInstance();

	void PlatformNew(void);
	nsresult PlatformDestroy(void);
	void PlatformResetWindow();
	PRInt16 PlatformHandleEvent(nsPluginEvent* event);
	void PlatformResizeWindow(nsPluginWindow* window);
	nsresult PlatformCreateWindow(nsPluginWindow* window);

	void SetMode(nsPluginMode mode) { fMode = mode; }

protected:
	
	
	nsresult DoSetWindow(nsPluginWindow* window);

	
	nsCOMPtr<nsIPluginInstancePeer> fPeer;
	nsPluginWindow *fWindow; 
	nsPluginMode fMode;
	PlatformInstance fPlatform;

	WinID wMain; 
	WinID wChild; 

	
	
#ifdef XP_WIN
	static LRESULT CALLBACK WndProcChild(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
};





class EventsPluginStreamListener : public nsIPluginStreamListener {
public:

	NS_DECL_ISUPPORTS

	
	

	
	
	NS_IMETHOD OnStartBinding(nsIPluginStreamInfo* pluginInfo);

	
	
	NS_IMETHOD OnDataAvailable(nsIPluginStreamInfo* pluginInfo,
	                           nsIInputStream* input,
	                           PRUint32 length);

	NS_IMETHOD OnFileAvailable(nsIPluginStreamInfo* pluginInfo, const char* fileName);

	
	
	
	NS_IMETHOD OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status);

	NS_IMETHOD OnNotify(const char* url, nsresult status);

	NS_IMETHOD GetStreamType(nsPluginStreamType *result);

	
	

	EventsPluginStreamListener(EventsPluginInstance *inst_, const char* url);
	virtual ~EventsPluginStreamListener(void);

protected:
	const char* fMessageName;
	EventsPluginInstance *inst;
};


static const nsModuleComponentInfo gComponentInfo[] = {
    { "Events Sample Plugin",
      NS_EVENTSAMPLEPLUGIN_CID,
      NS_INLINE_PLUGIN_CONTRACTID_PREFIX PLUGIN_MIME_TYPE,
      EventsPluginInstance::Create,
      EventsPluginInstance::RegisterSelf,
      EventsPluginInstance::UnregisterSelf },
};

NS_IMPL_NSGETMODULE(EventsPlugin, gComponentInfo)






NS_METHOD
EventsPluginInstance::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    EventsPluginInstance* plugin = new EventsPluginInstance();
    if (! plugin)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    NS_ADDREF(plugin);
    rv = plugin->QueryInterface(aIID, aResult);
    NS_RELEASE(plugin);
    return rv;
}

NS_METHOD
EventsPluginInstance::RegisterSelf(nsIComponentManager* aCompMgr,
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
        rv = pm->RegisterPlugin(kEventsPluginCID,
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
EventsPluginInstance::UnregisterSelf(nsIComponentManager* aCompMgr,
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
        rv = pm->UnregisterPlugin(kEventsPluginCID);
        NS_RELEASE(pm);
    }

    return rv;
}








NS_IMPL_ISUPPORTS2(EventsPluginInstance, nsIPluginInstance, nsIEventsSampleInstance)

EventsPluginInstance::EventsPluginInstance() :
		fPeer(NULL), fWindow(NULL), fMode(nsPluginMode_Embedded)
{
	wChild = 0;
}

EventsPluginInstance::~EventsPluginInstance(void) {
}


NS_METHOD EventsPluginInstance::Initialize(nsIPluginInstancePeer *peer) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::Initialize\n");
#endif 

	NS_ASSERTION(peer != NULL, "null peer");

	fPeer = peer;
	nsCOMPtr<nsIPluginTagInfo> taginfo;
	const char* const* names = nsnull;
	const char* const* values = nsnull;
	PRUint16 count = 0;
	nsresult result;

	peer->AddRef();
	result = peer->GetMode(&fMode);
	if (NS_FAILED(result)) return result;

	taginfo = do_QueryInterface(peer, &result);
	if (NS_SUCCEEDED(result)) {
		taginfo->GetAttributes(count, names, values);
	}

	PlatformNew(); 	
	return NS_OK;
}

NS_METHOD EventsPluginInstance::GetPeer(nsIPluginInstancePeer* *result) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::GetPeer\n");
#endif 

	*result = fPeer;
	NS_IF_ADDREF(*result);
	return NS_OK;
}

NS_METHOD EventsPluginInstance::Start(void) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::Start\n");
#endif 

	return NS_OK;
}

NS_METHOD EventsPluginInstance::Stop(void) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::Stop\n");
#endif 

	return NS_OK;
}

NS_METHOD EventsPluginInstance::Destroy(void) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::Destroy\n");
#endif 

	PlatformDestroy(); 
	return NS_OK;
}














NS_METHOD EventsPluginInstance::SetWindow(nsPluginWindow* window) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::SetWindow\n");
#endif 

	nsresult result;
	result = DoSetWindow(window);
	fWindow = window;
	return result;
}

nsresult EventsPluginInstance::DoSetWindow(nsPluginWindow* window) {
	






	 nsresult result = NS_OK;
	if ( fWindow != NULL ) {
		
		
		if ( window && window->window && wMain == (WinID)window->window ) {
			
			PlatformResizeWindow(window);
			return NS_OK;
		}
		
		PlatformResetWindow();
	}
	else if ( (window == NULL) || ( window->window == NULL ) ) {
		
 
		return NS_OK;
	}
	if (window && window->window) {
		
		wMain = (WinID)window->window;
		
		result = PlatformCreateWindow(window);
	}
	return result;
}

NS_METHOD EventsPluginInstance::NewStream(nsIPluginStreamListener** listener) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::NewStream\n");
#endif 

	if (listener != NULL) {
		EventsPluginStreamListener* sl =
		    new EventsPluginStreamListener(this, "http://www.mozilla.org");
		if (!sl)
			return NS_ERROR_UNEXPECTED;
		sl->AddRef();
		*listener = sl;
	}

	return NS_OK;
}

NS_METHOD EventsPluginInstance::Print(nsPluginPrint* printInfo) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::Print\n");
#endif 

	if (printInfo == NULL)
		return NS_ERROR_FAILURE;

	if (printInfo->mode == nsPluginMode_Full) {
		
















		
		printInfo->print.fullPrint.pluginPrinted = PR_FALSE;
	} else {	
		










	}
	return NS_OK;
}











NS_METHOD EventsPluginInstance::HandleEvent(nsPluginEvent* event, PRBool* handled) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::HandleEvent\n");
#endif 

	*handled = (PRBool)PlatformHandleEvent(event);
	return NS_OK;
}

NS_METHOD EventsPluginInstance::GetValue(nsPluginInstanceVariable , void * ) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::GetValue\n");
#endif 

	return NS_ERROR_FAILURE;
}





EventsPluginStreamListener::EventsPluginStreamListener(EventsPluginInstance* inst_,
        const char* msgName)
		: fMessageName(msgName), inst(inst_) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener: EventsPluginStreamListener for %s\n", fMessageName);
#endif
}

EventsPluginStreamListener::~EventsPluginStreamListener(void) {
}




NS_IMPL_ISUPPORTS1(EventsPluginStreamListener, nsIPluginStreamListener)

NS_METHOD EventsPluginStreamListener::OnStartBinding(nsIPluginStreamInfo * ) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener::OnStartBinding\n");
	printf("EventsPluginStreamListener: Opening plugin stream for %s\n", fMessageName);
#endif 
	return NS_OK;
}

NS_METHOD EventsPluginStreamListener::OnDataAvailable(
    nsIPluginStreamInfo * ,
    nsIInputStream* input,
    PRUint32 length) {

#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener::OnDataAvailable\n");
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

NS_METHOD EventsPluginStreamListener::OnFileAvailable(
    nsIPluginStreamInfo * ,
    const char* fileName) {

#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener::OnFileAvailable\n");
#endif 

	char msg[256];
	sprintf(msg, "### File available for %s: %s\n", fMessageName, fileName);
	return NS_OK;
}

NS_METHOD EventsPluginStreamListener::OnStopBinding(
    nsIPluginStreamInfo * ,
    nsresult ) {

#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener::OnStopBinding\n");
#endif 

	char msg[256];
	sprintf(msg, "### Closing plugin stream for %s\n", fMessageName);
	return NS_OK;
}

NS_METHOD EventsPluginStreamListener::OnNotify(const char * , nsresult ) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener::OnNotify\n");
#endif 

	return NS_OK;
}

NS_METHOD EventsPluginStreamListener::GetStreamType(nsPluginStreamType *result) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginStreamListener::GetStreamType\n");
#endif 

	*result = nsPluginStreamType_Normal;
	return NS_OK;
}






#ifdef XP_UNIX

void EventsPluginInstance::PlatformNew(void) {
	fPlatform.moz_box = 0;
}

nsresult EventsPluginInstance::PlatformDestroy(void) {
	
	
	return NS_OK;
}

void EventsPluginInstance::PlatformResetWindow() {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::PlatformResetWindow\n");
#endif
	fPlatform.moz_box = 0;
}

nsresult EventsPluginInstance::PlatformCreateWindow(nsPluginWindow* window) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::PlatformCreateWindow %lx\n", (long)window);
#endif 

	Window x_window = (Window)window->window;
	GdkWindow *gdk_window = (GdkWindow *)gdk_xid_table_lookup(x_window);
	if (!gdk_window) {
		fprintf(stderr, "NO WINDOW!!!\n");
		return NS_ERROR_FAILURE;
	}
	fPlatform.moz_box = gtk_mozbox_new(gdk_window);

	wChild = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(fPlatform.moz_box), wChild);
	gtk_widget_show_all(fPlatform.moz_box);
	return NS_OK;
}

void EventsPluginInstance::PlatformResizeWindow(nsPluginWindow* window) {
	NS_PRECONDITION(wChild, "Have no wChild!");
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::PlatformResizeWindow to size (%d,%d)\n", window->width, window->height);
#endif
	
	gtk_widget_set_usize(wChild, window->width, window->height);
}

int16 EventsPluginInstance::PlatformHandleEvent(nsPluginEvent * ) {
	
	return 0;
}


NS_IMETHODIMP EventsPluginInstance::GetVal(char * *aText) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::GetVal\n");
#endif 
	char *text = gtk_entry_get_text(GTK_ENTRY(wChild));
	*aText = reinterpret_cast<char*>(nsAllocator::Clone(text, strlen(text) + 1));
	return (*aText) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP EventsPluginInstance::SetVal(const char * aText) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::SetVal\n");
#endif 
	gtk_entry_set_text(GTK_ENTRY(wChild), aText);
	return NS_OK;
}
#endif     



#if defined(XP_WIN)

void EventsPluginInstance::PlatformNew(void) {
	
}

nsresult EventsPluginInstance::PlatformDestroy(void) {
	wChild = 0;
	return NS_OK;
}

nsresult EventsPluginInstance::PlatformCreateWindow(nsPluginWindow* window) {
	
	fPlatform.fParentWindowProc = (WNDPROC)::GetWindowLong(wMain, GWL_WNDPROC);
	NS_ABORT_IF_FALSE(fPlatform.fParentWindowProc!=NULL, "Couldn't get the parent WNDPROC");

	
	RECT rc;
	::GetWindowRect(wMain, &rc);

	wChild = ::CreateWindow("Edit", 
							"", 
							WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL, 
							0, 0, rc.right-rc.left, rc.bottom-rc.top,
							wMain, 
							(HMENU)1111, 
							0, 
							NULL); 
	NS_ABORT_IF_FALSE(wChild != NULL, "Failed to create the child window!");
	if (!wChild)
		return NS_ERROR_FAILURE;
	
	::SetProp(wChild, gInstanceLookupString, (HANDLE)this);
	fPlatform.fOldChildWindowProc =
		(WNDPROC)::SetWindowLong( wChild,
								GWL_WNDPROC, 
								(LONG)EventsPluginInstance::WndProcChild);
	return NS_OK;
}

int16 EventsPluginInstance::PlatformHandleEvent(nsPluginEvent * ) {
	return NS_OK;
}

void EventsPluginInstance::PlatformResetWindow() {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::PlatformResetWindow\n");
#endif
	fPlatform.fParentWindowProc = NULL;
	::SetWindowLong(wChild, GWL_WNDPROC, (LONG)fPlatform.fOldChildWindowProc);
	fPlatform.fOldChildWindowProc = NULL;
	wChild = NULL;
	wMain = NULL;
}

void EventsPluginInstance::PlatformResizeWindow(nsPluginWindow* window) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::PlatformResizeWindow with new size (%d,%d)\n", window->width, window->height);
#endif
	RECT rc;
	NS_PRECONDITION(wMain != nsnull, "Must have a valid wMain to resize");
	::GetClientRect(wMain, &rc);
	::SetWindowPos(wChild, 0, rc.left, rc.top,
	               rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
}


NS_IMETHODIMP EventsPluginInstance::GetVal(char * *aText) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::GetVal\n");
#endif 
	static char *empty = "";
	char *value = empty;
	char buffer[256];
	if (wChild) {
		GetWindowText(wChild, buffer, sizeof(buffer)/sizeof(buffer[0]));
		value = buffer;
	}
	*aText = reinterpret_cast<char*>(nsAllocator::Clone(value, strlen(value) + 1));
	return (*aText) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP EventsPluginInstance::SetVal(const char * aText) {
#ifdef EVENTSPLUGIN_DEBUG
	printf("EventsPluginInstance::SetVal\n");
#endif 
	NS_ABORT_IF_FALSE(wChild != 0, "Don't have a window!");
	SetWindowText(wChild, aText);
	return NS_OK;
}


LRESULT CALLBACK EventsPluginInstance::WndProcChild(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	EventsPluginInstance* inst = (EventsPluginInstance*) GetProp(hWnd, gInstanceLookupString);
	NS_ABORT_IF_FALSE(inst, "Could not get the inst from the Window!!");
	switch (Msg) {
	
	
	
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			
			return ::CallWindowProc(inst->fPlatform.fParentWindowProc, (HWND)inst->wMain, Msg, wParam, lParam);
		default:
			
			return ::CallWindowProc(inst->fPlatform.fOldChildWindowProc, hWnd, Msg, wParam, lParam);
	}
	
	NS_ABORT_IF_FALSE(0, "not reached!");
}


#endif     
