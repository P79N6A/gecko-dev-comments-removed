



































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





class nsFormSubmission {

public:
  virtual ~nsFormSubmission();

  





  virtual nsresult AddNameValuePair(const nsAString& aName,
                                    const nsAString& aValue) = 0;

  





  virtual nsresult AddNameFilePair(const nsAString& aName,
                                   nsIFile* aFile) = 0;
  
  






  virtual nsresult GetEncodedSubmission(nsIURI* aURI,
                                        nsIInputStream** aPostDataStream) = 0;

  


  void GetCharset(nsACString& aCharset)
  {
    aCharset = mCharset;
  }

protected:
  





  nsFormSubmission(const nsACString& aCharset,
                   PRInt32 aBidiOptions);

  






  nsresult EncodeVal(const nsAString& aStr, nsACString& aResult);

  






  nsresult UnicodeToNewBytes(const nsAString& aStr, nsACString& aOut);

  
  nsCString mCharset;
  
  nsCOMPtr<nsISaveAsCharset> mEncoder;
  
  PRInt32 mBidiOptions;
};








nsresult GetSubmissionFromForm(nsGenericHTMLElement* aForm,
                               nsFormSubmission** aFormSubmission);


#endif 
