



































#ifndef nsIFormSubmission_h___
#define nsIFormSubmission_h___

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIURI;
class nsIInputStream;
class nsGenericHTMLElement;
class nsILinkHandler;
class nsIContent;
class nsIFormControl;
class nsIDOMHTMLElement;
class nsIDocShell;
class nsIRequest;
class nsISaveAsCharset;
class nsIMultiplexInputStream;





class nsFormSubmission
{
public:
  virtual ~nsFormSubmission()
  {
    MOZ_COUNT_DTOR(nsFormSubmission);
  }

  





  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue) = 0;

  





  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIFile* aFile) = 0;
  
  




  virtual PRBool SupportsIsindexSubmission()
  {
    return PR_FALSE;
  }

  




  virtual nsresult AddIsindex(const nsAString& aValue)
  {
    NS_NOTREACHED("AddIsindex called when not supported");
    return NS_ERROR_UNEXPECTED;
  }

  






  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream) = 0;

  


  void GetCharset(nsACString& aCharset)
  {
    aCharset = mCharset;
  }

protected:
  




  nsFormSubmission(const nsACString& aCharset)
    : mCharset(aCharset)
  {
    MOZ_COUNT_CTOR(nsFormSubmission);
  }

  
  nsCString mCharset;
};

class nsEncodingFormSubmission : public nsFormSubmission
{
public:
  nsEncodingFormSubmission(const nsACString& aCharset);

  virtual ~nsEncodingFormSubmission();

  






  nsresult EncodeVal(const nsAString& aStr, nsACString& aResult);

private:
  
  nsCOMPtr<nsISaveAsCharset> mEncoder;
};





class nsFSMultipartFormData : public nsEncodingFormSubmission
{
public:
  


  nsFSMultipartFormData(const nsACString& aCharset);
  ~nsFSMultipartFormData();
 
  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue);
  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIFile* aFile);
  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream);

  void GetContentType(nsACString& aContentType)
  {
    aContentType =
      NS_LITERAL_CSTRING("multipart/form-data; boundary=") + mBoundary;
  }

  nsIInputStream* GetSubmissionBody();

protected:

  


  nsresult AddPostDataStream();

private:
  




  nsCOMPtr<nsIMultiplexInputStream> mPostDataStream;

  






  nsCString mPostDataChunk;

  




  nsCString mBoundary;
};







nsresult GetSubmissionFromForm(nsGenericHTMLElement* aForm,
                               nsFormSubmission** aFormSubmission);

#endif 
