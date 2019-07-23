





































#include "base/task.h"

#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/plugins/NPAPIPluginParent.h"

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
NPAPIPluginParent::LoadModule(const char* aFullPath,
                              PRLibrary* aLibrary)
{
    _MOZ_LOG(__FUNCTION__);

    
    NPAPIPluginParent* parent = new NPAPIPluginParent(aFullPath);

    
    {MonitorAutoEnter mon(parent->mMonitor);
        BrowserProcessSubThread::GetMessageLoop(BrowserProcessSubThread::IO)
            ->PostTask(
                FROM_HERE,
                NewRunnableMethod(parent,
                                  &NPAPIPluginParent::LaunchSubprocess));
        mon.Wait();
    }

    parent->mNpapi.Open(parent->mSubprocess.GetChannel());

    
    return parent->mShim;
}


NPAPIPluginParent::NPAPIPluginParent(const char* aFullPath) :
    mFullPath(aFullPath),       
    mSubprocess(aFullPath),
    mNpapi(this),
    mMonitor("mozilla.plugins.NPAPIPluginParent.LaunchPluginProcess"),
    mShim(new Shim(this))
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

void
NPAPIPluginParent::NPN_GetValue()
{
    _MOZ_LOG(__FUNCTION__);
}

NPError
NPAPIPluginParent::NP_Initialize(const NPNetscapeFuncs* npnIface,
                                 NPPluginFuncs* nppIface)
{
    _MOZ_LOG(__FUNCTION__);

    NPError rv = mNpapi.NP_Initialize();
    if (NPERR_NO_ERROR != rv)
        return rv;

    nppIface->version = mVersion;
    nppIface->javaClass = mJavaClass;

    
    
    nppIface->newp = Shim::NPP_New;
    nppIface->destroy = Shim::NPP_Destroy;
    nppIface->setwindow = Shim::NPP_SetWindow;
    nppIface->newstream = Shim::NPP_NewStream;
    nppIface->destroystream = Shim::NPP_DestroyStream;
    nppIface->asfile = Shim::NPP_StreamAsFile;
    nppIface->writeready = Shim::NPP_WriteReady;
    nppIface->write = Shim::NPP_Write;
    nppIface->print = Shim::NPP_Print;
    nppIface->event = Shim::NPP_HandleEvent;
    nppIface->urlnotify = Shim::NPP_URLNotify;
    nppIface->getvalue = Shim::NPP_GetValue;
    nppIface->setvalue = Shim::NPP_SetValue;

    return NPERR_NO_ERROR;
}

NPError
NPAPIPluginParent::NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
                           int16_t argc, char* argn[], char* argv[],
                           NPSavedData* saved)
{
    _MOZ_LOG(__FUNCTION__);

    
    NPPInstanceParent* parentInstance = new NPPInstanceParent(mNPNIface);

    
    StringArray names;
    StringArray values;

    for (int i = 0; i < argc; ++i) {
        names.push_back(argn[i]);
        values.push_back(argv[i]);
    }

    NPError rv = mNpapi.NPP_New(pluginType,
                                42,
                                mode, names,
                                values);
    printf ("[NPAPIPluginParent] %s: got return value %hd\n", __FUNCTION__,
            rv);

    if (NPERR_NO_ERROR != rv)
        return rv;


    
    parentInstance->mNpp.SetChannel(mNpapi.HACK_getchannel_please());
    mNpapi.HACK_npp = &(parentInstance->mNpp);


    instance->pdata = (void*) parentInstance;
    return rv;
}


NPAPIPluginParent* NPAPIPluginParent::Shim::HACK_target;


} 
} 

