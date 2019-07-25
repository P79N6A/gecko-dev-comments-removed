



































#include "nsFormData.h"
#include "nsIVariant.h"
#include "nsIDOMFileInternal.h"
#include "nsIInputStream.h"
#include "nsIFile.h"
#include "nsContentUtils.h"

nsFormData::nsFormData()
  : nsFormSubmission(NS_LITERAL_CSTRING("UTF-8"), nsnull)
{
}




DOMCI_DATA(FormData, nsFormData)

NS_IMPL_ADDREF(nsFormData)
NS_IMPL_RELEASE(nsFormData)
NS_INTERFACE_MAP_BEGIN(nsFormData)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFormData)
  NS_INTERFACE_MAP_ENTRY(nsIXHRSendable)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(FormData)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMFormData)
NS_INTERFACE_MAP_END



nsresult
nsFormData::GetEncodedSubmission(nsIURI* aURI,
                                 nsIInputStream** aPostDataStream)
{
  NS_NOTREACHED("Shouldn't call nsFormData::GetEncodedSubmission");
  return NS_OK;
}

nsresult
nsFormData::AddNameValuePair(const nsAString& aName,
                             const nsAString& aValue)
{
  FormDataTuple* data = mFormData.AppendElement();
  data->name = aName;
  data->stringValue = aValue;
  data->valueIsFile = PR_FALSE;

  return NS_OK;
}

nsresult
nsFormData::AddNameFilePair(const nsAString& aName,
                            nsIFile* aFile)
{
  FormDataTuple* data = mFormData.AppendElement();
  data->name = aName;
  data->fileValue = aFile;
  data->valueIsFile = PR_TRUE;

  return NS_OK;
}




NS_IMETHODIMP
nsFormData::Append(const nsAString& aName, nsIVariant* aValue)
{
  PRUint16 dataType;
  nsresult rv = aValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dataType == nsIDataType::VTYPE_INTERFACE ||
      dataType == nsIDataType::VTYPE_INTERFACE_IS) {
    nsCOMPtr<nsISupports> supports;
    nsID *iid;
    rv = aValue->GetAsInterface(&iid, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsMemory::Free(iid);

    nsCOMPtr<nsIDOMFileInternal> domFile = do_QueryInterface(supports);
    if (domFile) {
      nsCOMPtr<nsIFile> file;
      rv = domFile->GetInternalFile(getter_AddRefs(file));
      NS_ENSURE_SUCCESS(rv, rv);

      return AddNameFilePair(aName, file);
    }
  }

  PRUnichar* stringData = nsnull;
  PRUint32 stringLen = 0;
  rv = aValue->GetAsWStringWithSize(&stringLen, &stringData);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString valAsString;
  valAsString.Adopt(stringData, stringLen);

  return AddNameValuePair(aName, valAsString);
}




NS_IMETHODIMP
nsFormData::GetSendInfo(nsIInputStream** aBody, nsACString& aContentType,
                        nsACString& aCharset)
{
  nsFSMultipartFormData fs(NS_LITERAL_CSTRING("UTF-8"), nsnull);
  
  for (PRUint32 i = 0; i < mFormData.Length(); ++i) {
    if (mFormData[i].valueIsFile) {
      fs.AddNameFilePair(mFormData[i].name, mFormData[i].fileValue);
    }
    else {
      fs.AddNameValuePair(mFormData[i].name, mFormData[i].stringValue);
    }
  }

  fs.GetContentType(aContentType);
  aCharset.Truncate();
  NS_ADDREF(*aBody = fs.GetSubmissionBody());

  return NS_OK;
}
