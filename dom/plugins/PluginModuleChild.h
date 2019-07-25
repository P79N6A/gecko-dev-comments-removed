






































#ifndef dom_plugins_PluginModuleChild_h
#define dom_plugins_PluginModuleChild_h 1

#include <string>
#include <vector>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"

#include "nsAutoPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

#include "mozilla/plugins/PPluginModuleChild.h"
#include "mozilla/plugins/PluginInstanceChild.h"









#ifdef XP_OS2
#define NP_CALLBACK _System
#else
#define NP_CALLBACK
#endif

#if defined(XP_WIN)
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (__stdcall * _name)
#elif defined(XP_OS2)
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (_System * _name)
#else
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (* _name)
#endif

typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_GETENTRYPOINTS) (NPPluginFuncs* pCallbacks);
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGININIT) (const NPNetscapeFuncs* pCallbacks);
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGINUNIXINIT) (const NPNetscapeFuncs* pCallbacks, NPPluginFuncs* fCallbacks);
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) (void);

namespace mozilla {
namespace plugins {

class PluginScriptableObjectChild;
class PluginInstanceChild;

class PluginModuleChild : public PPluginModuleChild
{
protected:
    
    virtual bool AnswerNP_Initialize(NPError* rv);

    virtual PPluginInstanceChild*
    AllocPPluginInstance(const nsCString& aMimeType,
                         const uint16_t& aMode,
                         const nsTArray<nsCString>& aNames,
                         const nsTArray<nsCString>& aValues,
                         NPError* rv);

    virtual bool
    DeallocPPluginInstance(PPluginInstanceChild* aActor);

    virtual bool
    AnswerPPluginInstanceConstructor(PPluginInstanceChild* aActor,
                                     const nsCString& aMimeType,
                                     const uint16_t& aMode,
                                     const nsTArray<nsCString>& aNames,
                                     const nsTArray<nsCString>& aValues,
                                     NPError* rv);
    virtual bool
    AnswerNP_Shutdown(NPError *rv);

    virtual void
    ActorDestroy(ActorDestroyReason why);

public:
    PluginModuleChild();
    virtual ~PluginModuleChild();

    bool Init(const std::string& aPluginFilename,
              base::ProcessHandle aParentProcessHandle,
              MessageLoop* aIOLoop,
              IPC::Channel* aChannel);

    void CleanUp();

    const char* GetUserAgent();

    static const NPNetscapeFuncs sBrowserFuncs;

    static PluginModuleChild* current();

    bool RegisterActorForNPObject(NPObject* aObject,
                                  PluginScriptableObjectChild* aActor);

    void UnregisterActorForNPObject(NPObject* aObject);

    PluginScriptableObjectChild* GetActorForNPObject(NPObject* aObject);

#ifdef DEBUG
    bool NPObjectIsRegistered(NPObject* aObject);
#endif

    


    static NPObject* NP_CALLBACK NPN_CreateObject(NPP aNPP, NPClass* aClass);
    


    static NPObject* NP_CALLBACK NPN_RetainObject(NPObject* aNPObj);
    


    static void NP_CALLBACK NPN_ReleaseObject(NPObject* aNPObj);

private:
    bool InitGraphics();
#if defined(MOZ_WIDGET_GTK2)
    static gboolean DetectNestedEventLoop(gpointer data);
    static gboolean ProcessBrowserEvents(gpointer data);

    NS_OVERRIDE
    virtual void EnteredCxxStack();
    NS_OVERRIDE
    virtual void ExitedCxxStack();
#endif

    std::string mPluginFilename;
    PRLibrary* mLibrary;
    nsCString mUserAgent;

    
#ifdef OS_POSIX
    NP_PLUGINUNIXINIT mInitializeFunc;
#elif OS_WIN
    NP_PLUGININIT mInitializeFunc;
    NP_GETENTRYPOINTS mGetEntryPointsFunc;
#endif

    NP_PLUGINSHUTDOWN mShutdownFunc;
    NPPluginFuncs mFunctions;
    NPSavedData mSavedData;

#if defined(MOZ_WIDGET_GTK2)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    guint mNestedLoopTimerId;
#endif

    struct NPObjectData : public nsPtrHashKey<NPObject>
    {
        NPObjectData(const NPObject* key)
            : nsPtrHashKey<NPObject>(key)
            , instance(NULL)
            , actor(NULL)
        { }

        
        PluginInstanceChild* instance;

        
        PluginScriptableObjectChild* actor;
    };
    



    nsTHashtable<NPObjectData> mObjectMap;

public: 
    



    static void DeallocNPObject(NPObject* o);

    NPError NPP_Destroy(PluginInstanceChild* instance) {
        return mFunctions.destroy(instance->GetNPP(), 0);
    }

    



    void FindNPObjectsForInstance(PluginInstanceChild* instance);

private:
    static PLDHashOperator CollectForInstance(NPObjectData* d, void* userArg);
};

} 
} 

#endif  
