









































#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsSanePlugin_CID.h"
#include "nsSanePlugin.h"
#include "nsSanePluginFactory.h"
#include "plstr.h"

#define PLUGIN_NAME             "SANE Plugin"
#define PLUGIN_DESCRIPTION      "SANE Plugin is a generic scanner interface"
#define PLUGIN_MIME_DESCRIPTION "application/X-sane-plugin::Scanner/Camera"
#define PLUGIN_MIME_TYPE        "application/X-sane-plugin"

static NS_DEFINE_IID( kISupportsIID,            NS_ISUPPORTS_IID           );
static NS_DEFINE_IID( kIFactoryIID,             NS_IFACTORY_IID            );
static NS_DEFINE_IID( kIPluginIID,              NS_IPLUGIN_IID             );
static NS_DEFINE_CID( kComponentManagerCID,     NS_COMPONENTMANAGER_CID    );
static NS_DEFINE_CID( knsSanePluginControlCID,  NS_SANE_PLUGIN_CONTROL_CID );
static NS_DEFINE_CID( knsSanePluginInst,        NS_SANE_PLUGIN_CID         );



nsSanePluginFactoryImpl::nsSanePluginFactoryImpl( const nsCID &aClass,
                                                  const char* className,
                                                  const char* contractID )
    : mClassID(aClass), mClassName(className), mContractID(contractID)
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::nsSanePluginFactoryImpl()\n");
#endif
}

nsSanePluginFactoryImpl::~nsSanePluginFactoryImpl()
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::~nsSanePluginFactoryImpl()\n");
#endif

    printf("mRefCnt = %i\n", mRefCnt);
    NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
}

NS_IMETHODIMP
nsSanePluginFactoryImpl::QueryInterface(const nsIID &aIID, 
                                        void **aInstancePtr)
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::QueryInterface()\n");
#endif

    if (!aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = static_cast<nsISupports*>(this);
    } else if (aIID.Equals(kIFactoryIID)) {
        *aInstancePtr = static_cast<nsISupports*>
                                   (static_cast<nsIFactory*>(this));
    } else if (aIID.Equals(kIPluginIID)) {
        *aInstancePtr = static_cast<nsISupports*>
                                   (static_cast<nsIPlugin*>(this));
    } else {
        *aInstancePtr = nsnull;
        return NS_ERROR_NO_INTERFACE;
    }

    NS_ADDREF(reinterpret_cast<nsISupports*>(*aInstancePtr));

    return NS_OK;
}


NS_IMPL_ADDREF( nsSanePluginFactoryImpl )
NS_IMPL_RELEASE( nsSanePluginFactoryImpl )

NS_IMETHODIMP
nsSanePluginFactoryImpl::CreateInstance( nsISupports *aOuter,
                                         const nsIID &aIID,
                                         void **aResult)
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::CreateInstance()\n");
#endif

    if ( !aResult )
        return NS_ERROR_NULL_POINTER;
  
    if ( aOuter )
        return NS_ERROR_NO_AGGREGATION;

    nsSanePluginInstance * inst = new nsSanePluginInstance();
    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;
           
    inst->AddRef();
    *aResult = inst;
    return NS_OK;
}

nsresult 
nsSanePluginFactoryImpl::LockFactory(PRBool aLock)
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::LockFactory()\n");
#endif

    

    return NS_OK;
}


NS_METHOD
nsSanePluginFactoryImpl::Initialize()
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::Initialize()\n");
#endif

    return NS_OK;
}


NS_METHOD
nsSanePluginFactoryImpl::Shutdown( void )
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::Shutdown()\n");
#endif

    return NS_OK;
}


NS_METHOD 
nsSanePluginFactoryImpl::GetMIMEDescription(const char* *result)
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::GetMIMEDescription()\n");
#endif

    
    *result = PL_strdup( PLUGIN_MIME_DESCRIPTION );

    return NS_OK;
}

NS_METHOD
nsSanePluginFactoryImpl::GetValue( nsPluginVariable variable, void *value )
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::GetValue()\n");
#endif

    nsresult err = NS_OK;

    if ( variable == nsPluginVariable_NameString ) {
    
        *( ( char ** )value ) = strdup( PLUGIN_NAME );

    } else if ( variable == nsPluginVariable_DescriptionString ) {

        *( ( char ** )value ) = strdup( PLUGIN_DESCRIPTION );

    } else {
    
        err = NS_ERROR_FAILURE;

    }
  
    return err;
}


NS_IMETHODIMP 
nsSanePluginFactoryImpl::CreatePluginInstance( nsISupports *aOuter, 
                                               REFNSIID aIID, 
                                               const char* aPluginMIMEType,
                                               void **aResult)
{
#ifdef DEBUG
    printf("nsSanePluginFactoryImpl::CreatePluginInstance()\n");
#endif

    
    
    
    return NS_ERROR_NOT_IMPLEMENTED;
}











extern "C" PR_IMPLEMENT(nsresult)
NSGetFactory( nsISupports* aServMgr,
              const nsCID &aClass,
              const char *aClassName,
              const char *aContractID,
              nsIFactory **aFactory)
{
    if (! aFactory)
        return NS_ERROR_NULL_POINTER;
  
    nsSanePluginFactoryImpl* factory = new nsSanePluginFactoryImpl(aClass, 
                                                                   aClassName,
                                                                   aContractID);
    if ( factory == nsnull )
        return NS_ERROR_OUT_OF_MEMORY;
  
    NS_ADDREF(factory);
    *aFactory = factory;
    return NS_OK;

}

char *buf;

extern "C" PR_IMPLEMENT( nsresult )
NSRegisterSelf( nsISupports* aServMgr, const char* aPath )
{
    nsresult rv;
  
    nsCOMPtr<nsIServiceManager> servMgr( do_QueryInterface( aServMgr, &rv ) );
    if ( NS_FAILED( rv ) )
        return rv;
  
    nsCOMPtr<nsIComponentManager> compMgr = 
             do_GetService( kComponentManagerCID, &rv );
    if ( NS_FAILED( rv ) )
        return rv;
  
    
    rv = compMgr->RegisterComponent(knsSanePluginControlCID,
                                    "SANE Plugin Control",
                                    "@mozilla.org/plugins/sane-control;1",
                                    aPath, PR_TRUE, PR_TRUE );
  
    
    rv = compMgr->RegisterComponent( knsSanePluginInst,
                                     "SANE Plugin Component",
                                     NS_INLINE_PLUGIN_CONTRACTID_PREFIX PLUGIN_MIME_TYPE
                                     aPath, PR_TRUE, PR_TRUE);
    if ( NS_FAILED( rv ) )
        return rv;
  
    return NS_OK;

}

extern "C" PR_IMPLEMENT( nsresult )
NSUnregisterSelf(nsISupports* aServMgr, const char* aPath)
{
    nsresult rv;
  
    nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
    if (NS_FAILED(rv)) return rv;
  
    nsCOMPtr<nsIComponentManager> compMgr = 
             do_GetService(kComponentManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;
  
    rv = compMgr->UnregisterComponent(knsSanePluginControlCID, aPath);
    if (NS_FAILED(rv)) return rv;
  
    return NS_OK;
}


















