



#ifndef nsFormData_h__
#define nsFormData_h__

#include "mozilla/Attributes.h"
#include "nsIDOMFormData.h"
#include "nsIXMLHttpRequest.h"
#include "nsFormSubmission.h"
#include "nsWrapperCache.h"
#include "nsTArray.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/FormDataBinding.h"

namespace mozilla {
class ErrorResult;

namespace dom {
class File;
class HTMLFormElement;
class GlobalObject;
} 
} 

class nsFormData MOZ_FINAL : public nsIDOMFormData,
                             public nsIXHRSendable,
                             public nsFormSubmission,
                             public nsWrapperCache
{
private:
  ~nsFormData() {}

  typedef mozilla::dom::File File;
  struct FormDataTuple
  {
    nsString name;
    nsString stringValue;
    nsRefPtr<File> fileValue;
    nsString filename;
    bool valueIsFile;
  };

  
  
  FormDataTuple*
  RemoveAllOthersAndGetFirstFormDataTuple(const nsAString& aName);

  void SetNameValuePair(FormDataTuple* aData,
                        const nsAString& aName,
                        const nsAString& aValue)
  {
    MOZ_ASSERT(aData);
    aData->name = aName;
    aData->stringValue = aValue;
    aData->valueIsFile = false;
  }

  void SetNameFilePair(FormDataTuple* aData,
                       const nsAString& aName,
                       File* aBlob,
                       const nsAString& aFilename)
  {
    MOZ_ASSERT(aData);
    aData->name = aName;
    aData->fileValue = aBlob;
    aData->filename = aFilename;
    aData->valueIsFile = true;
  }

  void ExtractValue(const FormDataTuple& aTuple,
                    mozilla::dom::OwningFileOrUSVString* aOutValue);
public:
  explicit nsFormData(nsISupports* aOwner = nullptr);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsFormData,
                                                         nsIDOMFormData)

  NS_DECL_NSIDOMFORMDATA
  NS_DECL_NSIXHRSENDABLE

  
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  nsISupports*
  GetParentObject() const
  {
    return mOwner;
  }
  static already_AddRefed<nsFormData>
  Constructor(const mozilla::dom::GlobalObject& aGlobal,
              const mozilla::dom::Optional<mozilla::dom::NonNull<mozilla::dom::HTMLFormElement> >& aFormElement,
              mozilla::ErrorResult& aRv);
  void Append(const nsAString& aName, const nsAString& aValue);
  void Append(const nsAString& aName, File& aBlob,
              const mozilla::dom::Optional<nsAString>& aFilename);
  void Delete(const nsAString& aName);
  void Get(const nsAString& aName, mozilla::dom::Nullable<mozilla::dom::OwningFileOrUSVString>& aOutValue);
  void GetAll(const nsAString& aName, nsTArray<mozilla::dom::OwningFileOrUSVString>& aValues);
  bool Has(const nsAString& aName);
  void Set(const nsAString& aName, File& aBlob,
           const mozilla::dom::Optional<nsAString>& aFilename);
  void Set(const nsAString& aName, const nsAString& aValue);

  
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream) MOZ_OVERRIDE;
  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue) MOZ_OVERRIDE
  {
    FormDataTuple* data = mFormData.AppendElement();
    SetNameValuePair(data, aName, aValue);
    return NS_OK;
  }
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   File* aBlob,
                                   const nsString& aFilename) MOZ_OVERRIDE
  {
    FormDataTuple* data = mFormData.AppendElement();
    SetNameFilePair(data, aName, aBlob, aFilename);
    return NS_OK;
  }
private:
  nsCOMPtr<nsISupports> mOwner;

  nsTArray<FormDataTuple> mFormData;
};

#endif 
