















































#include "plugin.h"

nsScriptablePeer::nsScriptablePeer(CPlugin* aPlugin)
{

  mPlugin = aPlugin;
}

nsScriptablePeer::~nsScriptablePeer()
{
}


NS_IMPL_ISUPPORTS2(nsScriptablePeer, nsI4xScriptablePlugin, nsIClassInfo)




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
