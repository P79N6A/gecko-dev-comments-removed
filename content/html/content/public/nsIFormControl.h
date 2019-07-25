



































#ifndef nsIFormControl_h___
#define nsIFormControl_h___

#include "nsISupports.h"
class nsIDOMHTMLFormElement;
class nsPresState;
class nsIContent;
class nsString;
class nsIFormProcessor;
class nsFormSubmission;

namespace mozilla {
namespace dom {
class Element;
} 
} 

enum FormControlsTypes {
  NS_FORM_FIELDSET = 1,
  NS_FORM_LABEL,
  NS_FORM_OUTPUT,
  NS_FORM_SELECT,
  NS_FORM_TEXTAREA,
  NS_FORM_OBJECT,
  eFormControlsWithoutSubTypesMax,
  
  
  

  
  
  
  
  NS_FORM_BUTTON_ELEMENT = 0x40, 
  NS_FORM_INPUT_ELEMENT  = 0x80  
};

enum ButtonElementTypes {
  NS_FORM_BUTTON_BUTTON = NS_FORM_BUTTON_ELEMENT + 1,
  NS_FORM_BUTTON_RESET,
  NS_FORM_BUTTON_SUBMIT,
  eButtonElementTypesMax
};

enum InputElementTypes {
  NS_FORM_INPUT_BUTTON = NS_FORM_INPUT_ELEMENT + 1,
  NS_FORM_INPUT_CHECKBOX,
  NS_FORM_INPUT_EMAIL,
  NS_FORM_INPUT_FILE,
  NS_FORM_INPUT_HIDDEN,
  NS_FORM_INPUT_RESET,
  NS_FORM_INPUT_IMAGE,
  NS_FORM_INPUT_PASSWORD,
  NS_FORM_INPUT_RADIO,
  NS_FORM_INPUT_SEARCH,
  NS_FORM_INPUT_SUBMIT,
  NS_FORM_INPUT_TEL,
  NS_FORM_INPUT_TEXT,
  eInputElementTypesMax
};

PR_STATIC_ASSERT((PRUint32)eFormControlsWithoutSubTypesMax < (PRUint32)NS_FORM_BUTTON_ELEMENT);
PR_STATIC_ASSERT((PRUint32)eButtonElementTypesMax < (PRUint32)NS_FORM_INPUT_ELEMENT);
PR_STATIC_ASSERT((PRUint32)eInputElementTypesMax  < 1<<8);

#define NS_IFORMCONTROL_IID   \
{ 0x0dc5083b, 0xb0a8, 0x48c4, \
 { 0xb2, 0xeb, 0xc2, 0x4f, 0xfb, 0x7e, 0xc2, 0x8e } }






class nsIFormControl : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMCONTROL_IID)

  



  virtual mozilla::dom::Element *GetFormElement() = 0;

  








  virtual void SetForm(nsIDOMHTMLFormElement* aForm) = 0;

  






  virtual void ClearForm(PRBool aRemoveFromForm, PRBool aNotify) = 0;

  



  NS_IMETHOD_(PRUint32) GetType() const = 0 ;

  



  NS_IMETHOD Reset() = 0;

  







  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement) = 0;

  




  NS_IMETHOD SaveState() = 0;

  








  virtual PRBool RestoreState(nsPresState* aState) = 0;

  virtual PRBool AllowDrop() = 0;

  




  virtual PRBool IsSubmitControl() const = 0;

  




  virtual PRBool IsTextControl(PRBool aExcludePassword) const = 0;

  




  virtual PRBool IsSingleLineTextControl(PRBool aExcludePassword) const = 0;

  



  virtual PRBool IsLabelableControl() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormControl, NS_IFORMCONTROL_IID)

#endif 
