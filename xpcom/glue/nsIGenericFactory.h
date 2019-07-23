





































#ifndef nsIGenericFactory_h___
#define nsIGenericFactory_h___

#include "nsIFactory.h"
#include "nsIModule.h"
#include "nsIClassInfo.h"

class nsIFile;
class nsIComponentManager;


#define NS_GENERICFACTORY_CID                                                 \
  { 0x3bc97f01, 0xccdf, 0x11d2,                                               \
    { 0xba, 0xb8, 0xb5, 0x48, 0x65, 0x44, 0x61, 0xfc } }


#define NS_IGENERICFACTORY_IID                                                \
  { 0x3bc97f00, 0xccdf, 0x11d2,                                               \
    { 0xba, 0xb8, 0xb5, 0x48, 0x65, 0x44, 0x61, 0xfc } }

#define NS_GENERICFACTORY_CONTRACTID "@mozilla.org/generic-factory;1"
#define NS_GENERICFACTORY_CLASSNAME "Generic Factory"

struct nsModuleComponentInfo; 





class nsIGenericFactory : public nsIFactory {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGENERICFACTORY_IID)
    
    NS_IMETHOD SetComponentInfo(const nsModuleComponentInfo *info) = 0;
    NS_IMETHOD GetComponentInfo(const nsModuleComponentInfo **infop) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGenericFactory, NS_IGENERICFACTORY_IID)

NS_COM_GLUE nsresult
NS_NewGenericFactory(nsIGenericFactory **result,
                     const nsModuleComponentInfo *info);




 



















typedef NS_CALLBACK(NSConstructorProcPtr)(nsISupports *aOuter, 
                                          REFNSIID aIID,
                                          void **aResult);

























typedef NS_CALLBACK(NSRegisterSelfProcPtr)(nsIComponentManager *aCompMgr,
                                           nsIFile *aPath,
                                           const char *aLoaderStr,
                                           const char *aType,
                                           const nsModuleComponentInfo *aInfo);






















typedef NS_CALLBACK(NSUnregisterSelfProcPtr)(nsIComponentManager *aCompMgr,
                                             nsIFile *aPath,
                                             const char *aLoaderStr,
                                             const nsModuleComponentInfo *aInfo);






 
typedef NS_CALLBACK(NSFactoryDestructorProcPtr)(void);
















typedef NS_CALLBACK(NSGetInterfacesProcPtr)(PRUint32 *countp,
                                            nsIID* **array);

















typedef NS_CALLBACK(NSGetLanguageHelperProcPtr)(PRUint32 language,
                                                nsISupports **helper);
























struct nsModuleComponentInfo {
    const char*                                 mDescription;
    nsCID                                       mCID;
    const char*                                 mContractID;
    NSConstructorProcPtr                        mConstructor;
    NSRegisterSelfProcPtr                       mRegisterSelfProc;
    NSUnregisterSelfProcPtr                     mUnregisterSelfProc;
    NSFactoryDestructorProcPtr                  mFactoryDestructor;
    NSGetInterfacesProcPtr                      mGetInterfacesProc;
    NSGetLanguageHelperProcPtr                  mGetLanguageHelperProc;
    nsIClassInfo **                             mClassInfoGlobal;
    PRUint32                                    mFlags;
};















typedef nsresult (PR_CALLBACK *nsModuleConstructorProc) (nsIModule *self);









typedef void (PR_CALLBACK *nsModuleDestructorProc) (nsIModule *self);

















struct nsModuleInfo {
    PRUint32                mVersion;
    const char*             mModuleName;
    const nsModuleComponentInfo *mComponents;
    PRUint32                mCount;
    nsModuleConstructorProc mCtor;
    nsModuleDestructorProc  mDtor;
};






#define NS_MODULEINFO_VERSION 0x00015000UL // 1.5





NS_COM_GLUE nsresult
NS_NewGenericModule2(nsModuleInfo const *info, nsIModule* *result);




NS_COM_GLUE nsresult
NS_NewGenericModule(const char* moduleName,
                    PRUint32 componentCount,
                    nsModuleComponentInfo* components,
                    nsModuleDestructorProc dtor,
                    nsIModule* *result);

#if defined(XPCOM_TRANSLATE_NSGM_ENTRY_POINT)
#  define NSGETMODULE_ENTRY_POINT(_name)  NS_VISIBILITY_HIDDEN nsresult _name##_NSGetModule
#else
#  define NSGETMODULE_ENTRY_POINT(_name)  extern "C" NS_EXPORT nsresult NSGetModule
#endif







#define NS_IMPL_NSGETMODULE(_name, _components)                               \
    NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(_name, _components, nsnull, nsnull)

#define NS_IMPL_NSGETMODULE_WITH_CTOR(_name, _components, _ctor)              \
    NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(_name, _components, _ctor, nsnull)

#define NS_IMPL_NSGETMODULE_WITH_DTOR(_name, _components, _dtor)              \
    NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(_name, _components, nsnull, _dtor)

#define NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(_name, _components, _ctor, _dtor)  \
static nsModuleInfo const kModuleInfo = {                                     \
    NS_MODULEINFO_VERSION,                                                    \
    (#_name),                                                                 \
    (_components),                                                            \
    (sizeof(_components) / sizeof(_components[0])),                           \
    (_ctor),                                                                  \
    (_dtor)                                                                   \
};                                                                            \
NSGETMODULE_ENTRY_POINT(_name)                                                \
(nsIComponentManager *servMgr,                                                \
            nsIFile* location,                                                \
            nsIModule** result)                                               \
{                                                                             \
    return NS_NewGenericModule2(&kModuleInfo, result);                        \
}



#define NS_GENERIC_FACTORY_CONSTRUCTOR(_InstanceClass)                        \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    NS_ADDREF(inst);                                                          \
    rv = inst->QueryInterface(aIID, aResult);                                 \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \


#define NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(_InstanceClass, _InitMethod)      \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    NS_ADDREF(inst);                                                          \
    rv = inst->_InitMethod();                                                 \
    if(NS_SUCCEEDED(rv)) {                                                    \
        rv = inst->QueryInterface(aIID, aResult);                             \
    }                                                                         \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \



#define NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(_InstanceClass, _GetterProc) \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    inst = _GetterProc();                                                     \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    /* NS_ADDREF(inst); */                                                    \
    rv = inst->QueryInterface(aIID, aResult);                                 \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \

#endif 
