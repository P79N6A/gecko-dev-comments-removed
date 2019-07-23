





































#ifndef dom_plugins_PluginModuleParent_h
#define dom_plugins_PluginModuleParent_h 1

#include <cstring>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"
#include "nsplugindefs.h"

#include "base/string_util.h"

#include "mozilla/SharedLibrary.h"
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













class PluginModuleParent : public PPluginModuleParent
{
private:
    typedef mozilla::SharedLibrary SharedLibrary;

protected:
    PPluginInstanceParent*
    PPluginInstanceConstructor(const nsCString& aMimeType,
                               const uint16_t& aMode,
                               const nsTArray<nsCString>& aNames,
                               const nsTArray<nsCString>& aValues,
                               NPError* rv);

    virtual nsresult
    PPluginInstanceDestructor(PPluginInstanceParent* aActor,
                              NPError* _retval);

public:
    PluginModuleParent(const char* aFilePath);

    virtual ~PluginModuleParent();

    






    static SharedLibrary* LoadModule(const char* aFilePath,
                                     PRLibrary* aLibrary);

    
    virtual nsresult
    RecvNPN_GetStringIdentifier(const nsCString& aString,
                                NPRemoteIdentifier* aId);
    virtual nsresult
    RecvNPN_GetIntIdentifier(const int32_t& aInt,
                             NPRemoteIdentifier* aId);
    virtual nsresult
    RecvNPN_UTF8FromIdentifier(const NPRemoteIdentifier& aId,
                               nsCString* aString);
    virtual nsresult
    RecvNPN_IntFromIdentifier(const NPRemoteIdentifier& aId,
                              int32_t* aInt);
    virtual nsresult
    RecvNPN_IdentifierIsString(const NPRemoteIdentifier& aId,
                               bool* aIsString);
    virtual nsresult
    RecvNPN_GetStringIdentifiers(const nsTArray<nsCString>& aNames,
                                 nsTArray<NPRemoteIdentifier>* aIds);

private:
    void SetPluginFuncs(NPPluginFuncs* aFuncs);

    
    
#ifdef OS_LINUX
    NPError NP_Initialize(const NPNetscapeFuncs* npnIface,
                          NPPluginFuncs* nppIface);
#else
    NPError NP_Initialize(const NPNetscapeFuncs* npnIface);
    NPError NP_GetEntryPoints(NPPluginFuncs* nppIface);
#endif

    NPError NP_Shutdown()
    {
        
        

        _MOZ_LOG(__FUNCTION__);
        return 1;
    }

    char* NP_GetPluginVersion()
    {
        _MOZ_LOG(__FUNCTION__);
        return (char*) "0.0";
    }

    char* NP_GetMIMEDescription()
    {
        _MOZ_LOG(__FUNCTION__);
        return (char*) "application/x-foobar";
    }

    NPError NP_GetValue(void *future,
                        nsPluginVariable aVariable, void *aValue)
    {
        _MOZ_LOG(__FUNCTION__);
        return 1;
    }


    
    
    

    NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
                    int16_t argc, char* argn[], char* argv[],
                    NPSavedData* saved);

    static NPError NPP_Destroy(NPP instance, NPSavedData** save);

    static PluginInstanceParent* InstCast(NPP instance);
    static BrowserStreamParent* StreamCast(NPP instance, NPStream* s);

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

private:
    const char* mFilePath;
    PluginProcessParent mSubprocess;
    const NPNetscapeFuncs* mNPNIface;

    

    




    



















    class Shim : public SharedLibrary
    {
    public:
        Shim(PluginModuleParent* aTarget) :
            mTarget(aTarget)
        {
            HACK_target = mTarget;
        }
        virtual ~Shim()
        {
            mTarget = 0;
        }

        virtual symbol_type
        FindSymbol(const char* aSymbolName)
        {
            if (!strcmp("NP_Shutdown", aSymbolName))
                return (symbol_type) NP_Shutdown;
            if (!strcmp("NP_Initialize", aSymbolName))
                return (symbol_type) NP_Initialize;
            if (!strcmp("NP_GetMIMEDescription", aSymbolName))
                return (symbol_type) NP_GetMIMEDescription;
            if (!strcmp("NP_GetValue", aSymbolName))
                return (symbol_type) NP_GetValue;
#ifdef OS_WIN
            if (!strcmp("NP_GetEntryPoints", aSymbolName))
                return (symbol_type) NP_GetEntryPoints;
#endif

            _MOZ_LOG("WARNING! FAILED TO FIND SYMBOL");
            return 0;
        }

        virtual function_type
        FindFunctionSymbol(const char* aSymbolName)
        {
            if (!strcmp("NP_Shutdown", aSymbolName))
                return (function_type) NP_Shutdown;
            if (!strcmp("NP_Initialize", aSymbolName))
                return (function_type) NP_Initialize;
            if (!strcmp("NP_GetMIMEDescription", aSymbolName))
                return (function_type) NP_GetMIMEDescription;
            if (!strcmp("NP_GetValue", aSymbolName))
                return (function_type) NP_GetValue;
#ifdef OS_WINDOWS
            if (!strcmp("NP_GetEntryPoints", aSymbolName))
                return (function_type) NP_GetEntryPoints;
#endif

            _MOZ_LOG("WARNING! FAILED TO FIND SYMBOL");
            return 0;
        }

    private:
        PluginModuleParent* mTarget;

        

#ifdef OS_LINUX
        static NPError NP_Initialize(const NPNetscapeFuncs* npnIface,
                                     NPPluginFuncs* nppIface)
        {
            return HACK_target->NP_Initialize(npnIface, nppIface);
        }
#else
        static NPError NP_Initialize(const NPNetscapeFuncs* npnIface)
        {
            return HACK_target->NP_Initialize(npnIface);
        }
        static NPError NP_GetEntryPoints(NPPluginFuncs* nppIface)
        {
            return HACK_target->NP_GetEntryPoints(nppIface);
        }
#endif
        static NPError NP_Shutdown()
        {
            return HACK_target->NP_Shutdown();
        }
        static char* NP_GetPluginVersion()
        {
            return HACK_target->NP_GetPluginVersion();
        }
        static char* NP_GetMIMEDescription()
        {
            return HACK_target->NP_GetMIMEDescription();
        }
        static NPError NP_GetValue(void *future,
                                   nsPluginVariable aVariable, void *aValue)
        {
            return HACK_target->NP_GetValue(future, aVariable, aValue);
        }
        static NPError NPP_New(NPMIMEType pluginType, NPP instance,
                               uint16_t mode,
                               int16_t argc, char* argn[], char* argv[],
                               NPSavedData* saved)
        {
            return HACK_target->NPP_New(pluginType, instance, mode,
                                        argc, argn, argv,
                                        saved);
        }

        static PluginModuleParent* HACK_target;
        friend class PluginModuleParent;
    };

    friend class Shim;
    Shim* mShim;

    nsTHashtable<nsVoidPtrHashKey> mValidIdentifiers;
};

} 
} 

#endif  
