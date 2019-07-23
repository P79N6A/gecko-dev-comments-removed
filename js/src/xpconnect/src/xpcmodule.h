







































#ifndef xpcmodule_h___
#define xpcmodule_h___

#include "xpcprivate.h"
#ifdef MOZ_JSLOADER
#include "mozJSLoaderConstructors.h"
#endif




#define XPCVARIANT_CID                                                        \
    {0xdc524540, 0x487e, 0x4501,                                              \
      { 0x9a, 0xc7, 0xaa, 0xa7, 0x84, 0xb1, 0x7c, 0x1c } }

#define XPCVARIANT_CONTRACTID "@mozilla.org/xpcvariant;1"
#define XPC_JSCONTEXT_STACK_ITERATOR_CONTRACTID                               \
    "@mozilla.org/js/xpc/ContextStackIterator;1"


#define SCRIPTABLE_INTERFACES_CID                                             \
    {0xfe4f7592, 0xc1fc, 0x4662,                                              \
      { 0xac, 0x83, 0x53, 0x88, 0x41, 0x31, 0x88, 0x3 } }


#define XPCONNECT_GENERAL_FACTORIES                                           \
  NS_DECL_CLASSINFO(XPCVariant)                                               \
  NS_DECL_CLASSINFO(nsXPCException)                                           \
                                                                              \
  NS_GENERIC_FACTORY_CONSTRUCTOR(nsJSID)                                      \
  NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCException)                              \
  NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCJSContextStackIterator)                 \
  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIXPConnect,                      \
                                           nsXPConnect::GetSingleton)         \
  NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptError)                               \
  NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCComponents_Interfaces)


#ifdef XPC_IDISPATCH_SUPPORT

#define XPCONNECT_FACTORIES                                                   \
  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIDispatchSupport,                \
                                           nsDispatchSupport::GetSingleton)   \
  XPCONNECT_GENERAL_FACTORIES

#else

#define XPCONNECT_FACTORIES XPCONNECT_GENERAL_FACTORIES

#endif 


#ifdef XPCONNECT_STANDALONE
#define NO_SUBSCRIPT_LOADER
#endif


#define XPCONNECT_GENERAL_COMPONENTS                                          \
  {                                                                           \
    nsnull,                                                                   \
    NS_JS_ID_CID,                                                             \
    XPC_ID_CONTRACTID,                                                        \
    nsJSIDConstructor                                                         \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    NS_XPCONNECT_CID,                                                         \
    XPC_XPCONNECT_CONTRACTID,                                                 \
    nsIXPConnectConstructor                                                   \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    NS_XPC_THREAD_JSCONTEXT_STACK_CID,                                        \
    XPC_CONTEXT_STACK_CONTRACTID,                                             \
    nsIXPConnectConstructor                                                   \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    NS_XPCEXCEPTION_CID,                                                      \
    XPC_EXCEPTION_CONTRACTID,                                                 \
    nsXPCExceptionConstructor,                                                \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    NS_CI_INTERFACE_GETTER_NAME(nsXPCException),                              \
    nsnull,                                                                   \
    &NS_CLASSINFO_NAME(nsXPCException),                                       \
    nsIClassInfo::DOM_OBJECT                                                  \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    NS_JS_RUNTIME_SERVICE_CID,                                                \
    XPC_RUNTIME_CONTRACTID,                                                   \
    nsIXPConnectConstructor                                                   \
  },                                                                          \
  {                                                                           \
    NS_SCRIPTERROR_CLASSNAME,                                                 \
    NS_SCRIPTERROR_CID,                                                       \
    NS_SCRIPTERROR_CONTRACTID,                                                \
    nsScriptErrorConstructor                                                  \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    SCRIPTABLE_INTERFACES_CID,                                                \
    NS_SCRIPTABLE_INTERFACES_CONTRACTID,                                      \
    nsXPCComponents_InterfacesConstructor,                                    \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsIClassInfo::THREADSAFE                                                  \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    XPCVARIANT_CID,                                                           \
    XPCVARIANT_CONTRACTID,                                                    \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    NS_CI_INTERFACE_GETTER_NAME(XPCVariant),                                  \
    nsnull,                                                                   \
    &NS_CLASSINFO_NAME(XPCVariant)                                            \
  },                                                                          \
  {                                                                           \
    nsnull,                                                                   \
    NS_XPC_JSCONTEXT_STACK_ITERATOR_CID,                                      \
    XPC_JSCONTEXT_STACK_ITERATOR_CONTRACTID,                                  \
    nsXPCJSContextStackIteratorConstructor                                    \
  }


#ifdef MOZ_JSLOADER

#define XPCONNECT_LOADER_COMPONENTS                                           \
  {                                                                           \
    "JS component loader",                                                    \
    MOZJSCOMPONENTLOADER_CID,                                                 \
    MOZJSCOMPONENTLOADER_CONTRACTID,                                          \
    mozJSComponentLoaderConstructor,                                          \
    RegisterJSLoader,                                                         \
    UnregisterJSLoader                                                        \
  },                                                                          \
  XPCONNECT_SUBSCRIPT_LOADER_COMPONENTS

#ifdef NO_SUBSCRIPT_LOADER

#define XPCONNECT_SUBSCRIPT_LOADER_COMPONENTS

#else

#define XPCONNECT_SUBSCRIPT_LOADER_COMPONENTS                                 \
  {                                                                           \
    "JS subscript loader",                                                    \
    MOZ_JSSUBSCRIPTLOADER_CID,                                                \
    mozJSSubScriptLoadContractID,                                             \
    mozJSSubScriptLoaderConstructor                                           \
  },

#endif 

#else

#define XPCONNECT_LOADER_COMPONENTS

#endif 


#ifdef XPC_IDISPATCH_SUPPORT

#define XPCONNECT_IDISPATCH_COMPONENTS                                        \
  {                                                                           \
    nsnull,                                                                   \
    NS_IDISPATCH_SUPPORT_CID,                                                 \
    NS_IDISPATCH_SUPPORT_CONTRACTID,                                          \
    nsIDispatchSupportConstructor                                             \
  },

#else

#define XPCONNECT_IDISPATCH_COMPONENTS

#endif 


#define XPCONNECT_COMPONENTS                                                  \
  XPCONNECT_LOADER_COMPONENTS                                                 \
  XPCONNECT_IDISPATCH_COMPONENTS                                              \
  XPCONNECT_GENERAL_COMPONENTS

extern nsresult xpcModuleCtor(nsIModule* self);
extern void xpcModuleDtor(nsIModule*);

#endif 

