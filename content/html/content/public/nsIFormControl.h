



































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
  NS_FORM_PROGRESS,
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
  NS_FORM_INPUT_URL,
  eInputElementTypesMax
};

PR_STATIC_ASSERT((PRUint32)eFormControlsWithoutSubTypesMax < (PRUint32)NS_FORM_BUTTON_ELEMENT);
PR_STATIC_ASSERT((PRUint32)eButtonElementTypesMax < (PRUint32)NS_FORM_INPUT_ELEMENT);
PR_STATIC_ASSERT((PRUint32)eInputElementTypesMax  < 1<<8);

#define NS_IFORMCONTROL_IID   \
{ 0x671ef379, 0x7ac0, 0x414c, \
 { 0xa2, 0x2b, 0xc1, 0x9e, 0x0b, 0x61, 0x4e, 0x83 } }






class nsIFormControl : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMCONTROL_IID)

  



  virtual mozilla::dom::Element *GetFormElement() = 0;

  








  virtual void SetForm(nsIDOMHTMLFormElement* aForm) = 0;

  





  virtual void ClearForm(PRBool aRemoveFromForm) = 0;

  



  NS_IMETHOD_(PRUint32) GetType() const = 0 ;

  



  NS_IMETHOD Reset() = 0;

  





  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) = 0;

  




  NS_IMETHOD SaveState() = 0;

  








  virtual PRBool RestoreState(nsPresState* aState) = 0;

  virtual PRBool AllowDrop() = 0;

  




  inline PRBool IsSubmitControl() const;

  




  inline PRBool IsTextControl(PRBool aExcludePassword) const ;

  




  inline PRBool IsSingleLineTextControl(PRBool aExcludePassword) const;

  



  inline PRBool IsLabelableControl() const;

  



  inline PRBool IsSubmittableControl() const;

  



  inline PRBool AllowDraggableChildren() const;

protected:

  





  inline static bool IsSingleLineTextControl(bool aExcludePassword, PRUint32 aType);

  



  inline bool IsAutofocusable() const;
};

PRBool
nsIFormControl::IsSubmitControl() const
{
  PRUint32 type = GetType();
  return type == NS_FORM_INPUT_SUBMIT ||
         type == NS_FORM_INPUT_IMAGE ||
         type == NS_FORM_BUTTON_SUBMIT;
}

PRBool
nsIFormControl::IsTextControl(PRBool aExcludePassword) const
{
  PRUint32 type = GetType();
  return type == NS_FORM_TEXTAREA ||
         IsSingleLineTextControl(aExcludePassword, type);
}

PRBool
nsIFormControl::IsSingleLineTextControl(PRBool aExcludePassword) const
{
  return IsSingleLineTextControl(aExcludePassword, GetType());
}


bool
nsIFormControl::IsSingleLineTextControl(bool aExcludePassword, PRUint32 aType)
{
  return aType == NS_FORM_INPUT_TEXT ||
         aType == NS_FORM_INPUT_EMAIL ||
         aType == NS_FORM_INPUT_SEARCH ||
         aType == NS_FORM_INPUT_TEL ||
         aType == NS_FORM_INPUT_URL ||
         (!aExcludePassword && aType == NS_FORM_INPUT_PASSWORD);
}

PRBool
nsIFormControl::IsLabelableControl() const
{
  
  
  
  PRUint32 type = GetType();
  return type & NS_FORM_INPUT_ELEMENT ||
         type & NS_FORM_BUTTON_ELEMENT ||
         
         
         type == NS_FORM_OUTPUT ||
         type == NS_FORM_PROGRESS ||
         type == NS_FORM_SELECT ||
         type == NS_FORM_TEXTAREA;
}

PRBool
nsIFormControl::IsSubmittableControl() const
{
  
  PRUint32 type = GetType();
  return type == NS_FORM_OBJECT ||
         type == NS_FORM_TEXTAREA ||
         type == NS_FORM_SELECT ||
         
         type & NS_FORM_BUTTON_ELEMENT ||
         type & NS_FORM_INPUT_ELEMENT;
}

PRBool
nsIFormControl::AllowDraggableChildren() const
{
  PRUint32 type = GetType();
  return type == NS_FORM_OBJECT ||
         type == NS_FORM_LABEL ||
         type == NS_FORM_FIELDSET ||
         type == NS_FORM_OUTPUT;
}

bool
nsIFormControl::IsAutofocusable() const
{
  PRUint32 type = GetType();
  return type & NS_FORM_INPUT_ELEMENT ||
         type & NS_FORM_BUTTON_ELEMENT ||
         type == NS_FORM_TEXTAREA ||
         type == NS_FORM_SELECT;
}

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormControl, NS_IFORMCONTROL_IID)

#endif 
