





































#ifndef XPCONNECT_H
#define XPCONNECT_H

#include <servprov.h>

#ifdef XPC_IDISPATCH_SUPPORT
#include "nsIDispatchSupport.h"
#include "nsIActiveXSecurityPolicy.h"
#endif

#include "nsID.h"
#include "nsCOMPtr.h"
#include "nsIClassInfo.h"
#include "nsIProgrammingLanguage.h"
#include "nsIMozAxPlugin.h"
#include "nsServiceManagerUtils.h"
#include "nsIURI.h"

#include "ControlEventSink.h"

struct PluginInstanceData;

template <class T> class nsIClassInfoImpl : public nsIClassInfo
{
    NS_IMETHODIMP GetFlags(PRUint32 *aFlags)
    {
        *aFlags = nsIClassInfo::PLUGIN_OBJECT | nsIClassInfo::DOM_OBJECT;
        return NS_OK;
    }
    NS_IMETHODIMP GetImplementationLanguage(PRUint32 *aImplementationLanguage)
    {
        *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
        return NS_OK;
    }
    
    NS_IMETHODIMP GetInterfaces(PRUint32 *count, nsIID * **array)
    { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHODIMP GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
    { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHODIMP GetContractID(char * *aContractID)
    { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHODIMP GetClassDescription(char * *aClassDescription)
    { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHODIMP GetClassID(nsCID * *aClassID)
    { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHODIMP GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
    { return NS_ERROR_NOT_IMPLEMENTED; }
};

class nsScriptablePeerTearOff;

class nsScriptablePeer :
    public nsIClassInfoImpl<nsScriptablePeer>,
    public nsIMozAxPlugin
{
    friend nsScriptablePeerTearOff;
protected:
    virtual ~nsScriptablePeer();

public:
    nsScriptablePeer();

    nsScriptablePeerTearOff *mTearOff;
    PluginInstanceData* mPlugin;

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMOZAXPLUGIN

protected:
    HRESULT GetIDispatch(IDispatch **pdisp);
    HRESULT ConvertVariants(nsIVariant *aIn, VARIANT *aOut);
    HRESULT ConvertVariants(VARIANT *aIn, nsIVariant **aOut);
    nsresult HR2NS(HRESULT hr) const;
    NS_IMETHOD InternalInvoke(const char *aMethod, unsigned int aNumArgs, nsIVariant *aArgs[]);
};

class nsScriptablePeerTearOff :
    public IDispatch
{
public:
    nsScriptablePeerTearOff(nsScriptablePeer *pOwner);
    nsScriptablePeer *mOwner;


    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release( void);


    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT __RPC_FAR *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);
};

#ifdef XPC_IDISPATCH_SUPPORT
class nsEventSink : public CControlEventSink
{
public:
    PluginInstanceData* mPlugin;

    virtual HRESULT InternalInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
};

typedef CComObject<nsEventSink> nsEventSinkInstance;
#endif

namespace MozAxPlugin {
    extern void AddRef();
    extern void Release();
    extern CLSID GetCLSIDForType(const char *mimeType);
    extern NPError GetValue(NPP instance, NPPVariable variable, void *value);
    extern nsScriptablePeer *GetPeerForCLSID(const CLSID &clsid);
    extern void GetIIDForCLSID(const CLSID &clsid, nsIID &iid);
    extern HRESULT GetServiceProvider(PluginInstanceData *pData, IServiceProvider **pSP);
#ifdef XPC_IDISPATCH_SUPPORT
    extern PRUint32 PrefGetHostingFlags();
    extern void ReleasePrefObserver();
    extern nsresult GetCurrentLocation(NPP instance, nsIURI **aLocation);
#endif
}

#endif
