



































#ifndef nsIForm_h___
#define nsIForm_h___

#include "nsISupports.h"
#include "nsAString.h"

class nsIFormControl;
class nsISimpleEnumerator;
class nsIURI;
template<class T> class nsTArray;

#define NS_FORM_METHOD_GET  0
#define NS_FORM_METHOD_POST 1
#define NS_FORM_ENCTYPE_URLENCODED 0
#define NS_FORM_ENCTYPE_MULTIPART  1
#define NS_FORM_ENCTYPE_TEXTPLAIN  2


#define NS_IFORM_IID    \
{ 0xd4ffda0b, 0x2396, 0x4e64, \
  {0x97, 0x5c, 0x73, 0xab, 0x56, 0x4b, 0x14, 0x77} }







class nsIForm : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORM_IID)

  






  NS_IMETHOD AddElement(nsIFormControl* aElement,
                        PRBool aNotify) = 0;

  







  NS_IMETHOD AddElementToTable(nsIFormControl* aElement,
                               const nsAString& aName) = 0;

  






  NS_IMETHOD GetElementAt(PRInt32 aIndex, nsIFormControl** aElement) const = 0;

  





  NS_IMETHOD GetElementCount(PRUint32* aCount) const = 0;

  






  NS_IMETHOD RemoveElement(nsIFormControl* aElement,
                           PRBool aNotify) = 0;

  










  NS_IMETHOD RemoveElementFromTable(nsIFormControl* aElement,
                                    const nsAString& aName) = 0;

  








  NS_IMETHOD ResolveName(const nsAString& aName,
                         nsISupports **aResult) = 0;

  




  NS_IMETHOD IndexOfControl(nsIFormControl* aControl, PRInt32* aIndex) = 0;

  



 
  NS_IMETHOD OnSubmitClickBegin() = 0;
  NS_IMETHOD OnSubmitClickEnd() = 0;

  





  NS_IMETHOD FlushPendingSubmission() = 0;
  





  NS_IMETHOD ForgetPendingSubmission() = 0;

  




  NS_IMETHOD GetActionURL(nsIURI** aActionURL) = 0;

  








  NS_IMETHOD GetSortedControls(nsTArray<nsIFormControl*>& aControls) const = 0;

  



   NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const = 0;

   




   NS_IMETHOD_(PRBool) HasSingleTextControl() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIForm, NS_IFORM_IID)

#endif 
