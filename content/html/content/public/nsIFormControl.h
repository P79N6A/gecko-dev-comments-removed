



































#ifndef nsIFormControl_h___
#define nsIFormControl_h___

#include "nsISupports.h"
class nsIDOMHTMLFormElement;
class nsPresContext;
class nsPresState;
class nsIContent;
class nsString;
class nsIFormProcessor;
class nsIFormSubmission;

#define NS_FORM_BUTTON_BUTTON   1
#define NS_FORM_BUTTON_RESET    2
#define NS_FORM_BUTTON_SUBMIT   3
#define NS_FORM_FIELDSET        4
#define NS_FORM_INPUT_BUTTON    5
#define NS_FORM_INPUT_CHECKBOX  6
#define NS_FORM_INPUT_FILE      7
#define NS_FORM_INPUT_HIDDEN    8
#define NS_FORM_INPUT_RESET     9
#define NS_FORM_INPUT_IMAGE    10
#define NS_FORM_INPUT_PASSWORD 11
#define NS_FORM_INPUT_RADIO    12
#define NS_FORM_INPUT_SUBMIT   13
#define NS_FORM_INPUT_TEXT     14
#define NS_FORM_LABEL          15
#define NS_FORM_OPTION         16
#define NS_FORM_OPTGROUP       17
#define NS_FORM_LEGEND         18
#define NS_FORM_SELECT         19
#define NS_FORM_TEXTAREA       20
#define NS_FORM_OBJECT         21

#define NS_IFORMCONTROL_IID   \
{ 0x119c5ce8, 0xf4b0, 0x456c, \
  {0x83, 0x4d, 0xb1, 0x90, 0x3d, 0x99, 0x9f, 0xb3} }







class nsIFormControl : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMCONTROL_IID)

  



  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm) = 0;

  











  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm,
                     PRBool aRemoveFromForm,
                     PRBool aNotify) = 0;

  



  NS_IMETHOD_(PRInt32) GetType() const = 0 ;

  



  NS_IMETHOD Reset() = 0;

  







  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement) = 0;

  




  NS_IMETHOD SaveState() = 0;

  








  virtual PRBool RestoreState(nsPresState* aState) = 0;

  virtual PRBool AllowDrop() = 0;

  




  virtual PRBool IsSubmitControl() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormControl, NS_IFORMCONTROL_IID)

#endif 
