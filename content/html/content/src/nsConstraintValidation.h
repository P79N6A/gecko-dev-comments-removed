




































#ifndef nsConstraintValidition_h___
#define nsConstraintValidition_h___

#include "nsAutoPtr.h"
#include "nsString.h"

class nsDOMValidityState;
class nsIDOMValidityState;
class nsGenericHTMLFormElement;








class nsConstraintValidation
{
public:

  friend class nsDOMValidityState;

  virtual ~nsConstraintValidation();

protected:

  enum ValidityStateType
  {
    VALIDITY_STATE_VALUE_MISSING    = 0x01, 
    VALIDITY_STATE_TYPE_MISMATCH    = 0x02, 
    VALIDITY_STATE_PATTERN_MISMATCH = 0x04, 
    VALIDITY_STATE_TOO_LONG         = 0x08, 
    VALIDITY_STATE_RANGE_UNDERFLOW  = 0x10, 
    VALIDITY_STATE_RANGE_OVERFLOW   = 0x20, 
    VALIDITY_STATE_STEP_MISMATCH    = 0x40, 
    VALIDITY_STATE_CUSTOM_ERROR     = 0x80  
  };

  
  nsConstraintValidation();

  nsresult GetValidity(nsIDOMValidityState** aValidity);
  nsresult GetWillValidate(PRBool* aWillValidate,
                           nsGenericHTMLFormElement* aElement);
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                nsGenericHTMLFormElement* aElement);
  nsresult CheckValidity(PRBool* aValidity,
                         nsGenericHTMLFormElement* aElement);
  nsresult SetCustomValidity(const nsAString& aError);

  PRBool IsValid() const { return mValidityBitField == 0; }

  bool GetValidityState(ValidityStateType mState) const {
         return mValidityBitField & mState;
       }

  void   SetValidityState(ValidityStateType mState, PRBool mValue) {
           if (mValue) {
             mValidityBitField |= mState;
           } else {
             mValidityBitField &= ~mState;
           }
         }

  PRBool IsCandidateForConstraintValidation(const nsGenericHTMLFormElement* const aElement) const;

  virtual PRBool   IsBarredFromConstraintValidation() const { return PR_FALSE; }

  virtual nsresult GetValidationMessage(nsAString& aValidationMessage,
                                        ValidityStateType aType) {
                     return NS_OK;
                   }

private:

  



  PRInt8                        mValidityBitField;

  


  nsRefPtr<nsDOMValidityState>  mValidity;

  


  nsString                      mCustomValidity;
};





#define NS_FORWARD_NSCONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY            \
  NS_IMETHOD GetValidity(nsIDOMValidityState** aValidity) {                   \
    return nsConstraintValidation::GetValidity(aValidity);                    \
  }                                                                           \
  NS_IMETHOD GetWillValidate(PRBool* aWillValidate) {                         \
    return nsConstraintValidation::GetWillValidate(aWillValidate, this);      \
  }                                                                           \
  NS_IMETHOD GetValidationMessage(nsAString& aValidationMessage) {            \
    return nsConstraintValidation::GetValidationMessage(aValidationMessage, this); \
  }                                                                           \
  NS_IMETHOD CheckValidity(PRBool* aValidity) {                               \
    return nsConstraintValidation::CheckValidity(aValidity, this);            \
  }

#define NS_FORWARD_NSCONSTRAINTVALIDATION                                     \
  NS_FORWARD_NSCONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY                  \
  NS_IMETHOD SetCustomValidity(const nsAString& aError) {                     \
    return nsConstraintValidation::SetCustomValidity(aError);                 \
  }



#define NS_IMPL_NSCONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(_from)        \
  NS_IMETHODIMP _from::GetValidity(nsIDOMValidityState** aValidity) {         \
    return nsConstraintValidation::GetValidity(aValidity);                    \
  }                                                                           \
  NS_IMETHODIMP _from::GetWillValidate(PRBool* aWillValidate) {               \
    return nsConstraintValidation::GetWillValidate(aWillValidate, this);      \
  }                                                                           \
  NS_IMETHODIMP _from::GetValidationMessage(nsAString& aValidationMessage) {  \
    return nsConstraintValidation::GetValidationMessage(aValidationMessage, this); \
  }                                                                           \
  NS_IMETHODIMP _from::CheckValidity(PRBool* aValidity) {                     \
    return nsConstraintValidation::CheckValidity(aValidity, this);            \
  }

#define NS_IMPL_NSCONSTRAINTVALIDATION(_from)                                 \
  NS_IMPL_NSCONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(_from)              \
  NS_IMETHODIMP _from::SetCustomValidity(const nsAString& aError) {           \
    return nsConstraintValidation::SetCustomValidity(aError);                 \
  }


#endif 

