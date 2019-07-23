




































#ifndef nsNPAPIPlugin_h_
#define nsNPAPIPlugin_h_

#include "nsIPlugin.h"
#include "prlink.h"
#include "npfunctions.h"
#include "nsPluginHost.h"

#include "mozilla/SharedLibrary.h"







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

class nsNPAPIPlugin : public nsIPlugin
{
private:
  typedef mozilla::SharedLibrary SharedLibrary;

public:
  
  
  
  nsNPAPIPlugin(NPPluginFuncs* callbacks,
                SharedLibrary* aLibrary,
                PRLibrary* aPRLibrary,
                NP_PLUGINSHUTDOWN aShutdown);
  virtual ~nsNPAPIPlugin();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGIN

  
  
  static nsresult CreatePlugin(const char* aFilePath, PRLibrary* aLibrary,
                               nsIPlugin** aResult);
#ifdef XP_MACOSX
  void SetPluginRefNum(short aRefNum);
#endif

protected:
  
  static void CheckClassInitialized(void);

#ifdef XP_MACOSX
  short fPluginRefNum;
#endif

  
  
  NPPluginFuncs fCallbacks;
  SharedLibrary* fLibrary;
  PRLibrary* fPRLibrary;

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

bool NP_CALLBACK
_construct(NPP npp, NPObject* npobj, const NPVariant *args,
           uint32_t argCount, NPVariant *result);

void NP_CALLBACK
_releasevariantvalue(NPVariant *variant);

void NP_CALLBACK
_setexception(NPObject* npobj, const NPUTF8 *message);

void NP_CALLBACK
_pushpopupsenabledstate(NPP npp, NPBool enabled);

void NP_CALLBACK
_poppopupsenabledstate(NPP npp);

typedef void(*PluginThreadCallback)(void *);
void NP_CALLBACK
_pluginthreadasynccall(NPP instance, PluginThreadCallback func,
                       void *userData);

NPError NP_CALLBACK
_getvalueforurl(NPP instance, NPNURLVariable variable, const char *url,
                char **value, uint32_t *len);
NPError NP_CALLBACK
_setvalueforurl(NPP instance, NPNURLVariable variable, const char *url,
                const char *value, uint32_t len);

NPError NP_CALLBACK
_getauthenticationinfo(NPP instance, const char *protocol, const char *host,
                       int32_t port, const char *scheme, const char *realm,
                       char **username, uint32_t *ulen, char **password,
                       uint32_t *plen);

uint32_t NP_CALLBACK
_scheduletimer(NPP instance, uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID));

void NP_CALLBACK
_unscheduletimer(NPP instance, uint32_t timerID);

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










class NPPAutoPusher : public NPPStack,
                      protected PluginDestructionGuard
{
public:
  NPPAutoPusher(NPP npp)
    : PluginDestructionGuard(npp),
      mOldNPP(sCurrentNPP)
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
