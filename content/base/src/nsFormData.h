



































#ifndef nsFormData_h__
#define nsFormData_h__

#include "nsIDOMFormData.h"
#include "nsIXMLHttpRequest.h"
#include "nsFormSubmission.h"
#include "nsTArray.h"

class nsIDOMFile;

class nsFormData : public nsIDOMFormData,
                   public nsIXHRSendable,
                   public nsFormSubmission
{
public:
  nsFormData();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFORMDATA
  NS_DECL_NSIXHRSENDABLE

  
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream);
  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIDOMFile* aFile);

private:
  struct FormDataTuple
  {
    nsString name;
    nsString stringValue;
    nsCOMPtr<nsIDOMFile> fileValue;
    PRBool valueIsFile;
  };
  
  nsTArray<FormDataTuple> mFormData;
};

#endif 
