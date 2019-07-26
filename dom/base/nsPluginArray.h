





#ifndef nsPluginArray_h___
#define nsPluginArray_h___

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsWrapperCache.h"
#include "nsPluginTags.h"

class nsPIDOMWindow;
class nsPluginElement;
class nsMimeType;
class nsPluginTag;

class nsPluginArray MOZ_FINAL : public nsIObserver,
                                public nsSupportsWeakReference,
                                public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsPluginArray,
                                                         nsIObserver)

  
  NS_DECL_NSIOBSERVER

  nsPluginArray(nsWeakPtr aWindow);
  virtual ~nsPluginArray();

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  
  
  
  void Init();
  void Invalidate();

  void GetPlugins(nsTArray<nsRefPtr<nsPluginElement> >& aPlugins);

  

  nsPluginElement* Item(uint32_t aIndex);
  nsPluginElement* NamedItem(const nsAString& aName);
  void Refresh(bool aReloadDocuments);
  nsPluginElement* IndexedGetter(uint32_t aIndex, bool &aFound);
  nsPluginElement* NamedGetter(const nsAString& aName, bool &aFound);
  uint32_t Length();
  void GetSupportedNames(nsTArray< nsString >& aRetval);

private:
  bool AllowPlugins() const;
  void EnsurePlugins();

  nsWeakPtr mWindow;
  nsTArray<nsRefPtr<nsPluginElement> > mPlugins;
};

class nsPluginElement MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPluginElement)

  nsPluginElement(nsWeakPtr aWindow, nsPluginTag* aPluginTag);
  virtual ~nsPluginElement();

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsPluginTag* PluginTag() const
  {
    return mPluginTag;
  }

  

  void GetDescription(nsString& retval) const;
  void GetFilename(nsString& retval) const;
  void GetVersion(nsString& retval) const;
  void GetName(nsString& retval) const;
  nsMimeType* Item(uint32_t index);
  nsMimeType* NamedItem(const nsAString& name);
  nsMimeType* IndexedGetter(uint32_t index, bool &found);
  nsMimeType* NamedGetter(const nsAString& name, bool &found);
  uint32_t Length();
  void GetSupportedNames(nsTArray< nsString >& retval);

  nsTArray<nsRefPtr<nsMimeType> >& MimeTypes();

protected:
  void EnsureMimeTypes();
  void Invalidate();

  nsWeakPtr mWindow;
  nsRefPtr<nsPluginTag> mPluginTag;
  nsTArray<nsRefPtr<nsMimeType> > mMimeTypes;
};

#endif 
