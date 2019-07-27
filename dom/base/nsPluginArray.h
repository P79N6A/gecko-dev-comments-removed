





#ifndef nsPluginArray_h___
#define nsPluginArray_h___

#include "nsTArray.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsWrapperCache.h"
#include "nsPluginTags.h"
#include "nsPIDOMWindow.h"

class nsPluginElement;
class nsMimeType;

class nsPluginArray MOZ_FINAL : public nsIObserver,
                                public nsSupportsWeakReference,
                                public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsPluginArray,
                                                         nsIObserver)

  
  NS_DECL_NSIOBSERVER

  explicit nsPluginArray(nsPIDOMWindow* aWindow);
  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  
  
  
  void Init();
  void Invalidate();

  void GetMimeTypes(nsTArray<nsRefPtr<nsMimeType> >& aMimeTypes,
                    nsTArray<nsRefPtr<nsMimeType> >& aHiddenMimeTypes);

  

  nsPluginElement* Item(uint32_t aIndex);
  nsPluginElement* NamedItem(const nsAString& aName);
  void Refresh(bool aReloadDocuments);
  nsPluginElement* IndexedGetter(uint32_t aIndex, bool &aFound);
  nsPluginElement* NamedGetter(const nsAString& aName, bool &aFound);
  bool NameIsEnumerable(const nsAString& aName);
  uint32_t Length();
  void GetSupportedNames(unsigned, nsTArray<nsString>& aRetval);

private:
  virtual ~nsPluginArray();

  bool AllowPlugins() const;
  void EnsurePlugins();

  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  
  
  
  
  nsTArray<nsRefPtr<nsPluginElement> > mPlugins;

  
  
  
  nsTArray<nsRefPtr<nsPluginElement> > mHiddenPlugins;
};

class nsPluginElement MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPluginElement)

  nsPluginElement(nsPIDOMWindow* aWindow, nsPluginTag* aPluginTag);

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

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
  bool NameIsEnumerable(const nsAString& aName);
  uint32_t Length();
  void GetSupportedNames(unsigned, nsTArray<nsString>& retval);

  nsTArray<nsRefPtr<nsMimeType> >& MimeTypes();

protected:
  ~nsPluginElement();

  void EnsurePluginMimeTypes();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<nsPluginTag> mPluginTag;
  nsTArray<nsRefPtr<nsMimeType> > mMimeTypes;
};

#endif 
