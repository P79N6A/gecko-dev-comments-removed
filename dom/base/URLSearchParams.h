





#ifndef mozilla_dom_URLSearchParams_h
#define mozilla_dom_URLSearchParams_h

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsISupports.h"
#include "nsIUnicodeDecoder.h"

namespace mozilla {
namespace dom {

class URLSearchParams;

class URLSearchParamsObserver : public nsISupports
{
public:
  virtual ~URLSearchParamsObserver() {}

  virtual void URLSearchParamsUpdated(URLSearchParams* aFromThis) = 0;
};





class URLSearchParams final : public nsISupports,
                              public nsWrapperCache
{
  ~URLSearchParams();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(URLSearchParams)

  explicit URLSearchParams(URLSearchParamsObserver* aObserver);

  
  nsISupports* GetParentObject() const
  {
    return nullptr;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<URLSearchParams>
  Constructor(const GlobalObject& aGlobal, const nsAString& aInit,
              ErrorResult& aRv);

  static already_AddRefed<URLSearchParams>
  Constructor(const GlobalObject& aGlobal, URLSearchParams& aInit,
              ErrorResult& aRv);

  void ParseInput(const nsACString& aInput);

  void Serialize(nsAString& aValue) const;

  void Get(const nsAString& aName, nsString& aRetval);

  void GetAll(const nsAString& aName, nsTArray<nsString >& aRetval);

  void Set(const nsAString& aName, const nsAString& aValue);

  void Append(const nsAString& aName, const nsAString& aValue);

  bool Has(const nsAString& aName);

  void Delete(const nsAString& aName);

  void Stringify(nsString& aRetval) const
  {
    Serialize(aRetval);
  }

  class ForEachIterator
  {
  public:
    virtual bool
    URLSearchParamsIterator(const nsString& aName, const nsString& aValue) = 0;
  };

  bool
  ForEach(ForEachIterator& aIterator)
  {
    for (uint32_t i = 0; i < mSearchParams.Length(); ++i) {
      if (!aIterator.URLSearchParamsIterator(mSearchParams[i].mKey,
                                             mSearchParams[i].mValue)) {
        return false;
      }
    }

    return true;
  }

private:
  void AppendInternal(const nsAString& aName, const nsAString& aValue);

  void DeleteAll();

  void DecodeString(const nsACString& aInput, nsAString& aOutput);
  void ConvertString(const nsACString& aInput, nsAString& aOutput);

  void NotifyObserver();

  struct Param
  {
    nsString mKey;
    nsString mValue;
  };

  nsTArray<Param> mSearchParams;

  nsRefPtr<URLSearchParamsObserver> mObserver;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
};

} 
} 

#endif 
