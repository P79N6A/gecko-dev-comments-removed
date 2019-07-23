





































#include "mozilla/plugins/NPAPIPluginParent.h"

#include "base/task.h"

#include "mozilla/ipc/GeckoThread.h"

using mozilla::Monitor;
using mozilla::MonitorAutoEnter;
using mozilla::ipc::BrowserProcessSubThread;

template<>
struct RunnableMethodTraits<mozilla::plugins::NPAPIPluginParent>
{
    static void RetainCallee(mozilla::plugins::NPAPIPluginParent* obj) { }
    static void ReleaseCallee(mozilla::plugins::NPAPIPluginParent* obj) { }
};

namespace mozilla {
namespace plugins {


SharedLibrary*
NPAPIPluginParent::LoadModule(const char* aFilePath, PRLibrary* aLibrary)
{
    _MOZ_LOG(__FUNCTION__);

    
    NPAPIPluginParent* parent = new NPAPIPluginParent(aFilePath);

    
    {MonitorAutoEnter mon(parent->mMonitor);
        BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO)
            ->PostTask(
                FROM_HERE,
                NewRunnableMethod(parent,
                                  &NPAPIPluginParent::LaunchSubprocess));
        mon.Wait();
    }

    parent->Open(parent->mSubprocess.GetChannel());

    
    return parent->mShim;
}


NPAPIPluginParent::NPAPIPluginParent(const char* aFilePath) :
    mFilePath(aFilePath),
    mSubprocess(aFilePath),
    mMonitor("mozilla.plugins.NPAPIPluginParent.LaunchPluginProcess"),
    ALLOW_THIS_IN_INITIALIZER_LIST(mShim(new Shim(this)))
{
}

NPAPIPluginParent::~NPAPIPluginParent()
{
    _MOZ_LOG("  (closing Shim ...)");
    delete mShim;
}

void
NPAPIPluginParent::LaunchSubprocess()
{
    MonitorAutoEnter mon(mMonitor);
    mSubprocess.Launch();
    mon.Notify();
}

NPPProtocolParent*
NPAPIPluginParent::NPPConstructor(const String& aMimeType,
                                  const uint16_t& aMode,
                                  const StringArray& aNames,
                                  const StringArray& aValues,
                                  NPError* rv)
{
    _MOZ_LOG(__FUNCTION__);
    return new NPPInstanceParent(mNPNIface);
}

nsresult
NPAPIPluginParent::NPPDestructor(NPPProtocolParent* __a,
                                 NPError* rv)
{
    _MOZ_LOG(__FUNCTION__);
    delete __a;
    return NS_OK;
}

void
NPAPIPluginParent::SetPluginFuncs(NPPluginFuncs* aFuncs)
{
    aFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    aFuncs->javaClass = nsnull;

    
    
    aFuncs->newp = Shim::NPP_New;
    aFuncs->destroy = Shim::NPP_Destroy;
    aFuncs->setwindow = Shim::NPP_SetWindow;
    aFuncs->newstream = Shim::NPP_NewStream;
    aFuncs->destroystream = Shim::NPP_DestroyStream;
    aFuncs->asfile = Shim::NPP_StreamAsFile;
    aFuncs->writeready = Shim::NPP_WriteReady;
    aFuncs->write = Shim::NPP_Write;
    aFuncs->print = Shim::NPP_Print;
    aFuncs->event = Shim::NPP_HandleEvent;
    aFuncs->urlnotify = Shim::NPP_URLNotify;
    aFuncs->getvalue = Shim::NPP_GetValue;
    aFuncs->setvalue = Shim::NPP_SetValue;
}

#ifdef OS_LINUX
NPError
NPAPIPluginParent::NP_Initialize(const NPNetscapeFuncs* npnIface,
                                 NPPluginFuncs* nppIface)
{
    _MOZ_LOG(__FUNCTION__);

    NPError prv;
    nsresult rv = CallNP_Initialize(&prv);
    if (NS_OK != rv)
        return NPERR_GENERIC_ERROR;
    else if (NPERR_NO_ERROR != prv)
        return prv;

    SetPluginFuncs(nppIface);
    return NPERR_NO_ERROR;
}
#else
NPError
NPAPIPluginParent::NP_Initialize(const NPNetscapeFuncs* npnIface)
{
    _MOZ_LOG(__FUNCTION__);

    NPError prv;
    nsresult rv = CallNP_Initialize(&prv);
    if (NS_FAILED(rv))
        return rv;
    return prv;
}

NPError
NPAPIPluginParent::NP_GetEntryPoints(NPPluginFuncs* nppIface)
{
    NS_ASSERTION(nppIface, "Null pointer!");

    SetPluginFuncs(nppIface);
    return NPERR_NO_ERROR;
}
#endif

NPError
NPAPIPluginParent::NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
                           int16_t argc, char* argn[], char* argv[],
                           NPSavedData* saved)
{
    _MOZ_LOG(__FUNCTION__);

    
    StringArray names;
    StringArray values;

    for (int i = 0; i < argc; ++i) {
        names.push_back(argn[i]);
        values.push_back(argv[i]);
    }

    NPError prv;
    nsAutoPtr<NPPInstanceParent> parentInstance(
        static_cast<NPPInstanceParent*>(CallNPPConstructor(pluginType,
                                                           mode, names,
                                                           values,
                                                           &prv)));
    printf ("[NPAPIPluginParent] %s: got return value %hd\n", __FUNCTION__,
            prv);

    if (NPERR_NO_ERROR != prv)
        return prv;
    NS_ASSERTION(parentInstance,
                 "if there's no parentInstance, there should be an error");

    instance->pdata = (void*) parentInstance.forget();
    return prv;
}

NPError
NPAPIPluginParent::NPP_Destroy(NPP instance,
                               NPSavedData** save)
{
    
    
    
    
    

    _MOZ_LOG(__FUNCTION__);

    NPPInstanceParent* parentInstance =
        static_cast<NPPInstanceParent*>(instance->pdata);

    NPError prv;
    if (CallNPPDestructor(parentInstance, &prv)) {
        prv = NPERR_GENERIC_ERROR;
    }
    instance->pdata = nsnull;

    return prv;
 }


NPAPIPluginParent* NPAPIPluginParent::Shim::HACK_target;


} 
} 

