



#ifndef nsIForm_h___
#define nsIForm_h___

#include "nsISupports.h"
#include "nsAString.h"

class nsIFormControl;

#define NS_FORM_METHOD_GET  0
#define NS_FORM_METHOD_POST 1
#define NS_FORM_ENCTYPE_URLENCODED 0
#define NS_FORM_ENCTYPE_MULTIPART  1
#define NS_FORM_ENCTYPE_TEXTPLAIN  2


#define NS_IFORM_IID \
{ 0x5e8464c8, 0x015d, 0x4cf9, \
  { 0x92, 0xc9, 0xa6, 0xb3, 0x30, 0x8f, 0x60, 0x9d } }






class nsIForm : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORM_IID)

  






  NS_IMETHOD_(nsIFormControl*) GetElementAt(int32_t aIndex) const = 0;

  





  NS_IMETHOD_(uint32_t) GetElementCount() const = 0;

  




  NS_IMETHOD_(int32_t) IndexOfControl(nsIFormControl* aControl) = 0;

  



   NS_IMETHOD_(nsIFormControl*) GetDefaultSubmitElement() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIForm, NS_IFORM_IID)

#endif 
