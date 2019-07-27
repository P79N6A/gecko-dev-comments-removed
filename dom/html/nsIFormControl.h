



#ifndef nsIFormControl_h___
#define nsIFormControl_h___

#include "nsISupports.h"
class nsIDOMHTMLFormElement;
class nsPresState;
class nsFormSubmission;

namespace mozilla {
namespace dom {
class Element;
class HTMLFieldSetElement;
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
  NS_FORM_INPUT_COLOR,
  NS_FORM_INPUT_DATE,
  NS_FORM_INPUT_EMAIL,
  NS_FORM_INPUT_FILE,
  NS_FORM_INPUT_HIDDEN,
  NS_FORM_INPUT_RESET,
  NS_FORM_INPUT_IMAGE,
  NS_FORM_INPUT_NUMBER,
  NS_FORM_INPUT_PASSWORD,
  NS_FORM_INPUT_RADIO,
  NS_FORM_INPUT_SEARCH,
  NS_FORM_INPUT_SUBMIT,
  NS_FORM_INPUT_TEL,
  NS_FORM_INPUT_TEXT,
  NS_FORM_INPUT_TIME,
  NS_FORM_INPUT_URL,
  NS_FORM_INPUT_RANGE,
  eInputElementTypesMax
};

static_assert(static_cast<uint32_t>(eFormControlsWithoutSubTypesMax) <
              static_cast<uint32_t>(NS_FORM_BUTTON_ELEMENT),
              "Too many FormControlsTypes without sub-types");
static_assert(static_cast<uint32_t>(eButtonElementTypesMax) <
              static_cast<uint32_t>(NS_FORM_INPUT_ELEMENT),
              "Too many ButtonElementTypes");
static_assert(static_cast<uint32_t>(eInputElementTypesMax) < 1<<8,
              "Too many form control types");

#define NS_IFORMCONTROL_IID   \
{ 0x4b89980c, 0x4dcd, 0x428f, \
  { 0xb7, 0xad, 0x43, 0x5b, 0x93, 0x29, 0x79, 0xec } }






class nsIFormControl : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMCONTROL_IID)

  



  virtual mozilla::dom::HTMLFieldSetElement *GetFieldSet() = 0;

  



  virtual mozilla::dom::Element *GetFormElement() = 0;

  








  virtual void SetForm(nsIDOMHTMLFormElement* aForm) = 0;

  





  virtual void ClearForm(bool aRemoveFromForm) = 0;

  



  NS_IMETHOD_(uint32_t) GetType() const = 0 ;

  



  NS_IMETHOD Reset() = 0;

  





  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) = 0;

  




  NS_IMETHOD SaveState() = 0;

  








  virtual bool RestoreState(nsPresState* aState) = 0;

  virtual bool AllowDrop() = 0;

  




  inline bool IsSubmitControl() const;

  




  inline bool IsTextControl(bool aExcludePassword) const ;

  




  inline bool IsSingleLineTextControl(bool aExcludePassword) const;

  



  inline bool IsSubmittableControl() const;

  



  inline bool AllowDraggableChildren() const;

  virtual bool IsDisabledForEvents(uint32_t aMessage)
  {
    return false;
  }
protected:

  





  inline static bool IsSingleLineTextControl(bool aExcludePassword, uint32_t aType);

  



  inline bool IsAutofocusable() const;
};

bool
nsIFormControl::IsSubmitControl() const
{
  uint32_t type = GetType();
  return type == NS_FORM_INPUT_SUBMIT ||
         type == NS_FORM_INPUT_IMAGE ||
         type == NS_FORM_BUTTON_SUBMIT;
}

bool
nsIFormControl::IsTextControl(bool aExcludePassword) const
{
  uint32_t type = GetType();
  return type == NS_FORM_TEXTAREA ||
         IsSingleLineTextControl(aExcludePassword, type);
}

bool
nsIFormControl::IsSingleLineTextControl(bool aExcludePassword) const
{
  return IsSingleLineTextControl(aExcludePassword, GetType());
}


bool
nsIFormControl::IsSingleLineTextControl(bool aExcludePassword, uint32_t aType)
{
  return aType == NS_FORM_INPUT_TEXT ||
         aType == NS_FORM_INPUT_EMAIL ||
         aType == NS_FORM_INPUT_SEARCH ||
         aType == NS_FORM_INPUT_TEL ||
         aType == NS_FORM_INPUT_URL ||
         
         aType == NS_FORM_INPUT_DATE ||
         aType == NS_FORM_INPUT_TIME ||
         (!aExcludePassword && aType == NS_FORM_INPUT_PASSWORD);
}

bool
nsIFormControl::IsSubmittableControl() const
{
  
  uint32_t type = GetType();
  return type == NS_FORM_OBJECT ||
         type == NS_FORM_TEXTAREA ||
         type == NS_FORM_SELECT ||
         
         type & NS_FORM_BUTTON_ELEMENT ||
         type & NS_FORM_INPUT_ELEMENT;
}

bool
nsIFormControl::AllowDraggableChildren() const
{
  uint32_t type = GetType();
  return type == NS_FORM_OBJECT ||
         type == NS_FORM_LABEL ||
         type == NS_FORM_FIELDSET ||
         type == NS_FORM_OUTPUT;
}

bool
nsIFormControl::IsAutofocusable() const
{
  uint32_t type = GetType();
  return type & NS_FORM_INPUT_ELEMENT ||
         type & NS_FORM_BUTTON_ELEMENT ||
         type == NS_FORM_TEXTAREA ||
         type == NS_FORM_SELECT;
}

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormControl, NS_IFORMCONTROL_IID)

#endif 
