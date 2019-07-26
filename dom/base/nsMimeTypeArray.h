





#ifndef nsMimeTypeArray_h___
#define nsMimeTypeArray_h___

#include "nsString.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsPIDOMWindow.h"

class nsMimeType;
class nsPluginElement;

class nsMimeTypeArray MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  nsMimeTypeArray(nsPIDOMWindow* aWindow);
  virtual ~nsMimeTypeArray();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsMimeTypeArray)

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void Refresh();

  
  nsMimeType* Item(uint32_t index);
  nsMimeType* NamedItem(const nsAString& name);
  nsMimeType* IndexedGetter(uint32_t index, bool &found);
  nsMimeType* NamedGetter(const nsAString& name, bool &found);
  uint32_t Length();
  void GetSupportedNames(nsTArray< nsString >& retval);

protected:
  void EnsurePluginMimeTypes();
  void Clear();

  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  
  
  
  nsTArray<nsRefPtr<nsMimeType> > mMimeTypes;

  
  
  
  nsTArray<nsRefPtr<nsMimeType> > mHiddenMimeTypes;
};

class nsMimeType MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsMimeType)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsMimeType)

  nsMimeType(nsPIDOMWindow* aWindow, nsPluginElement* aPluginElement,
             uint32_t aPluginTagMimeIndex, const nsAString& aMimeType);
  nsMimeType(nsPIDOMWindow* aWindow, const nsAString& aMimeType);
  virtual ~nsMimeType();

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  const nsString& Type() const
  {
    return mType;
  }

  
  void GetDescription(nsString& retval) const;
  nsPluginElement *GetEnabledPlugin() const;
  void GetSuffixes(nsString& retval) const;
  void GetType(nsString& retval) const;

protected:
  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  
  
  
  nsRefPtr<nsPluginElement> mPluginElement;
  uint32_t mPluginTagMimeIndex;
  nsString mType;
};

#endif 
