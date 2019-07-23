















































#include "plugin.h"
#include "acmeIScriptObject.h"
#include "npapi.h"

static NS_DEFINE_IID(kI4xScriptablePluginIID, NS_I4XSCRIPTABLEPLUGIN_IID);
static NS_DEFINE_IID(kIClassInfoIID, NS_ICLASSINFO_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsScriptablePeer::nsScriptablePeer(CPlugin* aPlugin)
{
  mRefCnt = 0;
  mPlugin = aPlugin;
  mWindow = nsnull;
}

nsScriptablePeer::~nsScriptablePeer()
{
}



NS_IMETHODIMP_(nsrefcnt) nsScriptablePeer::AddRef() 
{ 
  ++mRefCnt; 
  return mRefCnt; 
} 

NS_IMETHODIMP_(nsrefcnt) nsScriptablePeer::Release() 
{ 
  --mRefCnt; 
  if (mRefCnt == 0) { 
    delete this;
    return 0; 
  } 
  return mRefCnt; 
} 



NS_IMETHODIMP nsScriptablePeer::QueryInterface(const nsIID& aIID, void** aInstancePtr) 
{ 
  if(!aInstancePtr) 
    return NS_ERROR_NULL_POINTER; 

  if(aIID.Equals(kI4xScriptablePluginIID)) {
    *aInstancePtr = static_cast<nsI4xScriptablePlugin*>(this); 
    AddRef();
    return NS_OK;
  }

  if(aIID.Equals(kIClassInfoIID)) {
    *aInstancePtr = static_cast<nsIClassInfo*>(this); 
    AddRef();
    return NS_OK;
  }

  if(aIID.Equals(kISupportsIID)) {
    *aInstancePtr = static_cast<nsISupports*>(static_cast<nsI4xScriptablePlugin*>(this)); 
    AddRef();
    return NS_OK;
  }

  return NS_NOINTERFACE; 
}




NS_IMETHODIMP nsScriptablePeer::ShowVersion()
{
  if (mPlugin)
    mPlugin->showVersion();

  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::Clear()
{
  if (mPlugin)
    mPlugin->clear();

  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::GetVersion(char * *aVersion)
{
  if (mPlugin)
    mPlugin->getVersion(aVersion);
  return NS_OK;
}

NS_IMETHODIMP nsScriptablePeer::SetWindow(acmeIScriptObject *window)
{
  NS_IF_ADDREF(window);
  NS_IF_RELEASE(mWindow);
  mWindow = window;
  
  
  acmeIScriptObject* result;
  nsresult rv = window->Evaluate("Math.PI", &result);
  if (NS_SUCCEEDED(rv) && result) {
      double value;
      result->ToNumber(&value);
      NS_RELEASE(result);
  }

  
  acmeIScriptObject* location = nsnull;
  rv = window->GetProperty("location", &location);
  if (NS_SUCCEEDED(rv) && location) {
    char* locationStr = NULL;
    rv = location->ToString(&locationStr);
    if (NS_SUCCEEDED(rv) && locationStr) {
      NPN_MemFree(locationStr);
    }
    NS_RELEASE(location);
  }
  
  return NS_OK;
}
