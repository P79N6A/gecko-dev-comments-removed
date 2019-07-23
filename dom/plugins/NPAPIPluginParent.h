





































#ifndef dom_plugins_NPAPIPluginParent_h
#define dom_plugins_NPAPIPluginParent_h

#include <cstring>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"
#include "nsplugindefs.h"

#include "base/string_util.h"

#include "mozilla/SharedLibrary.h"
#include "mozilla/plugins/NPAPIProtocol.h"
#include "mozilla/plugins/NPAPIProtocolParent.h"
#include "mozilla/plugins/NPPInstanceParent.h"
#include "mozilla/plugins/PluginProcessParent.h"

#include "nsAutoPtr.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPAPIPluginParent] %s\n", s)

namespace mozilla {
namespace plugins {













class NPAPIPluginParent : public NPAPIProtocolParent
{
private:
    typedef mozilla::SharedLibrary SharedLibrary;

protected:
    NPPProtocolParent* NPPConstructor(
                const nsCString& aMimeType,
                const uint16_t& aMode,
                const nsTArray<nsCString>& aNames,
                const nsTArray<nsCString>& aValues,
                NPError* rv);

    virtual nsresult NPPDestructor(
                NPPProtocolParent* __a,
                NPError* rv);

public:
    NPAPIPluginParent(const char* aFilePath);

    virtual ~NPAPIPluginParent();

    






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
    RecvNPN_GetStringIdentifiers(nsTArray<nsCString>* aNames,
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

    NPError NPP_Destroy(NPP instance, NPSavedData** save);

    static inline NPPInstanceParent& InstCast(void* p)
    {
        return *static_cast<NPPInstanceParent*>(p);
    }

    static inline const NPPInstanceParent& InstCast(const void* p)
    {
        return *static_cast<const NPPInstanceParent*>(p);
    }

    NPError NPP_SetWindow(NPP instance, NPWindow* window)
    {
        return InstCast(instance->pdata).NPP_SetWindow(window);
    }

    NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream,
                          NPBool seekable, uint16_t* stype)
    {
        return InstCast(instance->pdata).NPP_NewStream(type, stream, seekable, stype);
    }

    NPError NPP_DestroyStream(NPP instance,
                              NPStream* stream, NPReason reason)
    {
        return InstCast(instance->pdata).NPP_DestroyStream(stream, reason);
    }

    int32_t NPP_WriteReady(NPP instance, NPStream* stream)
    {
        return InstCast(instance->pdata).NPP_WriteReady(stream);
    }

    int32_t NPP_Write(NPP instance, NPStream* stream,
                      int32_t offset, int32_t len, void* buffer)
    {
        return InstCast(instance->pdata).NPP_Write(stream, offset, len, buffer);
    }

    void NPP_StreamAsFile(NPP instance,
                          NPStream* stream, const char* fname)
    {
        return InstCast(instance->pdata).NPP_StreamAsFile(stream, fname);
    }

    void NPP_Print(NPP instance, NPPrint* platformPrint)
    {
        return InstCast(instance->pdata).NPP_Print(platformPrint);
    }

    int16_t NPP_HandleEvent(NPP instance, void* event)
    {
        return InstCast(instance->pdata).NPP_HandleEvent(event);
    }

    void NPP_URLNotify(NPP instance,
                       const char* url, NPReason reason, void* notifyData)
    {
        return InstCast(instance->pdata).NPP_URLNotify(url,
                                                       reason, notifyData);
    }

    NPError NPP_GetValue(NPP instance,
                            NPPVariable variable, void *ret_value)
    {
        return InstCast(instance->pdata).NPP_GetValue(variable, ret_value);
    }

    NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
    {
        return InstCast(instance->pdata).NPP_SetValue(variable, value);
    }

#if 0
    
    
    
    

    bool
    NPN_SetProperty(NPP aInstance,
                    NPObject* aObj,
                    NPIdentifier aPropName,
                    const NPVariant* aValue)
    {
        return mNPNIface->setproperty(aInstance, aObj, aPropName, aValue);
    }

    NPIdentifier
    NPN_GetIntIdentifier(int32_t aInt)
    {
        return mNPNIface->getintidentifier(aInt);
    }

    NPIdentifier
    NPN_GetStringIdentifier(const NPUTF8* aName)
    {
        return mNPNIface->getstringidentifier(aName);
    }

    void
    NPN_GetStringIdentifiers(const NPUTF8** aNames,
                             int32_t aNamesCount,
                             NPIdentifier* aIdentifiers)
    {
        return mNPNIface->getstringidentifiers(aNames,
                                               aNamesCount,
                                               aIdentifiers);
    }

    NPUTF8*
    NPN_UTF8FromIdentifier(NPIdentifier aIdentifier)
    {
        return mNPNIface->utf8fromidentifier(aIdentifier);
    }

    int32_t
    NPN_IntFromIdentifier(NPIdentifier aIdentifier)
    {
        return mNPNIface->intfromidentifier(aIdentifier);
    }

    NPError
    NPN_GetValue(NPP aInstance,
                 NPNVariable aVariable,
                 void* aValue)
    {
        return mNPNIface->getvalue(aInstance, aVariable, aValue);
    }

    NPError
    NPN_SetValue(NPP aInstance,
                 NPPVariable aVariable,
                 void* aValue)
    {
        return mNPNIface->setvalue(aInstance, aVariable, aValue);
    }

    bool
    NPN_HasProperty(NPP aInstance,
                    NPObject* aObj,
                    NPIdentifier aPropName)
    {
        return mNPNIface->hasproperty(aInstance, aObj, aPropName);
    }

    NPObject*
    NPN_CreateObject(NPP aInstance,
                     NPClass* aClass)
    {
        return mNPNIface->createobject(aInstance, aClass);
    }

    const char*
    NPN_UserAgent(NPP aInstance)
    {
        return mNPNIface->uagent(aInstance);
    }

    NPObject*
    NPN_RetainObject(NPObject* aObj)
    {
        return mNPNIface->retainobject(aObj);
    }

    void
    NPN_ReleaseObject(NPObject* aObj)
    {
        return mNPNIface->releaseobject(aObj);
    }

    void*
    NPN_MemAlloc(uint32_t aSize)
    {
        return mNPNIface->memalloc(aSize);
    }

    void
    NPN_MemFree(void* aPtr)
    {
        return mNPNIface->memfree(aPtr);
    }


    
    static base::hash_map<int, PluginNPObject*> sNPObjects;
    static int sNextNPObjectId = 0;

    static NPObject*
    ScriptableAllocate(NPP aNPP,
                       NPClass* aClass)
    {
        PluginNPObject* obj = (PluginNPObject*)NPN_MemAlloc(sizeof(PluginNPObject));
        if (obj) {
            obj->objectId = sNextNPObjectId++;
            obj->classId = -1;
        }
        sNPObjects[obj->objectId] = obj;
        return obj;
    }

    static void
    ScriptableDeallocate(NPObject* aNPObj)
    {
        PluginNPObject* obj = static_cast<PluginNPObject*>(aNPObj);
        base::hash_map<int, PluginNPObject*>::iterator iter =
            sNPObjects.find(obj->objectId);
        if (iter != sNPObjects.end()) {
            sNPObjects.erase(iter);
        }
        NPN_MemFree(obj);
    }

    static void
    ScriptableInvalidate(NPObject* aNPObj)
    {
    }

    static bool
    ScriptableHasMethod(NPObject* aNPObj,
                        NPIdentifier aName)
    {
        return false;
    }

    static bool
    ScriptableInvoke(NPObject* aNPObj,
                     NPIdentifier aName,
                     const NPVariant* aArgs,
                     uint32_t aArgsCount,
                     NPVariant* aResult)
    {
        return false;
    }

    static bool
    ScriptableInvokeDefault(NPObject* aNPObj,
                            const NPVariant* aArgs,
                            uint32_t aArgsCount,
                            NPVariant* aResult)
    {
        return false;
    }

    static bool
    ScriptableHasProperty(NPObject* aNPObj,
                          NPIdentifier aName)
    {
        return false;
    }

    static bool
    ScriptableGetProperty(NPObject* aNPObj,
                          NPIdentifier aName,
                          NPVariant* aResult)
    {
        return false;
    }

    static bool
    ScriptableSetProperty(NPObject* aNPObj,
                          NPIdentifier aName,
                          const NPVariant* aValue)
    {
        return false;
    }

    static bool
    ScriptableRemoveProperty(NPObject* aNPObj,
                             NPIdentifier aName)
    {
        return false;
    }

    static bool
    ScriptableEnumerate(NPObject* aNPObj,
                        NPIdentifier** aIdentifier,
                        uint32_t* aCount)
    {
        return false;
    }

    static bool
    ScriptableConstruct(NPObject* aNPObj,
                        const NPVariant* aArgs,
                        uint32_t aArgsCount,
                        NPVariant* aResult)
    {
        return false;
    }

    static NPClass sNPClass = {
        NP_CLASS_STRUCT_VERSION,
        ScriptableAllocate,
        ScriptableDeallocate,
        ScriptableInvalidate,
        ScriptableHasMethod,
        ScriptableInvoke,
        ScriptableInvokeDefault,
        ScriptableHasProperty,
        ScriptableGetProperty,
        ScriptableSetProperty,
        ScriptableRemoveProperty,
        ScriptableEnumerate,
        ScriptableConstruct
    };
#endif


private:
    const char* mFilePath;
    PluginProcessParent mSubprocess;
    const NPNetscapeFuncs* mNPNIface;

    

#if 0
    struct PluginNPObject : NPObject
    {
        int objectId;
        int classId;
    };
#endif

    




    



















    class Shim : public SharedLibrary
    {
    public:
        Shim(NPAPIPluginParent* aTarget) :
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
        NPAPIPluginParent* mTarget;

        

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
        static NPError NPP_Destroy(NPP instance, NPSavedData** save)
        {
            return HACK_target->NPP_Destroy(instance, save);
        }
        static NPError NPP_SetWindow(NPP instance, NPWindow* window)
        {
            return HACK_target->NPP_SetWindow(instance, window);
        }
        static NPError NPP_NewStream(NPP instance,
                                     NPMIMEType type, NPStream* stream,
                                     NPBool seekable, uint16_t* stype)
        {
            return HACK_target->NPP_NewStream(instance, type, stream,
                                              seekable, stype);
        }
        static NPError NPP_DestroyStream(NPP instance,
                                         NPStream* stream, NPReason reason)
        {
            return HACK_target->NPP_DestroyStream(instance, stream, reason);
        }
        static int32_t NPP_WriteReady(NPP instance, NPStream* stream)
        {
            return HACK_target->NPP_WriteReady(instance, stream);
        }
        static int32_t NPP_Write(NPP instance, NPStream* stream,
                                 int32_t offset, int32_t len, void* buffer)
        {
            return HACK_target->NPP_Write(instance, stream,
                                          offset, len, buffer);
        }
        static void NPP_StreamAsFile(NPP instance,
                                     NPStream* stream, const char* fname)
        {
            return HACK_target->NPP_StreamAsFile(instance, stream, fname);
        }
        static void NPP_Print(NPP instance, NPPrint* platformPrint)
        {
            return HACK_target->NPP_Print(instance, platformPrint);
        }
        static int16_t NPP_HandleEvent(NPP instance, void* event)
        {
            return HACK_target->NPP_HandleEvent(instance, event);
        }
        static void NPP_URLNotify(NPP instance, const char* url,
                                  NPReason reason, void* notifyData)
        {
            return HACK_target->NPP_URLNotify(instance, url, reason,
                                              notifyData);
        }
        static NPError NPP_GetValue(NPP instance,
                                    NPPVariable variable, void *ret_value)
        {
            return HACK_target->NPP_GetValue(instance, variable, ret_value);
        }
        static NPError NPP_SetValue(NPP instance,
                                    NPNVariable variable, void *value)
        {
            return HACK_target->NPP_SetValue(instance, variable, value);
        }

        static NPAPIPluginParent* HACK_target;
        friend class NPAPIPluginParent;
    };

    friend class Shim;
    Shim* mShim;
};

} 
} 

#endif  
