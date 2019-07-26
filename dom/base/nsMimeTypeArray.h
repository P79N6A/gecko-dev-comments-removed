





#ifndef nsMimeTypeArray_h___
#define nsMimeTypeArray_h___

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "nsWrapperCache.h"

class nsPIDOMWindow;
class nsPluginElement;
class nsMimeType;

class nsMimeTypeArray MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  nsMimeTypeArray(nsWeakPtr aWindow);
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
  void EnsureMimeTypes();
  void Clear();

  nsWeakPtr mWindow;

  
  
  
  nsTArray<nsRefPtr<nsMimeType> > mMimeTypes;

  
  
  
  uint32_t mPluginMimeTypeCount;
};

class nsMimeType MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsMimeType)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsMimeType)

  nsMimeType(nsWeakPtr aWindow, nsPluginElement* aPluginElement,
             uint32_t aPluginTagMimeIndex, const nsAString& aMimeType);
  nsMimeType(nsWeakPtr aWindow, const nsAString& aMimeType);
  virtual ~nsMimeType();

  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void Invalidate()
  {
    mPluginElement = nullptr;
  }

  const nsString& Type() const
  {
    return mType;
  }

  
  void GetDescription(nsString& retval) const;
  nsPluginElement *GetEnabledPlugin() const;
  void GetSuffixes(nsString& retval) const;
  void GetType(nsString& retval) const;

protected:
  nsWeakPtr mWindow;

  
  nsPluginElement *mPluginElement;
  uint32_t mPluginTagMimeIndex;
  nsString mType;
};

#endif 
