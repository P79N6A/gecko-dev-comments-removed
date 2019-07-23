














































#ifndef _NO_XPCOMDLL_
# ifdef IPLUGINW_OUTOFTREE
#  define _NO_XPCOMDLL_ 1
# endif
#endif

#ifndef _MINIMAL_XPCOMDLL_
# ifdef _NO_XPCOMDLL_
#  define _MINIMAL_XPCOMDLL_ 1
# endif
#endif





#ifdef _NO_XPCOMDLL_
#define INCL_BASE
#include <os2.h>
#endif
#ifdef DEBUG
#include <stdio.h>
#endif
#include "nsCOMPtr.h"
#include "nsIPlugin.h"
#include "nsIGenericFactory.h"
#include "nsILegacyPluginWrapperOS2.h"
#include "nsInnoTekPluginWrapper.h"





static NS_DEFINE_CID(kPluginCID, NS_PLUGIN_CID);
static NS_DEFINE_CID(kSupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kLegacyPluginWrapperIID, NS_ILEGACYPLUGINWRAPPEROS2_IID);





class nsInnoTekPluginWrapper : public nsILegacyPluginWrapperOS2
{
#ifdef _MINIMAL_XPCOMDLL_
public:
    NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
    NS_IMETHOD_(nsrefcnt) AddRef(void);
    NS_IMETHOD_(nsrefcnt) Release(void);

protected:
    nsAutoRefCnt mRefCnt;
#else
    NS_DECL_ISUPPORTS
#endif

public:
    nsInnoTekPluginWrapper();
    static nsresult Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    


    
    NS_IMETHOD GetFactory(nsIServiceManagerObsolete *aServMgr, REFNSIID aClass, const char *aClassName, const char *aContractID, PRLibrary * aLibrary, nsIPlugin **aResult);

    











    
    NS_IMETHOD MaybeWrap(REFNSIID aIID, nsISupports *aIn, nsISupports **aOut);
};



#ifdef _MINIMAL_XPCOMDLL_
NS_IMETHODIMP_(nsrefcnt) nsInnoTekPluginWrapper::AddRef(void)
{
    return ++mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt) nsInnoTekPluginWrapper::Release(void)
{
    --mRefCnt;
    if (mRefCnt == 0)
    {
        mRefCnt = 1; 
        delete this;
        return 0;
    }
    return mRefCnt;
}

NS_IMETHODIMP nsInnoTekPluginWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (aIID.Equals(kLegacyPluginWrapperIID))
    {
        *aInstancePtr = static_cast< nsILegacyPluginWrapperOS2 * > (this);
        AddRef();
        return NS_OK;
    }

    if (aIID.Equals(kSupportsIID))
    {
        *aInstancePtr = static_cast< nsISupports * > (this);
        AddRef();
        return NS_OK;
    }

    *aInstancePtr = nsnull;
    return NS_ERROR_NO_INTERFACE;
}

#else
NS_IMPL_ISUPPORTS1(nsInnoTekPluginWrapper, nsILegacyPluginWrapperOS2)
#endif




nsInnoTekPluginWrapper::nsInnoTekPluginWrapper()
{
    
    npXPCOMInitSems();
}






nsresult nsInnoTekPluginWrapper::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    *aResult = nsnull;
#ifndef _MINIMAL_XPCOMDLL_
    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
#endif
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

#ifdef _MINIMAL_XPCOMDLL_
    nsILegacyPluginWrapperOS2 *
#else
    nsCOMPtr<nsILegacyPluginWrapperOS2>
#endif
        factory  = new nsInnoTekPluginWrapper();
    if (!factory)
        return NS_ERROR_OUT_OF_MEMORY;

    return factory->QueryInterface(aIID, aResult);
}





NS_IMETHODIMP nsInnoTekPluginWrapper::GetFactory(nsIServiceManagerObsolete *aServMgr,
                                                 REFNSIID aClass,
                                                 const char *aClassName,
                                                 const char *aContractID,
                                                 PRLibrary * aLibrary,
                                                 nsIPlugin **aResult)
{
    nsresult rc;
    dprintf(("nsInnoTekPluginWrapper::CreatePlugin: enter"));

    


    rc = npXPCOMGenericGetFactory(aServMgr, aClass, aClassName, aContractID, aLibrary, aResult);
    dprintf(("nsInnoTekPluginWrapper::CreatePlugin: npXPCOMGenericGetFactory -> rc=%d and *aResult=%x",
             rc, *aResult));

    return rc;
}















NS_IMETHODIMP nsInnoTekPluginWrapper::MaybeWrap(REFNSIID aIID, nsISupports *aIn, nsISupports **aOut)
{
   return npXPCOMGenericMaybeWrap(aIID, aIn, aOut);
}





static const nsModuleComponentInfo gComponentInfo[] =
{
    {
        "InnoTek Legacy XPCOM Plugin Wrapper",
        NS_LEGACY_PLUGIN_WRAPPER_CID,
        NS_LEGACY_PLUGIN_WRAPPER_CONTRACTID,
        nsInnoTekPluginWrapper::Create
    }
};




NS_IMPL_NSGETMODULE(nsInnoTekPluginWrapperModule, gComponentInfo)


#ifdef _NO_XPCOMDLL_






static char *GetBasename(char *pszPath)
{
    char *pszName;
    char *psz;

    pszName = strrchr(pszPath, '\\');
    psz = strrchr(pszName ? pszName : pszPath, '/');
    if (psz)
        pszName = psz;
    if (!pszName)
        pszName = strchr(pszPath, ':');
    if (pszName)
        pszName++;
    else
        pszName = pszPath;

    return pszName;
}

nsresult NS_NewGenericModule2(nsModuleInfo *info, nsIModule* *result)
{
    HMODULE     hmod;
    nsresult    rc = NS_ERROR_UNEXPECTED;
    APIRET      rc2;

    





    char    szXPCOM[CCHMAXPATH];
    char *  pszName;
    PPIB    ppib;
    PTIB    ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    pszName = GetBasename(strcpy(szXPCOM, ppib->pib_pchcmd));
    strcpy(pszName, "XPCOM.DLL");
    rc2 = DosLoadModule(NULL, 0, (PCSZ)szXPCOM, &hmod);
    if (!rc2)
    {
        NS_COM nsresult (* pfnNS_NewGenericModule2)(nsModuleInfo *info, nsIModule* *result);

        
        rc2 = DosQueryProcAddr(hmod, 0, (PCSZ)"__Z20NS_NewGenericModule2P12nsModuleInfoPP9nsIModule",
                               (PFN*)&pfnNS_NewGenericModule2);
        if (rc2)
            
            rc2 = DosQueryProcAddr(hmod, 0, (PCSZ)"__Z20NS_NewGenericModule2PK12nsModuleInfoPP9nsIModule",
                                   (PFN*)&pfnNS_NewGenericModule2);
        if (rc2)
        {
            
            DosFreeModule(hmod);
            strcpy(pszName, "XPCOMCOR.DLL");
            rc2 = DosLoadModule(NULL, 0, (PCSZ)szXPCOM, &hmod);
            if (!rc2)
            {
                rc2 = DosQueryProcAddr(hmod, 0, (PCSZ)"__Z20NS_NewGenericModule2P12nsModuleInfoPP9nsIModule",
                                       (PFN*)&pfnNS_NewGenericModule2);
                if (rc2)
                    
                    rc2 = DosQueryProcAddr(hmod, 0, (PCSZ)"__Z20NS_NewGenericModule2PK12nsModuleInfoPP9nsIModule",
                                           (PFN*)&pfnNS_NewGenericModule2);
            }
        }
        if (!rc2)
             rc = pfnNS_NewGenericModule2(info, result);
        #ifdef DEBUG
        else
            fprintf(stderr, "ipluginw: DosQueryProcAddr -> %ld\n", rc2);
        #endif
        DosFreeModule(hmod);
    }
    #ifdef DEBUG
    else
        fprintf(stderr, "ipluginw: DosLoadModule -> %ld\n", rc2);
    if (rc)
        fprintf(stderr, "ipluginw: NS_NewGenericModule2 -> %#x\n", rc);
    #endif

    return rc;
}
#endif


