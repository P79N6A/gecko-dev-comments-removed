











































#ifndef __nsScriptablePeer_h__
#define __nsScriptablePeer_h__

#include "nsI4xScriptablePlugin.h"
#include "nsIClassInfo.h"
#include "nsIProgrammingLanguage.h"

class CPlugin;




class nsClassInfoMixin : public nsIClassInfo
{
  
  
  NS_IMETHOD GetFlags(PRUint32 *aFlags)
    {*aFlags = nsIClassInfo::PLUGIN_OBJECT | nsIClassInfo::DOM_OBJECT;
     return NS_OK;}
  NS_IMETHOD GetImplementationLanguage(PRUint32 *aImplementationLanguage)
    {*aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
     return NS_OK;}
  
  NS_IMETHOD GetInterfaces(PRUint32 *count, nsIID * **array)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetContractID(char * *aContractID)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetClassDescription(char * *aClassDescription)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetClassID(nsCID * *aClassID)
    {return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
    {return NS_ERROR_NOT_IMPLEMENTED;}
};

class nsScriptablePeer : public nsI4xScriptablePlugin,
                         public nsClassInfoMixin
{
public:
  nsScriptablePeer(CPlugin* plugin);
  ~nsScriptablePeer();

public:
  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr); 
  NS_IMETHOD_(nsrefcnt) AddRef(); 
  NS_IMETHOD_(nsrefcnt) Release(); 

protected: 
  nsrefcnt mRefCnt;  

public:
  
  NS_DECL_NSI4XSCRIPTABLEPLUGIN

protected:
  CPlugin* mPlugin;
  acmeIScriptObject* mWindow;
};

#endif
