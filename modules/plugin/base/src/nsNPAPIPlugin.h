




































#ifndef nsNPAPIPlugin_h_
#define nsNPAPIPlugin_h_

#include "nsIPlugin.h"
#include "prlink.h"
#include "npfunctions.h"
#include "nsPluginHost.h"

#include "jsapi.h"

#include "mozilla/PluginLibrary.h"







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
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) ();

class nsNPAPIPlugin : public nsIPlugin
{
private:
  typedef mozilla::PluginLibrary PluginLibrary;

public:
  nsNPAPIPlugin();
  virtual ~nsNPAPIPlugin();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGIN

  
  
  static nsresult CreatePlugin(const char* aFilePath, PRLibrary* aLibrary,
                               nsIPlugin** aResult);

#if defined(XP_MACOSX) && !defined(__LP64__)
  void SetPluginRefNum(short aRefNum);
#endif

#ifdef MOZ_IPC
  
  
  
  
  void PluginCrashed(const nsAString& pluginDumpID,
                     const nsAString& browserDumpID);
#endif

protected:

#if defined(XP_MACOSX) && !defined(__LP64__)
  short mPluginRefNum;
#endif

  NPPluginFuncs mPluginFuncs;
  PluginLibrary* mLibrary;
};

namespace mozilla {
namespace plugins {
namespace parent {







#if JS_BITS_PER_WORD == 64

JS_STATIC_ASSERT(sizeof(NPIdentifier) == sizeof(jsval));

static inline bool
NPIdentifierIsString(NPIdentifer id)
{
    return JSVAL_IS_STRING((jsval)id);
}

static inline JSString *
NPIdentifierToString(NPIdentifer id)
{
    return JSVAL_TO_STRING((jsval)id);
}

static inline NPIdentifier
StringToNPIdentifier(JSString *str)
{
    return (NPIdentifier)STRING_TO_JSVAL(str);
}

static inline bool
NPIdentifierIsInt(NPIdentifer id)
{
    return JSVAL_IS_INT((jsval)id);
}

static inline jsint
NPIdentifierToInt(NPIdentifer id)
{
    return JSVAL_TO_INT((jsval)id);
}

static inline bool
IntToNPIdentifier(JSContext *, jsint i, NPIdentifier *pid)
{
    *pid = (NPIdentifier)INT_TO_JSVAL(i);
    return true;
}

static inline bool
NPIdentifierIsVoid(NPIdentifier id)
{
    return JSVAL_IS_VOID((NPIdentifier)id);
}

static const NPIdentifier NPIdentifier_VOID = (NPIdentifier)JSVAL_VOID;

#else  

static inline bool
NPIdentifierIsString(NPIdentifier id)
{
    return ((size_t)id & 0x3) == 0;
}

static inline JSString *
NPIdentifierToString(NPIdentifier id)
{
    NS_ASSERTION(NPIdentifierIsString(id), "id must be string");
    return (JSString *)id;
}

static inline NPIdentifier
StringToNPIdentifier(JSString *str)
{
    NS_ASSERTION(((size_t)str & 3) == 0, "Strings are assumed to be at least 4-byte aligned");
    return (NPIdentifier)str;
}

static inline bool
NPIdentifierIsInt(NPIdentifier id)
{
    return ((size_t)id & 1) != 0;
}

static inline jsint
NPIdentifierToInt(NPIdentifier id)
{
    NS_ASSERTION(NPIdentifierIsInt(id), "id must be int");
    return (jsint)id >> 1;
}

static inline NPIdentifier
IntToNPIdentifier(jsint i)
{
    NS_ASSERTION(i < (1 << 30) - 1, "integer id is too big, will be truncated");
    return (NPIdentifier)((i << 1) | 0x1);
}

static inline bool
NPIdentifierIsVoid(NPIdentifier id)
{
    return (size_t)id == 0x2;
}

static const NPIdentifier NPIdentifier_VOID = (NPIdentifier)0x2;

#endif

static inline jsval
NPIdentifierToJSVal(NPIdentifier id)
{
    if (NPIdentifierIsString(id))
        return STRING_TO_JSVAL(NPIdentifierToString(id));
    return INT_TO_JSVAL(NPIdentifierToInt(id));
}

static inline NPIdentifier
JSValToNPIdentifier(jsval val)
{
    if (JSVAL_IS_STRING(val))
        return StringToNPIdentifier(JSVAL_TO_STRING(val));
    return IntToNPIdentifier(JSVAL_TO_INT(val));
}

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

typedef void(*PluginTimerFunc)(NPP npp, uint32_t timerID);

uint32_t NP_CALLBACK
_scheduletimer(NPP instance, uint32_t interval, NPBool repeat, PluginTimerFunc timerFunc);

void NP_CALLBACK
_unscheduletimer(NPP instance, uint32_t timerID);

NPError NP_CALLBACK
_popupcontextmenu(NPP instance, NPMenu* menu);

NPBool NP_CALLBACK
_convertpoint(NPP instance, double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double *destX, double *destY, NPCoordinateSpace destSpace);

NPError NP_CALLBACK
_requestread(NPStream *pstream, NPByteRange *rangeList);

NPError NP_CALLBACK
_geturlnotify(NPP npp, const char* relativeURL, const char* target,
              void* notifyData);

NPError NP_CALLBACK
_getvalue(NPP npp, NPNVariable variable, void *r_value);

NPError NP_CALLBACK
_setvalue(NPP npp, NPPVariable variable, void *r_value);

NPError NP_CALLBACK
_geturl(NPP npp, const char* relativeURL, const char* target);

NPError NP_CALLBACK
_posturlnotify(NPP npp, const char* relativeURL, const char *target,
               uint32_t len, const char *buf, NPBool file, void* notifyData);

NPError NP_CALLBACK
_posturl(NPP npp, const char* relativeURL, const char *target, uint32_t len,
            const char *buf, NPBool file);

NPError NP_CALLBACK
_newstream(NPP npp, NPMIMEType type, const char* window, NPStream** pstream);

int32_t NP_CALLBACK
_write(NPP npp, NPStream *pstream, int32_t len, void *buffer);

NPError NP_CALLBACK
_destroystream(NPP npp, NPStream *pstream, NPError reason);

void NP_CALLBACK
_status(NPP npp, const char *message);

void NP_CALLBACK
_memfree (void *ptr);

uint32_t NP_CALLBACK
_memflush(uint32_t size);

void NP_CALLBACK
_reloadplugins(NPBool reloadPages);

void NP_CALLBACK
_invalidaterect(NPP npp, NPRect *invalidRect);

void NP_CALLBACK
_invalidateregion(NPP npp, NPRegion invalidRegion);

void NP_CALLBACK
_forceredraw(NPP npp);

const char* NP_CALLBACK
_useragent(NPP npp);

void* NP_CALLBACK
_memalloc (uint32_t size);


void* NP_CALLBACK 
_getJavaEnv();

void* NP_CALLBACK 
_getJavaPeer(NPP npp);

} 
} 
} 

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
