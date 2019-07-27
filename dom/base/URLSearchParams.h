





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





class URLParams final
{
public:
  URLParams() {}

  ~URLParams()
  {
    DeleteAll();
  }

  explicit URLParams(const URLParams& aOther)
    : mParams(aOther.mParams)
  {}

  explicit URLParams(const URLParams&& aOther)
    : mParams(Move(aOther.mParams))
  {}

  class ForEachIterator
  {
  public:
    virtual bool
    URLParamsIterator(const nsString& aName, const nsString& aValue) = 0;
  };

  void
  ParseInput(const nsACString& aInput);

  bool
  ForEach(ForEachIterator& aIterator) const
  {
    for (uint32_t i = 0; i < mParams.Length(); ++i) {
      if (!aIterator.URLParamsIterator(mParams[i].mKey, mParams[i].mValue)) {
        return false;
      }
    }

    return true;
  }

  void Serialize(nsAString& aValue) const;

  void Get(const nsAString& aName, nsString& aRetval);

  void GetAll(const nsAString& aName, nsTArray<nsString >& aRetval);

  void Set(const nsAString& aName, const nsAString& aValue);

  void Append(const nsAString& aName, const nsAString& aValue);

  bool Has(const nsAString& aName);

  
  bool Delete(const nsAString& aName);

  void DeleteAll()
  {
    mParams.Clear();
  }

private:
  void DecodeString(const nsACString& aInput, nsAString& aOutput);
  void ConvertString(const nsACString& aInput, nsAString& aOutput);

  struct Param
  {
    nsString mKey;
    nsString mValue;
  };

  nsTArray<Param> mParams;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
};

class URLSearchParams final : public nsISupports,
                              public nsWrapperCache
{
  ~URLSearchParams();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(URLSearchParams)

  explicit URLSearchParams(URLSearchParamsObserver* aObserver);

  explicit URLSearchParams(const URLSearchParams& aOther);

  
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

  void GetAll(const nsAString& aName, nsTArray<nsString>& aRetval);

  void Set(const nsAString& aName, const nsAString& aValue);

  void Append(const nsAString& aName, const nsAString& aValue);

  bool Has(const nsAString& aName);

  void Delete(const nsAString& aName);

  void Stringify(nsString& aRetval) const
  {
    Serialize(aRetval);
  }

  typedef URLParams::ForEachIterator ForEachIterator;

  bool
  ForEach(ForEachIterator& aIterator) const
  {
    return mParams->ForEach(aIterator);

    return true;
  }

private:
  void AppendInternal(const nsAString& aName, const nsAString& aValue);

  void DeleteAll();

  void NotifyObserver();

  UniquePtr<URLParams> mParams;
  nsRefPtr<URLSearchParamsObserver> mObserver;
};

} 
} 

#endif 
