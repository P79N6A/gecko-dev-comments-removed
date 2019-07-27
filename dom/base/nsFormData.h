





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
#include "mozilla/dom/File.h"
#include "mozilla/dom/FormDataBinding.h"

namespace mozilla {
class ErrorResult;

namespace dom {
class HTMLFormElement;
class GlobalObject;
} 
} 

class nsFormData final : public nsIDOMFormData,
                         public nsIXHRSendable,
                         public nsFormSubmission,
                         public nsWrapperCache
{
private:
  ~nsFormData() {}

  typedef mozilla::dom::Blob Blob;
  typedef mozilla::dom::File File;

  struct FormDataTuple
  {
    nsString name;
    nsString stringValue;
    nsRefPtr<File> fileValue;
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
                       File* aFile)
  {
    MOZ_ASSERT(aData);
    aData->name = aName;
    aData->fileValue = aFile;
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

  
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
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
  void Append(const nsAString& aName, Blob& aBlob,
              const mozilla::dom::Optional<nsAString>& aFilename);
  void Delete(const nsAString& aName);
  void Get(const nsAString& aName, mozilla::dom::Nullable<mozilla::dom::OwningFileOrUSVString>& aOutValue);
  void GetAll(const nsAString& aName, nsTArray<mozilla::dom::OwningFileOrUSVString>& aValues);
  bool Has(const nsAString& aName);
  void Set(const nsAString& aName, Blob& aBlob,
           const mozilla::dom::Optional<nsAString>& aFilename);
  void Set(const nsAString& aName, const nsAString& aValue);

  
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream) override;
  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue) override
  {
    FormDataTuple* data = mFormData.AppendElement();
    SetNameValuePair(data, aName, aValue);
    return NS_OK;
  }
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   File* aFile) override;

  typedef bool (*FormDataEntryCallback)(const nsString& aName, bool aIsFile,
                                        const nsString& aValue,
                                        File* aFile, void* aClosure);

  uint32_t
  Length() const
  {
    return mFormData.Length();
  }

  
  
  bool
  ForEach(FormDataEntryCallback aFunc, void* aClosure)
  {
    for (uint32_t i = 0; i < mFormData.Length(); ++i) {
      FormDataTuple& tuple = mFormData[i];
      if (!aFunc(tuple.name, tuple.valueIsFile, tuple.stringValue,
                 tuple.fileValue, aClosure)) {
        return false;
      }
    }

    return true;
  }

private:
  nsCOMPtr<nsISupports> mOwner;

  nsTArray<FormDataTuple> mFormData;
};

#endif 
