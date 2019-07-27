





#ifndef mozilla_dom_PerformanceEntry_h___
#define mozilla_dom_PerformanceEntry_h___

#include "nsDOMNavigationTiming.h"
#include "nsString.h"
#include "nsWrapperCache.h"

class nsISupports;

namespace mozilla {
namespace dom {


class PerformanceEntry : public nsISupports,
                         public nsWrapperCache
{
protected:
  virtual ~PerformanceEntry();

public:
  PerformanceEntry(nsISupports* aParent,
                   const nsAString& aName,
                   const nsAString& aEntryType);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(PerformanceEntry)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  void GetName(nsAString& aName) const
  {
    aName = mName;
  }

  const nsAString& GetName() const
  {
    return mName;
  }

  void SetName(const nsAString& aName)
  {
    mName = aName;
  }

  void GetEntryType(nsAString& aEntryType) const
  {
    aEntryType = mEntryType;
  }

  const nsAString& GetEntryType()
  {
    return mEntryType;
  }

  void SetEntryType(const nsAString& aEntryType)
  {
    mEntryType = aEntryType;
  }

  virtual DOMHighResTimeStamp StartTime() const
  {
    return 0;
  }

  virtual DOMHighResTimeStamp Duration() const
  {
    return 0;
  }

protected:
  nsCOMPtr<nsISupports> mParent;
  nsString mName;
  nsString mEntryType;
};

} 
} 

#endif 
