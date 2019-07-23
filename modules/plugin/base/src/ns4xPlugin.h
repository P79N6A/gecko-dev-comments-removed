




































#ifndef ns4xPlugin_h__
#define ns4xPlugin_h__

#include "nsIFactory.h"
#include "nsIPlugin.h"
#include "nsIPluginInstancePeer.h"
#include "nsIWindowlessPlugInstPeer.h"
#include "prlink.h"  
#include "npupp.h"
#include "nsPluginHostImpl.h"












#ifdef XP_OS2
#define NP_CALLBACK _System
#else
#define NP_CALLBACK
#endif

#if defined(XP_WIN)
#define NS_4XPLUGIN_CALLBACK(_type, _name) _type (__stdcall * _name)
#elif defined(XP_OS2)
#define NS_4XPLUGIN_CALLBACK(_type, _name) _type (_System * _name)
#else
#define NS_4XPLUGIN_CALLBACK(_type, _name) _type (* _name)
#endif





#if defined(XP_WIN) || defined(XP_UNIX) || defined(XP_BEOS) || defined(XP_OS2)
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_GETENTRYPOINTS) (NPPluginFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGININIT) (const NPNetscapeFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGINUNIXINIT) (const NPNetscapeFuncs* pCallbacks,NPPluginFuncs* fCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) (void);
#endif

#ifdef XP_MACOSX
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) (void);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_MAIN) (NPNetscapeFuncs* nCallbacks, NPPluginFuncs* pCallbacks, NPP_ShutdownUPP* unloadUpp);








#ifndef __POWERPC__
#define MACOSX_GETENTRYPOINT_SUPPORT 1
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_GETENTRYPOINTS) (NPPluginFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGININIT) (const NPNetscapeFuncs* pCallbacks);
#endif

#endif

class nsIServiceManagerObsolete;
class nsIMemory;







class ns4xPlugin : public nsIPlugin
{
public:
  ns4xPlugin(NPPluginFuncs* callbacks, PRLibrary* aLibrary,
             NP_PLUGINSHUTDOWN aShutdown,
             nsIServiceManagerObsolete* serviceMgr);
  virtual ~ns4xPlugin(void);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY
  NS_DECL_NSIPLUGIN
  
  
  

  




   
  static nsresult
  CreatePlugin(nsIServiceManagerObsolete* aServiceMgr,
               const char* aFileName,
               const char* aFullPath,
               PRLibrary* aLibrary,
               nsIPlugin** aResult);

#ifdef XP_MACOSX
  void SetPluginRefNum(short aRefNum);
#endif

protected:
  


  static void CheckClassInitialized(void);


#ifdef XP_MACOSX
  short fPluginRefNum;
#ifdef MACOSX_GETENTRYPOINT_SUPPORT
  PRBool usesGetEntryPoints;
#endif
#endif

  



  NPPluginFuncs fCallbacks;
  PRLibrary*    fLibrary;

  NP_PLUGINSHUTDOWN fShutdownEntry;

  


  static NPNetscapeFuncs CALLBACKS;
};


PR_BEGIN_EXTERN_C
NPObject* NP_CALLBACK
_getwindowobject(NPP npp);

NPObject* NP_CALLBACK
_getpluginelement(NPP npp);

NPIdentifier NP_CALLBACK
_getstringidentifier(const NPUTF8* name);

void NP_CALLBACK
_getstringidentifiers(const NPUTF8** names, int32_t nameCount,
                      NPIdentifier *identifiers);

bool NP_CALLBACK
_identifierisstring(NPIdentifier identifiers);

NPIdentifier NP_CALLBACK
_getintidentifier(int32_t intid);

NPUTF8* NP_CALLBACK
_utf8fromidentifier(NPIdentifier identifier);

int32_t NP_CALLBACK
_intfromidentifier(NPIdentifier identifier);

NPObject* NP_CALLBACK
_createobject(NPP npp, NPClass* aClass);

NPObject* NP_CALLBACK
_retainobject(NPObject* npobj);

void NP_CALLBACK
_releaseobject(NPObject* npobj);

bool NP_CALLBACK
_invoke(NPP npp, NPObject* npobj, NPIdentifier method, const NPVariant *args,
        uint32_t argCount, NPVariant *result);

bool NP_CALLBACK
_invokeDefault(NPP npp, NPObject* npobj, const NPVariant *args,
               uint32_t argCount, NPVariant *result);

bool NP_CALLBACK
_evaluate(NPP npp, NPObject* npobj, NPString *script, NPVariant *result);

bool NP_CALLBACK
_getproperty(NPP npp, NPObject* npobj, NPIdentifier property,
             NPVariant *result);

bool NP_CALLBACK
_setproperty(NPP npp, NPObject* npobj, NPIdentifier property,
             const NPVariant *value);

bool NP_CALLBACK
_removeproperty(NPP npp, NPObject* npobj, NPIdentifier property);

bool NP_CALLBACK
_hasproperty(NPP npp, NPObject* npobj, NPIdentifier propertyName);

bool NP_CALLBACK
_hasmethod(NPP npp, NPObject* npobj, NPIdentifier methodName);

bool NP_CALLBACK
_enumerate(NPP npp, NPObject *npobj, NPIdentifier **identifier,
           uint32_t *count);

void NP_CALLBACK
_releasevariantvalue(NPVariant *variant);

void NP_CALLBACK
_setexception(NPObject* npobj, const NPUTF8 *message);

PR_END_EXTERN_C

const char *
PeekException();

void
PopException();

void
OnPluginDestroy(NPP instance);

void
OnShutdown();

void
EnterAsyncPluginThreadCallLock();
void
ExitAsyncPluginThreadCallLock();

class NPPStack
{
public:
  static NPP Peek()
  {
    return sCurrentNPP;
  }

protected:
  static NPP sCurrentNPP;
};

class NPPAutoPusher : public NPPStack
{
public:
  NPPAutoPusher(NPP npp)
    : mOldNPP(sCurrentNPP)
  {
    NS_ASSERTION(npp, "Uh, null npp passed to NPPAutoPusher!");

    sCurrentNPP = npp;
  }

  ~NPPAutoPusher()
  {
    sCurrentNPP = mOldNPP;
  }

private:
  NPP mOldNPP;
};

class NPPExceptionAutoHolder
{
public:
  NPPExceptionAutoHolder();
  ~NPPExceptionAutoHolder();

protected:
  char *mOldException;
};

#endif 
