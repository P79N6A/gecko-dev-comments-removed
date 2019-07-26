



#ifndef nsFormData_h__
#define nsFormData_h__

#include "nsIDOMFormData.h"
#include "nsIXMLHttpRequest.h"
#include "nsFormSubmission.h"
#include "nsWrapperCache.h"
#include "nsTArray.h"
#include "mozilla/ErrorResult.h"

class nsHTMLFormElement;
class nsIDOMFile;

namespace mozilla {
class ErrorResult;

namespace dom {
class GlobalObject;
template<class> class Optional;
} 
} 

class nsFormData : public nsIDOMFormData,
                   public nsIXHRSendable,
                   public nsFormSubmission,
                   public nsWrapperCache
{
public:
  nsFormData(nsISupports* aOwner = nullptr);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsFormData,
                                                         nsIDOMFormData)

  NS_DECL_NSIDOMFORMDATA
  NS_DECL_NSIXHRSENDABLE

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap) MOZ_OVERRIDE;

  
  nsISupports*
  GetParentObject() const
  {
    return mOwner;
  }
  static already_AddRefed<nsFormData>
  Constructor(const mozilla::dom::GlobalObject& aGlobal,
              const mozilla::dom::Optional<nsHTMLFormElement*>& aFormElement,
              mozilla::ErrorResult& aRv);
  void Append(const nsAString& aName, const nsAString& aValue);
  void Append(const nsAString& aName, nsIDOMBlob* aBlob);

  
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream);
  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue)
  {
    Append(aName, aValue);
    return NS_OK;
  }
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIDOMBlob* aBlob)
  {
    Append(aName, aBlob);
    return NS_OK;
  }

private:
  nsCOMPtr<nsISupports> mOwner;

  struct FormDataTuple
  {
    nsString name;
    nsString stringValue;
    nsCOMPtr<nsIDOMBlob> fileValue;
    bool valueIsFile;
  };

  nsTArray<FormDataTuple> mFormData;
};

#endif 
