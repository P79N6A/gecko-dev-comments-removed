




































#ifndef nsPluginArray_h___
#define nsPluginArray_h___

#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include "nsIPluginHost.h"
#include "nsIURL.h"

class nsNavigator;
class nsIDocShell;
class nsIPluginHost;

class nsPluginArray : public nsIDOMPluginArray,
                      public nsIDOMJSPluginArray
{
public:
  nsPluginArray(nsNavigator* navigator, nsIDocShell *aDocShell);
  virtual ~nsPluginArray();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMPLUGINARRAY

  
  NS_DECL_NSIDOMJSPLUGINARRAY

  nsresult GetPluginHost(nsIPluginHost** aPluginHost);

private:
  nsresult GetPlugins();
  PRBool AllowPlugins();

public:
  void SetDocShell(nsIDocShell* aDocShell);

protected:
  nsNavigator* mNavigator;
  nsCOMPtr<nsIPluginHost> mPluginHost;
  PRUint32 mPluginCount;
  nsIDOMPlugin** mPluginArray;
  nsIDocShell* mDocShell; 
};

class nsPluginElement : public nsIDOMPlugin
{
public:
  nsPluginElement(nsIDOMPlugin* plugin);
  virtual ~nsPluginElement();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPLUGIN

private:
  nsresult GetMimeTypes();

protected:
  nsIDOMPlugin* mPlugin;
  PRUint32 mMimeTypeCount;
  nsIDOMMimeType** mMimeTypeArray;
};

#endif 
