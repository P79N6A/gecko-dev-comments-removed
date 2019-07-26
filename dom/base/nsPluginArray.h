




#ifndef nsPluginArray_h___
#define nsPluginArray_h___

#include "nsCOMPtr.h"
#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include "nsIPluginHost.h"
#include "nsIURL.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {
class Navigator;
} 
} 

class nsIDocShell;



class nsPluginArray : public nsIDOMPluginArray
                    , public nsIObserver
                    , public nsSupportsWeakReference
{
public:
  nsPluginArray(mozilla::dom::Navigator* navigator, nsIDocShell *aDocShell);
  virtual ~nsPluginArray();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMPLUGINARRAY
  
  NS_DECL_NSIOBSERVER

  
  
  
  
  void Init();

  nsresult GetPluginHost(nsIPluginHost** aPluginHost);

  nsIDOMPlugin* GetItemAt(uint32_t aIndex, nsresult* aResult);
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

    return static_cast<nsPluginArray*>(static_cast<nsIDOMPluginArray*>(aSupports));
  }

private:
  nsresult GetPlugins();
  bool AllowPlugins();

public:
  void Invalidate();

protected:
  mozilla::dom::Navigator* mNavigator;
  nsCOMPtr<nsIPluginHost> mPluginHost;
  uint32_t mPluginCount;
  nsIDOMPlugin** mPluginArray;
  nsWeakPtr mDocShell;
};

class nsPluginElement : public nsIDOMPlugin
{
public:
  nsPluginElement(nsIDOMPlugin* plugin);
  virtual ~nsPluginElement();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPLUGIN

  nsIDOMMimeType* GetItemAt(uint32_t aIndex, nsresult* aResult);
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
  uint32_t mMimeTypeCount;
  nsIDOMMimeType** mMimeTypeArray;
};

#endif 
