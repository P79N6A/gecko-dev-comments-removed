



































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
{ 0x6e8456c2, 0xcf49, 0x4b6d, \
 { 0xb5, 0xfe, 0x80, 0x0d, 0x03, 0x4f, 0x55, 0x33 } }







class nsIForm : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORM_IID)

  






  NS_IMETHOD AddElement(nsIFormControl* aElement,
                        PRBool aNotify) = 0;

  







  NS_IMETHOD AddElementToTable(nsIFormControl* aElement,
                               const nsAString& aName) = 0;

  






  NS_IMETHOD GetElementAt(PRInt32 aIndex, nsIFormControl** aElement) const = 0;

  





  NS_IMETHOD_(PRUint32) GetElementCount() const = 0;

  






  NS_IMETHOD RemoveElement(nsIFormControl* aElement,
                           PRBool aNotify) = 0;

  










  NS_IMETHOD RemoveElementFromTable(nsIFormControl* aElement,
                                    const nsAString& aName) = 0;

  








  NS_IMETHOD_(already_AddRefed<nsISupports>) ResolveName(const nsAString& aName) = 0;

  




  NS_IMETHOD_(PRInt32) IndexOfControl(nsIFormControl* aControl) = 0;

  



 
  NS_IMETHOD OnSubmitClickBegin() = 0;
  NS_IMETHOD OnSubmitClickEnd() = 0;

  





  NS_IMETHOD FlushPendingSubmission() = 0;
  





  NS_IMETHOD ForgetPendingSubmission() = 0;

  




  NS_IMETHOD GetActionURL(nsIURI** aActionURL) = 0;

  








  NS_IMETHOD GetSortedControls(nsTArray<nsIFormControl*>& aControls) const = 0;

  



   NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const = 0;

  






  NS_IMETHOD_(PRBool) IsDefaultSubmitElement(const nsIFormControl* aControl) const = 0;

   




   NS_IMETHOD_(PRBool) HasSingleTextControl() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIForm, NS_IFORM_IID)

#endif 
