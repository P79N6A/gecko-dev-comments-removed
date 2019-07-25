




































#ifndef nsPluginArray_h___
#define nsPluginArray_h___

#include "nsCOMPtr.h"
#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include "nsIPluginHost.h"
#include "nsIURL.h"

class nsNavigator;
class nsIDocShell;



class nsPluginArray : public nsIDOMPluginArray
{
public:
  nsPluginArray(nsNavigator* navigator, nsIDocShell *aDocShell);
  virtual ~nsPluginArray();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMPLUGINARRAY

  nsresult GetPluginHost(nsIPluginHost** aPluginHost);

  nsIDOMPlugin* GetItemAt(PRUint32 aIndex, nsresult* aResult);
  nsIDOMPlugin* GetNamedItem(const nsAString& aName, nsresult* aResult);

  static nsPluginArray* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMPluginArray> array_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(array_qi == static_cast<nsIDOMPluginArray*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsPluginArray*>(aSupports);
  }

private:
  nsresult GetPlugins();
  PRBool AllowPlugins();

public:
  void SetDocShell(nsIDocShell *aDocShell);
  void Invalidate();

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

  nsIDOMMimeType* GetItemAt(PRUint32 aIndex, nsresult* aResult);
  nsIDOMMimeType* GetNamedItem(const nsAString& aName, nsresult* aResult);

  static nsPluginElement* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMPlugin> plugin_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(plugin_qi == static_cast<nsIDOMPlugin*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsPluginElement*>(aSupports);
  }

private:
  nsresult GetMimeTypes();

protected:
  nsIDOMPlugin* mPlugin;
  PRUint32 mMimeTypeCount;
  nsIDOMMimeType** mMimeTypeArray;
};

#endif 
