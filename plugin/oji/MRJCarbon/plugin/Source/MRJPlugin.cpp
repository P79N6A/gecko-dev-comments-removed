












































#include "MRJPlugin.h"
#include "MRJSession.h"
#include "MRJContext.h"
#include "MRJConsole.h"
#include "EmbeddedFramePluginInstance.h"

#if !TARGET_CARBON
#include "MRJFrame.h"
#endif

#include "nsIServiceManager.h"
#include "nsIServiceManagerObsolete.h"
#include "nsObsoleteModuleLoading.h"

#include "nsIMemory.h"
#include "nsIJVMManager.h"
#include "nsIJVMPluginTagInfo.h"
#include "nsIPluginManager2.h"
#include "nsIPluginInstancePeer.h"
#include "nsIWindowlessPlugInstPeer.h"
#include "LiveConnectNativeMethods.h"
#include "CSecureEnv.h"

#include <Resources.h>
#include <LaunchServices.h>
#include <string>
#include <CFBundle.h>

nsIPluginManager* thePluginManager = NULL;
nsIPluginManager2* thePluginManager2 = NULL;
nsIMemory* theMemoryAllocator = NULL;

FSSpec thePluginSpec;
short thePluginRefnum = -1;



static NS_DEFINE_IID(kPluginCID, NS_PLUGIN_CID);
static NS_DEFINE_IID(kPluginManagerCID, NS_PLUGINMANAGER_CID);
static NS_DEFINE_IID(kJVMManagerCID, NS_JVMMANAGER_CID);

static NS_DEFINE_IID(kIWindowlessPluginInstancePeerIID, NS_IWINDOWLESSPLUGININSTANCEPEER_IID);





static nsIServiceManager* theServiceManager = NULL;
static nsIServiceManagerObsolete* theServiceManagerObsolete = NULL;

nsresult MRJPlugin::GetService(const nsCID& aCID, const nsIID& aIID, void* *aService)
{
    if (theServiceManager)
        return theServiceManager->GetService(aCID, aIID, aService);
    if (theServiceManagerObsolete)
        return theServiceManagerObsolete->GetService(aCID, aIID, (nsISupports **)aService);
    return NS_ERROR_FAILURE;
}

nsresult MRJPlugin::GetService(const char* aContractID, const nsIID& aIID, void* *aService)
{
    if (theServiceManager)
        return theServiceManager->GetServiceByContractID(aContractID, aIID, aService);
    if (theServiceManagerObsolete)
        return theServiceManagerObsolete->GetService(aContractID, aIID, (nsISupports **)aService);
    return NS_ERROR_FAILURE;
}

#pragma export on

static long getSystemVersion()
{
    long version = 0;
    Gestalt(gestaltSystemVersion, &version);
    return version;
}

extern "C"
nsresult NSGetFactory(nsISupports* serviceManager, const nsCID &aClass, const char *aClassName, const char *aContractID, nsIFactory **aFactory)
{
    nsresult result = NS_OK;

    
    if (getSystemVersion() < 0x00001010)
        return NS_ERROR_FAILURE;

    if (theServiceManager == NULL && theServiceManagerObsolete == NULL) {
        if (NS_FAILED(serviceManager->QueryInterface(NS_GET_IID(nsIServiceManager), (void**)&theServiceManager)))
            if (NS_FAILED(serviceManager->QueryInterface(NS_GET_IID(nsIServiceManagerObsolete), (void**)&theServiceManagerObsolete)))
                return NS_ERROR_FAILURE;

        
        
        if (NS_FAILED(MRJPlugin::GetService("@mozilla.org/xpcom/memory-service;1", NS_GET_IID(nsIMemory), (void **)&theMemoryAllocator)))
            return NS_ERROR_FAILURE;
    }

    if (aClass.Equals(kPluginCID)) {
        MRJPlugin* pluginFactory = new MRJPlugin();
        pluginFactory->AddRef();
        *aFactory = pluginFactory;
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

#pragma export off

#if TARGET_RT_MAC_CFM

extern "C" {

pascal OSErr __initialize(const CFragInitBlock *initBlock);
pascal void __terminate(void);

#if defined(MRJPLUGIN_GC)
pascal OSErr __NSInitialize(const CFragInitBlock* initBlock);
pascal void __NSTerminate(void);
#define __initialize __NSInitialize
#define __terminate __NSTerminate
#endif

pascal OSErr MRJPlugin__initialize(const CFragInitBlock *initBlock);
pascal void MRJPlugin__terminate(void);

}

pascal OSErr MRJPlugin__initialize(const CFragInitBlock *initBlock)
{
    OSErr err = __initialize(initBlock);
    if (err != noErr) return err;

    if (initBlock->fragLocator.where == kDataForkCFragLocator) {
        thePluginSpec = *initBlock->fragLocator.u.onDisk.fileSpec;
    
        
        thePluginRefnum = ::FSpOpenResFile(&thePluginSpec, fsRdPerm);
    }
    
    return noErr;
}

pascal void MRJPlugin__terminate()
{
#if !TARGET_CARBON
    
    if (theMemoryAllocator != NULL) {
        theMemoryAllocator->Release();
        theMemoryAllocator = NULL;
    }
#endif

    
    
    if (thePluginRefnum != -1) {
        ::CloseResFile(thePluginRefnum);
        thePluginRefnum = -1;
    }

    __terminate();
}

#endif 

#if TARGET_RT_MAC_MACHO

extern "C" {
CF_EXPORT Boolean CFURLGetFSRef(CFURLRef url, FSRef *fsRef);
}

static CFBundleRef getPluginBundle()
{
#ifdef DEBUG
    printf("### MRJPlugin:  getPluginBundle() here. ###\n");
#endif
    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.netscape.MRJPlugin"));
    if (bundle) {
#ifdef DEBUG
        printf("### MRJPlugin:  CFBundleGetBundleWithIdentifier() succeeded. ###\n");
#endif
        
        CFURLRef url = CFBundleCopyExecutableURL(bundle);
        if (url) {
            FSRef ref;
            if (CFURLGetFSRef(url, &ref)) {
#ifdef DEBUG
                printf("### MRJPlugin:  CFURLGetFSRef() succeeded. ###\n");
#endif
                FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &thePluginSpec, NULL);

                
                thePluginRefnum = ::CFBundleOpenBundleResourceMap(bundle);
            }
            CFRelease(url);
        }
    }
    return bundle;
}

CFBundleRef thePluginBundle = getPluginBundle();

#endif 





#pragma mark *** MRJPlugin ***

const InterfaceInfo MRJPlugin::sInterfaces[] = {
    { NS_IPLUGIN_IID, INTERFACE_OFFSET(MRJPlugin, nsIPlugin) },
    { NS_IJVMPLUGIN_IID, INTERFACE_OFFSET(MRJPlugin, nsIJVMPlugin) },
#if USE_SYSTEM_CONSOLE
    { NS_IJVMCONSOLE_IID, INTERFACE_OFFSET(MRJPlugin, nsIJVMConsole) },
#endif
    { NS_IRUNNABLE_IID, INTERFACE_OFFSET(MRJPlugin, nsIRunnable) },
};
const UInt32 MRJPlugin::kInterfaceCount = sizeof(sInterfaces) / sizeof(InterfaceInfo);

MRJPlugin::MRJPlugin()
    :   SupportsMixin(this, sInterfaces, kInterfaceCount),
        mManager(NULL), mThreadManager(NULL), mSession(NULL), mConsole(NULL), mPluginThreadID(NULL), mIsEnabled(false)
{
}

MRJPlugin::~MRJPlugin()
{
    
    NS_IF_RELEASE(mConsole);

    
    if (mSession != NULL) {
        delete mSession;
        mSession = NULL;
    }

    
    NS_IF_RELEASE(mManager);
    NS_IF_RELEASE(mThreadManager);
}

#if !USE_SYSTEM_CONSOLE







NS_METHOD MRJPlugin::QueryInterface(const nsIID& aIID, void** instancePtr)
{
    nsresult result = queryInterface(aIID, instancePtr);
    if (result == NS_NOINTERFACE) {
        result = mConsole->queryInterface(aIID, instancePtr);
    }
    return result;
}
#endif

NS_METHOD MRJPlugin::CreateInstance(nsISupports *aOuter, const nsIID& aIID, void **aResult)
{
    nsresult result = StartupJVM();
    if (result == NS_OK) {
        MRJPluginInstance* instance = new MRJPluginInstance(this);
        if (instance == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        result = instance->QueryInterface(aIID, aResult);
        if (result != NS_OK)
            delete instance;
    }
    return result;
}

#define NS_APPLET_MIME_TYPE "application/x-java-applet"

NS_METHOD MRJPlugin::CreatePluginInstance(nsISupports *aOuter, REFNSIID aIID, const char* aPluginMIMEType, void **aResult)
{
    nsresult result = NS_NOINTERFACE;

    if (::strcmp(aPluginMIMEType, "application/x-java-frame") == 0) {
#if !TARGET_CARBON
        
        EmbeddedFramePluginInstance* instance = new EmbeddedFramePluginInstance();
        nsresult result = instance->QueryInterface(aIID, aResult);
        if (result != NS_OK)
            delete instance;
#endif
    } else {
        
        result = CreateInstance(aOuter, aIID, aResult);
    }
    return result;
}

NS_METHOD MRJPlugin::Initialize()
{
    nsresult result = NS_OK;

    
    if (thePluginManager == NULL) {
        result = MRJPlugin::GetService(kPluginManagerCID, NS_GET_IID(nsIPluginManager), (void**)&thePluginManager);
        if (result != NS_OK || thePluginManager == NULL)
            return NS_ERROR_FAILURE;
    }

    
    if (thePluginManager2 == NULL) {
        if (thePluginManager->QueryInterface(NS_GET_IID(nsIPluginManager2), (void**)&thePluginManager2) != NS_OK)
            thePluginManager2 = NULL;
    }

    
    if (MRJPlugin::GetService(kJVMManagerCID, NS_GET_IID(nsIJVMManager), (void**)&mManager) != NS_OK)
        mManager = NULL;
    
    
    if (mManager != NULL) {
        if (mManager->QueryInterface(NS_GET_IID(nsIThreadManager), (void**)&mThreadManager) != NS_OK)
            mThreadManager = NULL;

        if (mThreadManager != NULL)
            mThreadManager->GetCurrentThread(&mPluginThreadID);
    }

#if !USE_SYSTEM_CONSOLE
    
    if (thePluginManager2 != NULL) {
        mConsole = new MRJConsole(this);
        mConsole->AddRef();
    }
#endif

    return result;
}

NS_METHOD MRJPlugin::Shutdown()
{
    
    ShutdownLiveConnectSupport();

    
    NS_IF_RELEASE(thePluginManager2);
    NS_IF_RELEASE(thePluginManager);

    
    NS_IF_RELEASE(theServiceManager);
    NS_IF_RELEASE(theServiceManagerObsolete);
    
    return NS_OK;
}

NS_METHOD MRJPlugin::GetMIMEDescription(const char* *result)
{
    *result = NS_JVM_MIME_TYPE;
    return NS_OK;
}

NS_METHOD MRJPlugin::GetValue(nsPluginVariable variable, void *value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

MRJSession* MRJPlugin::getSession()
{
    StartupJVM();
    return mSession;
}

nsIJVMManager* MRJPlugin::getManager()
{
    return mManager;
}

nsIThreadManager* MRJPlugin::getThreadManager()
{
    return mThreadManager;
}

static char* getJavaConsolePath(char* path, UInt32 maxPathSize)
{
    path[0] = '\0';
    FSRef ref;
    OSErr err = FSFindFolder(kUserDomain, kDomainLibraryFolderType, true, &ref);
    if (err == noErr) {
        err = FSRefMakePath(&ref, (UInt8*) path, maxPathSize);
        const char kJavaConsoleLog[] = { "/Logs/Java Console.log" };
        int len = strlen(path);
        if (err == noErr && (len + sizeof(kJavaConsoleLog)) <= maxPathSize) {
            strcat(path + len, kJavaConsoleLog);
        }
    }
    return path;
}

nsresult MRJPlugin::StartupJVM()
{
    if (mSession == NULL) {
        
        mSession = new MRJSession();

        
        FSSpec jarFileSpec = { thePluginSpec.vRefNum, thePluginSpec.parID, "\pMRJPlugin.jar" };
        mSession->addToClassPath(jarFileSpec);
        
        
        char consolePath[512];
        mSession->open(getJavaConsolePath(consolePath, sizeof(consolePath)));

        if (mSession->getStatus() != noErr) {
            
            delete mSession;
            mSession = NULL;
            return NS_ERROR_FAILURE;
        }

        InitLiveConnectSupport(this);

#if 0
        
        if (mThreadManager != NULL) {
            PRUint32 threadID;
            mThreadManager->CreateThread(&threadID, this);
        }
#endif

        mIsEnabled = true;
    }
    return NS_OK;
}

NS_METHOD MRJPlugin::AddToClassPath(const char* dirPath)
{
    if (mSession != NULL) {
        mSession->addToClassPath(dirPath);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_METHOD MRJPlugin::GetClassPath(const char* *result)
{
    char* classPath = mSession->getProperty("java.class.path");
    *result = classPath;
    return (classPath != NULL ? NS_OK : NS_ERROR_FAILURE);
}

NS_METHOD MRJPlugin::GetJavaWrapper(JNIEnv* env, jint jsobj, jobject *jobj)
{
    
    
    *jobj = Wrap_JSObject(env, jsobj);
    return NS_OK;
}

NS_METHOD MRJPlugin::UnwrapJavaWrapper(JNIEnv* env, jobject jobj, jint* jsobj)
{
    *jsobj = Unwrap_JSObject(env, jobj);
    return NS_OK;
}

NS_METHOD MRJPlugin::CreateSecureEnv(JNIEnv* proxyEnv, nsISecureEnv* *outSecureEnv)
{
    *outSecureEnv = NULL;
    nsresult rv = StartupJVM();
    if (rv == NS_OK) {
        
        
        static NS_DEFINE_IID(kISecureEnvIID, NS_ISECUREENV_IID);
        rv = CSecureEnv::Create(this, proxyEnv, kISecureEnvIID, (void**)outSecureEnv);
    }
    return rv;
}

NS_METHOD MRJPlugin::SpendTime(PRUint32 timeMillis)
{
    nsresult result = NS_OK;
    
    if (MRJPluginInstance::getInstances() == NULL) {
        if (mSession == NULL)
            result = StartupJVM();
        if (mSession != NULL)
            mSession->idle(timeMillis);
    }
    return result;
}












NS_METHOD MRJPlugin::Show()
{
    
    char consolePath[512];
    getJavaConsolePath(consolePath, sizeof(consolePath));
    CFURLRef consoleURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)consolePath, strlen(consolePath), false);
    if (consoleURL) {
        LSOpenCFURLRef(consoleURL, NULL);
        CFRelease(consoleURL);
    }
    return NS_OK;
}

NS_METHOD MRJPlugin::Hide()
{
    
    return NS_OK;
}

NS_METHOD MRJPlugin::IsVisible(PRBool *result)
{
    
    *result = PR_FALSE;
    return NS_OK;
}
    
NS_METHOD MRJPlugin::Print(const char* msg, const char* )
{
    
    return NS_OK;
}

NS_METHOD MRJPlugin::Run()
{
    while (mSession != NULL) {
        mSession->idle();
        mThreadManager->Sleep();
    }
    return NS_OK;
}

MRJPluginInstance* MRJPlugin::getPluginInstance(jobject applet)
{
    JNIEnv* env = mSession->getCurrentEnv();
    MRJPluginInstance* instance = MRJPluginInstance::getInstances();
    while (instance != NULL) {
        jobject object = NULL;
        if (instance->GetJavaObject(&object) == NS_OK && env->IsSameObject(applet, object)) {
            instance->AddRef();
            return instance;
        }
        instance = instance->getNextInstance();
    }
    return NULL;
}

MRJPluginInstance* MRJPlugin::getPluginInstance(JNIEnv* jenv)
{
#if !TARGET_CARBON
    
    MRJPluginInstance* instance = MRJPluginInstance::getInstances();
    if (&::JMJNIToAWTContext != NULL) {
        JMAWTContextRef contextRef = ::JMJNIToAWTContext(mSession->getSessionRef(), jenv);
        if (contextRef != NULL) {
            while (instance != NULL) {
                if (instance->getContext()->getContextRef() == contextRef) {
                    instance->AddRef();
                    return instance;
                }
                instance = instance->getNextInstance();
            }
        }
    } else {
        if (instance != NULL) {
            instance->AddRef();
            return instance;
        }
    }
#endif
    return NULL;
}

Boolean MRJPlugin::inPluginThread()
{
    Boolean result = false;
    nsPluginThread *currentThreadID = NULL;
    
    if (mThreadManager != NULL)
        mThreadManager->GetCurrentThread(&currentThreadID);
    if ((NULL != currentThreadID) && (NULL != mPluginThreadID)) {
        if (currentThreadID == mPluginThreadID) {
            result = true;
        }
    }
    
    return result;
}

#pragma mark *** MRJPluginInstance ***

const InterfaceInfo MRJPluginInstance::sInterfaces[] = {
    { NS_IPLUGININSTANCE_IID, INTERFACE_OFFSET(MRJPluginInstance, nsIPluginInstance) },
    { NS_IJVMPLUGININSTANCE_IID, INTERFACE_OFFSET(MRJPluginInstance, nsIJVMPluginInstance) },
    { NS_IEVENTHANDLER_IID, INTERFACE_OFFSET(MRJPluginInstance, nsIEventHandler) },
};
const UInt32 MRJPluginInstance::kInterfaceCount = sizeof(sInterfaces) / sizeof(InterfaceInfo);

MRJPluginInstance::MRJPluginInstance(MRJPlugin* plugin)
    :   SupportsMixin(this, sInterfaces, kInterfaceCount),
        mPeer(NULL), mWindowlessPeer(NULL),
        mPlugin(plugin), mSession(plugin->getSession()),
        mContext(NULL), mApplet(NULL), mPluginWindow(NULL),
        mNext(NULL)
{
    
    pushInstance();

    
    mPlugin->AddRef();
}

MRJPluginInstance::~MRJPluginInstance()
{
    
    popInstance();

#if 0
    if (mContext != NULL) {
        delete mContext;
        mContext = NULL;
    }

    if (mPlugin != NULL) {
        mPlugin->Release();
        mPlugin = NULL;
    }

    if (mWindowlessPeer != NULL) {
        mWindowlessPeer->Release();
        mWindowlessPeer = NULL;
    }

    if (mPeer != NULL) {
        mPeer->Release();
        mPeer = NULL;
    }

    if (mApplet != NULL) {
        JNIEnv* env = mSession->getCurrentEnv();
        env->DeleteGlobalRef(mApplet);
        mApplet = NULL;
    }
#endif
}

static const char* kGetCodeBaseScriptURL = "javascript:var href = window.location.href; href.substring(0, href.lastIndexOf('/') + 1)";
static const char* kGetDocumentBaseScriptURL = "javascript:window.location";

static bool hasTagInfo(nsISupports* supports)
{
    nsIJVMPluginTagInfo* tagInfo;
    if (supports->QueryInterface(NS_GET_IID(nsIJVMPluginTagInfo), (void**)&tagInfo) == NS_OK) {
        NS_RELEASE(tagInfo);
        return true;
    }
    return false;
}

NS_METHOD MRJPluginInstance::Initialize(nsIPluginInstancePeer* peer)
{
    
    mPeer = peer;
    mPeer->AddRef();

    
    nsresult result = mPeer->QueryInterface(kIWindowlessPluginInstancePeerIID, (void**)&mWindowlessPeer);
    if (result != NS_OK) mWindowlessPeer = NULL;

    
    mContext = new MRJContext(mSession, this);

    if (hasTagInfo(mPeer)) {
        mContext->processAppletTag();
        mContext->createContext();
    } else {
        
        
        nsIPluginInstance* pluginInstance = this;
        nsIPluginStreamListener* listener = this;
        result = thePluginManager->GetURL(pluginInstance, kGetDocumentBaseScriptURL, NULL, listener);
    }

    return NS_OK;
}

NS_METHOD MRJPluginInstance::OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length)
{
    
    char* documentBase = new char[length + 1];
    if (documentBase != NULL) {
        if (input->Read(documentBase, length, &length) == NS_OK) {
            
            
            documentBase[length] = '\0';
            
            
            mContext->setDocumentBase(documentBase);
            delete[] documentBase;
            
            mContext->processAppletTag();
            mContext->createContext();
            
            
            if (mPluginWindow != NULL)
                mContext->setWindow(mPluginWindow);
        }
    }
    return NS_OK;
}

NS_METHOD MRJPluginInstance::GetPeer(nsIPluginInstancePeer* *result)
{
    NS_IF_ADDREF(mPeer);
    *result = mPeer;
    return NS_OK;
}

NS_METHOD MRJPluginInstance::Start()
{
    
    mContext->showFrames();
    
    mContext->resumeApplet();
    
    return NS_OK;
}

NS_METHOD MRJPluginInstance::Stop()
{
    
    mContext->hideFrames();

    mContext->suspendApplet();

    return NS_OK;
}

NS_METHOD MRJPluginInstance::Destroy()
{
    
    
    if (mContext != NULL) {
        delete mContext;
        mContext = NULL;
    }

    if (mPlugin != NULL) {
        mPlugin->Release();
        mPlugin = NULL;
    }

    if (mWindowlessPeer != NULL) {
        mWindowlessPeer->Release();
        mWindowlessPeer = NULL;
    }

    if (mPeer != NULL) {
        mPeer->Release();
        mPeer = NULL;
    }

    if (mApplet != NULL) {
        JNIEnv* env = mSession->getCurrentEnv();
        env->DeleteGlobalRef(mApplet);
        mApplet = NULL;
    }

    return NS_OK;
}



NS_METHOD MRJPluginInstance::SetWindow(nsPluginWindow* pluginWindow)
{
    mPluginWindow = pluginWindow;

    mContext->setWindow(pluginWindow);

    return NS_OK;
}

NS_METHOD MRJPluginInstance::HandleEvent(nsPluginEvent* pluginEvent, PRBool* eventHandled)
{
    *eventHandled = PR_TRUE;
    Boolean isUpdate;

    if (pluginEvent != NULL) {
        EventRecord* event = pluginEvent->event;

        
        
        isUpdate = (event->what == updateEvt);
        inspectInstance(isUpdate);
    
        if (event->what == nullEvent) {
            
            mSession->idle(0x00000400); 
        } else {
#if TARGET_CARBON
            *eventHandled = mContext->handleEvent(event);
#else
            MRJFrame* frame = mContext->findFrame(pluginEvent->window);
            if (frame != NULL) {
                switch (event->what) {
                case nsPluginEventType_GetFocusEvent:
                    frame->focusEvent(true);
                    break;
                
                case nsPluginEventType_LoseFocusEvent:
                    frame->focusEvent(false);
                    break;

                case nsPluginEventType_AdjustCursorEvent:
                    frame->idle(event->modifiers);
                    break;
                
                case nsPluginEventType_MenuCommandEvent:
                    frame->menuSelected(event->message, event->modifiers);
                    break;
                    
                default:
                    *eventHandled = frame->handleEvent(event);
                    break;
                }
            } else {
                if (event->what == updateEvt) {
                    mContext->drawApplet();
                }
            }
#endif
        }
    }
    
    return NS_OK;
}

NS_METHOD MRJPluginInstance::Print(nsPluginPrint* platformPrint)
{
    if (platformPrint->mode == nsPluginMode_Embedded) {
        mContext->printApplet(&platformPrint->print.embedPrint.window);
        return NS_OK;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD MRJPluginInstance::GetValue(nsPluginInstanceVariable variable, void *value)
{
    switch (variable) {
    case nsPluginInstanceVariable_WindowlessBool:
        *(PRBool*)value = PR_FALSE;
        break;
    case nsPluginInstanceVariable_TransparentBool:
        *(PRBool*)value = PR_FALSE;
        break;
    case nsPluginInstanceVariable_DoCacheBool:
        *(PRBool*)value = PR_FALSE;
        break;
    default:
        break;
    }
    return NS_OK;
}

NS_METHOD MRJPluginInstance::GetJavaObject(jobject *result)
{
    if (mApplet == NULL) {
        jobject applet = mContext->getApplet();
        JNIEnv* env = mSession->getCurrentEnv();
        mApplet = env->NewGlobalRef(applet);
    }
    *result = mApplet;
    return NS_OK;
}



static MRJPluginInstance* theInstances = NULL;

void MRJPluginInstance::pushInstance()
{
    mNext = theInstances;
    theInstances = this;
}

void MRJPluginInstance::popInstance()
{
    MRJPluginInstance** link = &theInstances;
    MRJPluginInstance* instance  = *link;
    while (instance != NULL) {
        if (instance == this) {
            *link = mNext;
            mNext = NULL;
            break;
        }
        link = &instance->mNext;
        instance = *link;
    }
}

MRJPluginInstance* MRJPluginInstance::getInstances()
{
    return theInstances;
}

MRJPluginInstance* MRJPluginInstance::getNextInstance()
{
    return mNext;
}

MRJContext* MRJPluginInstance::getContext()
{
    return mContext;
}

MRJSession* MRJPluginInstance::getSession()
{
    return mSession;
}

void MRJPluginInstance::inspectInstance(Boolean isUpdateEvt)
{
    if (mContext != NULL && mContext->inspectWindow() && !isUpdateEvt && mWindowlessPeer != NULL)
        mWindowlessPeer->ForceRedraw();
}
