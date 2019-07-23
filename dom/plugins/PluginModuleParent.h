





































#ifndef dom_plugins_PluginModuleParent_h
#define dom_plugins_PluginModuleParent_h 1

#include <cstring>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"

#include "base/string_util.h"

#include "mozilla/PluginLibrary.h"
#include "mozilla/plugins/PPluginModuleParent.h"
#include "mozilla/plugins/PluginInstanceParent.h"
#include "mozilla/plugins/PluginProcessParent.h"

#include "nsAutoPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[PluginModuleParent] %s\n", s)

namespace mozilla {
namespace plugins {


class BrowserStreamParent;












class PluginModuleParent : public PPluginModuleParent, PluginLibrary
{
private:
    typedef mozilla::PluginLibrary PluginLibrary;

protected:
    PPluginInstanceParent*
    AllocPPluginInstance(const nsCString& aMimeType,
                         const uint16_t& aMode,
                         const nsTArray<nsCString>& aNames,
                         const nsTArray<nsCString>& aValues,
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

    virtual bool
    AnswerNPN_UserAgent(nsCString* userAgent);

    
    virtual bool
    RecvNPN_GetStringIdentifier(const nsCString& aString,
                                NPRemoteIdentifier* aId);
    virtual bool
    RecvNPN_GetIntIdentifier(const int32_t& aInt,
                             NPRemoteIdentifier* aId);
    virtual bool
    RecvNPN_UTF8FromIdentifier(const NPRemoteIdentifier& aId,
                               NPError* err,
                               nsCString* aString);
    virtual bool
    RecvNPN_IntFromIdentifier(const NPRemoteIdentifier& aId,
                              NPError* err,
                              int32_t* aInt);
    virtual bool
    RecvNPN_IdentifierIsString(const NPRemoteIdentifier& aId,
                               bool* aIsString);
    virtual bool
    RecvNPN_GetStringIdentifiers(const nsTArray<nsCString>& aNames,
                                 nsTArray<NPRemoteIdentifier>* aIds);

    virtual bool
    AnswerNPN_GetValue_WithBoolReturn(const NPNVariable& aVariable,
                                      NPError* aError,
                                      bool* aBoolVal);

    const NPNetscapeFuncs* GetNetscapeFuncs() {
        return mNPNIface;
    }

    static PluginInstanceParent* InstCast(NPP instance);
    static BrowserStreamParent* StreamCast(NPP instance, NPStream* s);

    bool EnsureValidNPIdentifier(NPIdentifier aIdentifier);

    base::ProcessHandle ChildProcessHandle() { return mSubprocess->GetChildProcessHandle(); }
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

    NPIdentifier GetValidNPIdentifier(NPRemoteIdentifier aRemoteIdentifier);

    virtual bool HasRequiredFunctions();

#if defined(XP_UNIX) && !defined(XP_MACOSX)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error);
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error);
#endif
    virtual nsresult NP_Shutdown(NPError* error);
    virtual nsresult NP_GetMIMEDescription(char** mimeDesc);
    virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                                 void *aValue, NPError* error);
#if defined(XP_WIN) || defined(XP_MACOSX)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error);
#endif
    virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                             uint16_t mode, int16_t argc, char* argn[],
                             char* argv[], NPSavedData* saved,
                             NPError* error);
private:
    PluginProcessParent* mSubprocess;
    bool mShutdown;
    const NPNetscapeFuncs* mNPNIface;
    nsTHashtable<nsVoidPtrHashKey> mValidIdentifiers;
    nsNPAPIPlugin* mPlugin;
};

} 
} 

#endif  
