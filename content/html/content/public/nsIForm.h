



































#ifndef nsIForm_h___
#define nsIForm_h___

#include "nsISupports.h"
#include "nsAString.h"

class nsIFormControl;
class nsISimpleEnumerator;
class nsIURI;

#define NS_FORM_METHOD_GET  0
#define NS_FORM_METHOD_POST 1
#define NS_FORM_ENCTYPE_URLENCODED 0
#define NS_FORM_ENCTYPE_MULTIPART  1
#define NS_FORM_ENCTYPE_TEXTPLAIN  2


#define NS_IFORM_IID    \
{ 0x27f1ff6c, 0xeb78, 0x405b, \
 { 0xa6, 0xeb, 0xf0, 0xce, 0xa8, 0x30, 0x85, 0x58 } }






class nsIForm : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORM_IID)

  






  NS_IMETHOD_(nsIFormControl*) GetElementAt(PRInt32 aIndex) const = 0;

  





  NS_IMETHOD_(PRUint32) GetElementCount() const = 0;

  








  NS_IMETHOD_(already_AddRefed<nsISupports>) ResolveName(const nsAString& aName) = 0;

  




  NS_IMETHOD_(PRInt32) IndexOfControl(nsIFormControl* aControl) = 0;

  



   NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIForm, NS_IFORM_IID)

#endif 
