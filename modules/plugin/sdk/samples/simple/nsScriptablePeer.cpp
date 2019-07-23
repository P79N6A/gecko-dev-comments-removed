














































#include "plugin.h"

static NS_DEFINE_IID(kISimplePluginIID, NS_ISIMPLEPLUGIN_IID);
static NS_DEFINE_IID(kIClassInfoIID, NS_ICLASSINFO_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsScriptablePeer::nsScriptablePeer(nsPluginInstance* aPlugin)
{
  mPlugin = aPlugin;
  mRefCnt = 0;
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

  if(aIID.Equals(kISimplePluginIID)) {
    *aInstancePtr = static_cast<nsISimplePlugin*>(this); 
    AddRef();
    return NS_OK;
  }

  if(aIID.Equals(kIClassInfoIID)) {
    *aInstancePtr = static_cast<nsIClassInfo*>(this); 
    AddRef();
    return NS_OK;
  }

  if(aIID.Equals(kISupportsIID)) {
    *aInstancePtr = static_cast<nsISupports*>((static_cast<nsISimplePlugin*>(this))); 
    AddRef();
    return NS_OK;
  }

  return NS_NOINTERFACE; 
}

void nsScriptablePeer::SetInstance(nsPluginInstance* plugin)
{
  mPlugin = plugin;
}




NS_IMETHODIMP nsScriptablePeer::GetVersion(char * *aVersion)
{
  if (mPlugin)
    mPlugin->getVersion(aVersion);
  return NS_OK;
}
