






































#ifndef dom_plugins_PluginModuleChild_h
#define dom_plugins_PluginModuleChild_h 1

#include <string>
#include <vector>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"

#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
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
#ifdef XP_MACOSX
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_MAIN) (NPNetscapeFuncs* nCallbacks, NPPluginFuncs* pCallbacks, NPP_ShutdownProcPtr* unloadProcPtr);
#endif

#undef _MOZ_LOG
#define _MOZ_LOG(s)  printf("[PluginModuleChild] %s\n", s)

namespace mozilla {
namespace plugins {

class PluginScriptableObjectChild;

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
    DeallocPPluginInstance(PPluginInstanceChild* aActor,
                           NPError* rv);

    virtual bool
    AnswerPPluginInstanceDestructor(PPluginInstanceChild* aActor,
                                    NPError* rv);

    virtual bool
    AnswerPPluginInstanceConstructor(PPluginInstanceChild* aActor,
                                     const nsCString& aMimeType,
                                     const uint16_t& aMode,
                                     const nsTArray<nsCString>& aNames,
                                     const nsTArray<nsCString>& aValues,
                                     NPError* rv);

public:
    PluginModuleChild();
    virtual ~PluginModuleChild();

    bool Init(const std::string& aPluginFilename,
              base::ProcessHandle aParentProcessHandle,
              MessageLoop* aIOLoop,
              IPC::Channel* aChannel);

    void CleanUp();

    static const NPNetscapeFuncs sBrowserFuncs;

    static PluginModuleChild* current();

    bool RegisterNPObject(NPObject* aObject,
                          PluginScriptableObjectChild* aActor);

    void UnregisterNPObject(NPObject* aObject);

    PluginScriptableObjectChild* GetActorForNPObject(NPObject* aObject);

private:
    bool InitGraphics();

    std::string mPluginFilename;
    PRLibrary* mLibrary;

    
#ifdef OS_LINUX
    NP_PLUGINUNIXINIT mInitializeFunc;
#elif OS_WIN
    NP_PLUGININIT mInitializeFunc;
    NP_GETENTRYPOINTS mGetEntryPointsFunc;
#endif
    NP_PLUGINSHUTDOWN mShutdownFunc;
    NPPluginFuncs mFunctions;
    NPSavedData mSavedData;

    nsDataHashtable<nsVoidPtrHashKey, PluginScriptableObjectChild*> mObjectMap;
};

} 
} 

#endif  
