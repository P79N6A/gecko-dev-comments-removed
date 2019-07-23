



































#ifndef nsIFormSubmission_h___
#define nsIFormSubmission_h___

#include "nsISupports.h"
class nsAString;
class nsACString;
class nsIURI;
class nsIInputStream;
class nsGenericHTMLElement;
class nsILinkHandler;
class nsIContent;
class nsIFormControl;
class nsIDOMHTMLElement;
class nsIDocShell;
class nsIRequest;

#define NS_IFORMSUBMISSION_IID   \
{ 0x7ee38e3a, 0x1dd2, 0x11b2, \
  {0x89, 0x6f, 0xab, 0x28, 0x03, 0x96, 0x25, 0xa9} }





class nsIFormSubmission : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMSUBMISSION_IID)

  




  virtual PRBool AcceptsFiles() const = 0;

  










  virtual nsresult SubmitTo(nsIURI* aActionURL, const nsAString& aTarget,
                            nsIContent* aSource, nsILinkHandler* aLinkHandler,
                            nsIDocShell** aDocShell,
                            nsIRequest** aRequest) = 0;

  






  virtual nsresult AddNameValuePair(nsIDOMHTMLElement* aSource,
                                    const nsAString& aName,
                                    const nsAString& aValue) = 0;

  










  virtual nsresult AddNameFilePair(nsIDOMHTMLElement* aSource,
                               const nsAString& aName,
                               const nsAString& aFilename,
                               nsIInputStream* aStream,
                               const nsACString& aContentType,
                               PRBool aMoreFilesToCome) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormSubmission, NS_IFORMSUBMISSION_IID)











nsresult GetSubmissionFromForm(nsGenericHTMLElement* aForm,
                               nsIFormSubmission** aFormSubmission);


#endif 
