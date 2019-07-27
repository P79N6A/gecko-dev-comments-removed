





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
  explicit nsMimeTypeArray(nsPIDOMWindow* aWindow);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsMimeTypeArray)

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void Refresh();

  
  nsMimeType* Item(uint32_t index);
  nsMimeType* NamedItem(const nsAString& name);
  nsMimeType* IndexedGetter(uint32_t index, bool &found);
  nsMimeType* NamedGetter(const nsAString& name, bool &found);
  bool NameIsEnumerable(const nsAString& name);
  uint32_t Length();
  void GetSupportedNames(unsigned, nsTArray< nsString >& retval);

protected:
  virtual ~nsMimeTypeArray();

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
  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  const nsString& Type() const
  {
    return mType;
  }

  
  void GetDescription(nsString& retval) const;
  nsPluginElement *GetEnabledPlugin() const;
  void GetSuffixes(nsString& retval) const;
  void GetType(nsString& retval) const;

protected:
  virtual ~nsMimeType();

  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  
  
  
  nsRefPtr<nsPluginElement> mPluginElement;
  uint32_t mPluginTagMimeIndex;
  nsString mType;
};

#endif 
