







































#ifndef dom_plugins_NPAPIPluginChild_h
#define dom_plugins_NPAPIPluginChild_h 1

#include <string>
#include <vector>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"
#include "nsplugindefs.h"

#include "nsAutoPtr.h"

#include "mozilla/plugins/NPAPIProtocolChild.h"
#include "mozilla/plugins/NPPInstanceChild.h"









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
#define _MOZ_LOG(s)  printf("[NPAPIPluginChild] %s\n", s)

namespace mozilla {
namespace plugins {


class NPAPIPluginChild : public NPAPIProtocolChild
{
protected:
    
    virtual nsresult AnswerNP_Initialize(NPError* rv);

    virtual NPPProtocolChild* NPPConstructor(
        const String& aMimeType,
        const uint16_t& aMode,
        const StringArray& aNames,
        const StringArray& aValues,
        NPError* rv);

    virtual nsresult NPPDestructor(
        NPPProtocolChild* actor,
        NPError* rv);

public:
    NPAPIPluginChild();
    virtual ~NPAPIPluginChild();

    bool Init(const std::string& aPluginFilename,
              MessageLoop* aIOLoop,
              IPC::Channel* aChannel);

    void CleanUp();

    static const NPNetscapeFuncs sBrowserFuncs;

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
};

} 
} 

#endif  
