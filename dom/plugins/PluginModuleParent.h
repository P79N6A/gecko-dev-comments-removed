





































#ifndef dom_plugins_PluginModuleParent_h
#define dom_plugins_PluginModuleParent_h 1

#include <cstring>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"

#include "base/string_util.h"

#include "mozilla/FileUtils.h"
#include "mozilla/PluginLibrary.h"
#include "mozilla/plugins/PPluginModuleParent.h"
#include "mozilla/plugins/PluginInstanceParent.h"
#include "mozilla/plugins/PluginProcessParent.h"
#include "mozilla/plugins/PluginIdentifierParent.h"

#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsIFileStreams.h"
#include "nsTObserverArray.h"
#include "nsITimer.h"

namespace mozilla {
namespace plugins {


class BrowserStreamParent;












class PluginModuleParent : public PPluginModuleParent, PluginLibrary
{
private:
    typedef mozilla::PluginLibrary PluginLibrary;

protected:

    virtual PPluginIdentifierParent*
    AllocPPluginIdentifier(const nsCString& aString,
                           const int32_t& aInt);

    virtual bool
    DeallocPPluginIdentifier(PPluginIdentifierParent* aActor);

    PPluginInstanceParent*
    AllocPPluginInstance(const nsCString& aMimeType,
                         const uint16_t& aMode,
                         const InfallibleTArray<nsCString>& aNames,
                         const InfallibleTArray<nsCString>& aValues,
                         NPError* rv);

    virtual bool
    DeallocPPluginInstance(PPluginInstanceParent* aActor);

public:
    
    PluginModuleParent(const char* aFilePath);
    virtual ~PluginModuleParent();

    NS_OVERRIDE virtual void SetPlugin(nsNPAPIPlugin* plugin)
    {
        mPlugin = plugin;
    }

    NS_OVERRIDE virtual void ActorDestroy(ActorDestroyReason why);

    





    static PluginLibrary* LoadModule(const char* aFilePath);

    const NPNetscapeFuncs* GetNetscapeFuncs() {
        return mNPNIface;
    }

    PluginProcessParent* Process() const { return mSubprocess; }
    base::ProcessHandle ChildProcessHandle() { return mSubprocess->GetChildProcessHandle(); }

    bool OkToCleanup() const {
        return !IsOnCxxStack();
    }

    PPluginIdentifierParent*
    GetIdentifierForNPIdentifier(NPIdentifier aIdentifier);

#ifdef OS_MACOSX
    void AddToRefreshTimer(PluginInstanceParent *aInstance);
    void RemoveFromRefreshTimer(PluginInstanceParent *aInstance);
#endif

protected:
    NS_OVERRIDE
    virtual mozilla::ipc::RPCChannel::RacyRPCPolicy
    MediateRPCRace(const Message& parent, const Message& child)
    {
        return MediateRace(parent, child);
    }

    virtual bool RecvXXX_HACK_FIXME_cjones(Shmem& mem) { NS_RUNTIMEABORT("not reached"); return false; }

    NS_OVERRIDE
    virtual bool ShouldContinueFromReplyTimeout();

    NS_OVERRIDE
    virtual bool
    RecvBackUpXResources(const FileDescriptor& aXSocketFd);

    virtual bool
    AnswerNPN_UserAgent(nsCString* userAgent);

    virtual bool
    AnswerNPN_GetValue_WithBoolReturn(const NPNVariable& aVariable,
                                      NPError* aError,
                                      bool* aBoolVal);

    NS_OVERRIDE
    virtual bool AnswerProcessSomeEvents();

    NS_OVERRIDE virtual bool
    RecvProcessNativeEventsInRPCCall();

    virtual bool
    RecvAppendNotesToCrashReport(const nsCString& aNotes);

    NS_OVERRIDE virtual bool
    RecvPluginShowWindow(const uint32_t& aWindowId, const bool& aModal,
                         const int32_t& aX, const int32_t& aY,
                         const size_t& aWidth, const size_t& aHeight);

    NS_OVERRIDE virtual bool
    RecvPluginHideWindow(const uint32_t& aWindowId);

    static PluginInstanceParent* InstCast(NPP instance);
    static BrowserStreamParent* StreamCast(NPP instance, NPStream* s);

private:
    void SetPluginFuncs(NPPluginFuncs* aFuncs);

    
    
#ifdef OS_LINUX
    NPError NP_Initialize(const NPNetscapeFuncs* npnIface,
                          NPPluginFuncs* nppIface);
#else
    NPError NP_Initialize(const NPNetscapeFuncs* npnIface);
    NPError NP_GetEntryPoints(NPPluginFuncs* nppIface);
#endif

    
    
    

    static NPError NPP_Destroy(NPP instance, NPSavedData** save);

    static NPError NPP_SetWindow(NPP instance, NPWindow* window);
    static NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream,
                                 NPBool seekable, uint16_t* stype);
    static NPError NPP_DestroyStream(NPP instance,
                                     NPStream* stream, NPReason reason);
    static int32_t NPP_WriteReady(NPP instance, NPStream* stream);
    static int32_t NPP_Write(NPP instance, NPStream* stream,
                             int32_t offset, int32_t len, void* buffer);
    static void NPP_StreamAsFile(NPP instance,
                                 NPStream* stream, const char* fname);
    static void NPP_Print(NPP instance, NPPrint* platformPrint);
    static int16_t NPP_HandleEvent(NPP instance, void* event);
    static void NPP_URLNotify(NPP instance, const char* url,
                              NPReason reason, void* notifyData);
    static NPError NPP_GetValue(NPP instance,
                                NPPVariable variable, void *ret_value);
    static NPError NPP_SetValue(NPP instance, NPNVariable variable,
                                void *value);
    static void NPP_URLRedirectNotify(NPP instance, const char* url,
                                      int32_t status, void* notifyData);

    virtual bool HasRequiredFunctions();
    virtual nsresult AsyncSetWindow(NPP instance, NPWindow* window);
    virtual nsresult GetImage(NPP instance, mozilla::layers::ImageContainer* aContainer, mozilla::layers::Image** aImage);
    virtual nsresult GetImageSize(NPP instance, nsIntSize* aSize);
    NS_OVERRIDE virtual bool UseAsyncPainting() { return true; }
    NS_OVERRIDE
    virtual nsresult SetBackgroundUnknown(NPP instance);
    NS_OVERRIDE
    virtual nsresult BeginUpdateBackground(NPP instance,
                                           const nsIntRect& aRect,
                                           gfxContext** aCtx);
    NS_OVERRIDE
    virtual nsresult EndUpdateBackground(NPP instance,
                                         gfxContext* aCtx,
                                         const nsIntRect& aRect);

#if defined(XP_UNIX) && !defined(XP_MACOSX)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error);
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error);
#endif
    virtual nsresult NP_Shutdown(NPError* error);
    virtual nsresult NP_GetMIMEDescription(const char** mimeDesc);
    virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                                 void *aValue, NPError* error);
#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error);
#endif
    virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                             uint16_t mode, int16_t argc, char* argn[],
                             char* argv[], NPSavedData* saved,
                             NPError* error);
    virtual nsresult NPP_ClearSiteData(const char* site, uint64_t flags,
                                       uint64_t maxAge);
    virtual nsresult NPP_GetSitesWithData(InfallibleTArray<nsCString>& result);

#if defined(XP_MACOSX)
    virtual nsresult IsRemoteDrawingCoreAnimation(NPP instance, PRBool *aDrawing);
#endif

private:
    void WritePluginExtraDataForMinidump(const nsAString& id);
    void WriteExtraDataForHang();
    void CleanupFromTimeout();
    static int TimeoutChanged(const char* aPref, void* aModule);
    void NotifyPluginCrashed();

    nsCString mCrashNotes;
    PluginProcessParent* mSubprocess;
    
    NativeThreadId mPluginThread;
    bool mShutdown;
    bool mClearSiteDataSupported;
    bool mGetSitesWithDataSupported;
    const NPNetscapeFuncs* mNPNIface;
    nsDataHashtable<nsVoidPtrHashKey, PluginIdentifierParent*> mIdentifiers;
    nsNPAPIPlugin* mPlugin;
    time_t mProcessStartTime;
    ScopedRunnableMethodFactory<PluginModuleParent> mTaskFactory;
    nsString mPluginDumpID;
    nsString mBrowserDumpID;
    nsString mHangID;

#ifdef OS_MACOSX
    nsCOMPtr<nsITimer> mCATimer;
    nsTObserverArray<PluginInstanceParent*> mCATimerTargets;
#endif

#ifdef MOZ_X11
    
    
    ScopedClose mPluginXSocketFdDup;
#endif
};

} 
} 

#endif  
