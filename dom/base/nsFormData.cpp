





#include "nsFormData.h"
#include "nsIVariant.h"
#include "nsIInputStream.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/HTMLFormElement.h"

#include "MultipartBlobImpl.h"

using namespace mozilla;
using namespace mozilla::dom;

nsFormData::nsFormData(nsISupports* aOwner)
  : nsFormSubmission(NS_LITERAL_CSTRING("UTF-8"), nullptr)
  , mOwner(aOwner)
{
}

namespace {


already_AddRefed<File>
CreateNewFileInstance(Blob& aBlob, const Optional<nsAString>& aFilename)
{
  
  
  
  
  
  
  nsAutoString filename;
  if (aFilename.WasPassed()) {
    filename = aFilename.Value();
  } else {
    
    
    nsRefPtr<File> file = aBlob.ToFile();
    if (file) {
      return file.forget();
    }

    filename = NS_LITERAL_STRING("blob");
  }

  return aBlob.ToFile(filename);
}

} 




NS_IMPL_CYCLE_COLLECTION_CLASS(nsFormData)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFormData)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOwner)

  for (uint32_t i = 0, len = tmp->mFormData.Length(); i < len; ++i) {
    ImplCycleCollectionUnlink(tmp->mFormData[i].fileValue);
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFormData)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)

  for (uint32_t i = 0, len = tmp->mFormData.Length(); i < len; ++i) {
   ImplCycleCollectionTraverse(cb,tmp->mFormData[i].fileValue,
                               "mFormData[i].fileValue", 0);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(nsFormData)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFormData)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFormData)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFormData)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMFormData)
  NS_INTERFACE_MAP_ENTRY(nsIXHRSendable)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMFormData)
NS_INTERFACE_MAP_END



nsresult
nsFormData::GetEncodedSubmission(nsIURI* aURI,
                                 nsIInputStream** aPostDataStream)
{
  NS_NOTREACHED("Shouldn't call nsFormData::GetEncodedSubmission");
  return NS_OK;
}

void
nsFormData::Append(const nsAString& aName, const nsAString& aValue)
{
  AddNameValuePair(aName, aValue);
}

void
nsFormData::Append(const nsAString& aName, Blob& aBlob,
                   const Optional<nsAString>& aFilename)
{
  nsRefPtr<File> file = CreateNewFileInstance(aBlob, aFilename);
  AddNameFilePair(aName, file);
}

void
nsFormData::Delete(const nsAString& aName)
{
  
  
  for (uint32_t i = mFormData.Length(); i-- > 0; ) {
    if (aName.Equals(mFormData[i].name)) {
      mFormData.RemoveElementAt(i);
    }
  }
}

void
nsFormData::ExtractValue(const FormDataTuple& aTuple,
                         OwningFileOrUSVString* aOutValue)
{
  if (aTuple.valueIsFile) {
    aOutValue->SetAsFile() = aTuple.fileValue;
  } else {
    aOutValue->SetAsUSVString() = aTuple.stringValue;
  }
}

void
nsFormData::Get(const nsAString& aName,
                Nullable<OwningFileOrUSVString>& aOutValue)
{
  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (aName.Equals(mFormData[i].name)) {
      ExtractValue(mFormData[i], &aOutValue.SetValue());
      return;
    }
  }

  aOutValue.SetNull();
}

void
nsFormData::GetAll(const nsAString& aName,
                   nsTArray<OwningFileOrUSVString>& aValues)
{
  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (aName.Equals(mFormData[i].name)) {
      OwningFileOrUSVString* element = aValues.AppendElement();
      ExtractValue(mFormData[i], element);
    }
  }
}

bool
nsFormData::Has(const nsAString& aName)
{
  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (aName.Equals(mFormData[i].name)) {
      return true;
    }
  }

  return false;
}

nsresult
nsFormData::AddNameFilePair(const nsAString& aName, File* aFile)
{
  FormDataTuple* data = mFormData.AppendElement();
  SetNameFilePair(data, aName, aFile);
  return NS_OK;
}

nsFormData::FormDataTuple*
nsFormData::RemoveAllOthersAndGetFirstFormDataTuple(const nsAString& aName)
{
  FormDataTuple* lastFoundTuple = nullptr;
  uint32_t lastFoundIndex = mFormData.Length();
  
  
  for (uint32_t i = mFormData.Length(); i-- > 0; ) {
    if (aName.Equals(mFormData[i].name)) {
      if (lastFoundTuple) {
        
        mFormData.RemoveElementAt(lastFoundIndex);
      }

      lastFoundTuple = &mFormData[i];
      lastFoundIndex = i;
    }
  }

  return lastFoundTuple;
}

void
nsFormData::Set(const nsAString& aName, Blob& aBlob,
                const Optional<nsAString>& aFilename)
{
  FormDataTuple* tuple = RemoveAllOthersAndGetFirstFormDataTuple(aName);
  if (tuple) {
    nsRefPtr<File> file = CreateNewFileInstance(aBlob, aFilename);
    SetNameFilePair(tuple, aName, file);
  } else {
    Append(aName, aBlob, aFilename);
  }
}

void
nsFormData::Set(const nsAString& aName, const nsAString& aValue)
{
  FormDataTuple* tuple = RemoveAllOthersAndGetFirstFormDataTuple(aName);
  if (tuple) {
    SetNameValuePair(tuple, aName, aValue);
  } else {
    Append(aName, aValue);
  }
}




NS_IMETHODIMP
nsFormData::Append(const nsAString& aName, nsIVariant* aValue)
{
  uint16_t dataType;
  nsresult rv = aValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dataType == nsIDataType::VTYPE_INTERFACE ||
      dataType == nsIDataType::VTYPE_INTERFACE_IS) {
    nsCOMPtr<nsISupports> supports;
    nsID *iid;
    rv = aValue->GetAsInterface(&iid, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    free(iid);

    nsCOMPtr<nsIDOMBlob> domBlob = do_QueryInterface(supports);
    nsRefPtr<Blob> blob = static_cast<Blob*>(domBlob.get());
    if (domBlob) {
      Optional<nsAString> temp;
      Append(aName, *blob, temp);
      return NS_OK;
    }
  }

  char16_t* stringData = nullptr;
  uint32_t stringLen = 0;
  rv = aValue->GetAsWStringWithSize(&stringLen, &stringData);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString valAsString;
  valAsString.Adopt(stringData, stringLen);

  Append(aName, valAsString);
  return NS_OK;
}

 JSObject*
nsFormData::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FormDataBinding::Wrap(aCx, this, aGivenProto);
}

 already_AddRefed<nsFormData>
nsFormData::Constructor(const GlobalObject& aGlobal,
                        const Optional<NonNull<HTMLFormElement> >& aFormElement,
                        ErrorResult& aRv)
{
  nsRefPtr<nsFormData> formData = new nsFormData(aGlobal.GetAsSupports());
  if (aFormElement.WasPassed()) {
    aRv = aFormElement.Value().WalkFormElements(formData);
  }
  return formData.forget();
}




NS_IMETHODIMP
nsFormData::GetSendInfo(nsIInputStream** aBody, uint64_t* aContentLength,
                        nsACString& aContentType, nsACString& aCharset)
{
  nsFSMultipartFormData fs(NS_LITERAL_CSTRING("UTF-8"), nullptr);

  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (mFormData[i].valueIsFile) {
      fs.AddNameFilePair(mFormData[i].name, mFormData[i].fileValue);
    }
    else {
      fs.AddNameValuePair(mFormData[i].name, mFormData[i].stringValue);
    }
  }

  fs.GetContentType(aContentType);
  aCharset.Truncate();
  *aContentLength = 0;
  NS_ADDREF(*aBody = fs.GetSubmissionBody(aContentLength));

  return NS_OK;
}
