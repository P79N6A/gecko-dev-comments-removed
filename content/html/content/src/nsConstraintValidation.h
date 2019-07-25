




































#ifndef nsConstraintValidition_h___
#define nsConstraintValidition_h___

#include "nsDOMValidityState.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsGenericHTMLFormElement;






class nsConstraintValidation
{
public:

  virtual ~nsConstraintValidation();

  nsresult GetValidity(nsIDOMValidityState** aValidity);
  nsresult GetWillValidate(PRBool* aWillValidate,
                           nsGenericHTMLFormElement* aElement);
  nsresult GetValidationMessage(nsAString & aValidationMessage,
                                nsGenericHTMLFormElement* aElement);
  nsresult CheckValidity(PRBool* aValidity,
                         nsGenericHTMLFormElement* aElement);
  nsresult SetCustomValidity(const nsAString & aError);

  virtual PRBool   IsValueMissing    () { return PR_FALSE; }
  virtual PRBool   HasTypeMismatch   () { return PR_FALSE; }
  virtual PRBool   HasPatternMismatch() { return PR_FALSE; }
  virtual PRBool   IsTooLong         () { return PR_FALSE; }
  virtual PRBool   HasRangeUnderflow () { return PR_FALSE; }
  virtual PRBool   HasRangeOverflow  () { return PR_FALSE; }
  virtual PRBool   HasStepMismatch   () { return PR_FALSE; }
          PRBool   HasCustomError    () const;
          PRBool   IsValid           ();

protected:

  enum ValidationMessageType
  {
    VALIDATION_MESSAGE_VALUE_MISSING,
    VALIDATION_MESSAGE_TYPE_MISMATCH,
    VALIDATION_MESSAGE_PATTERN_MISMATCH,
    VALIDATION_MESSAGE_TOO_LONG,
    VALIDATION_MESSAGE_RANGE_UNDERFLOW,
    VALIDATION_MESSAGE_RANGE_OVERFLOW,
    VALIDATION_MESSAGE_STEP_MISMATCH
  };

          PRBool   IsCandidateForConstraintValidation(nsGenericHTMLFormElement* aElement);
  virtual PRBool   IsBarredFromConstraintValidation()       { return PR_FALSE; }
  virtual nsresult GetValidationMessage(nsAString& aValidationMessage,
                                        ValidationMessageType aType) {
                     return NS_OK;
                   }

  nsRefPtr<nsDOMValidityState>  mValidity;
  nsString                      mCustomValidity;
};





#define NS_FORWARD_NSCONSTRAINTVALIDATION                                     \
  NS_IMETHOD GetValidity(nsIDOMValidityState** aValidity) {                   \
    return nsConstraintValidation::GetValidity(aValidity);                    \
  }                                                                           \
  NS_IMETHOD GetWillValidate(PRBool* aWillValidate) {                         \
    return nsConstraintValidation::GetWillValidate(aWillValidate, this);      \
  }                                                                           \
  NS_IMETHOD GetValidationMessage(nsAString& aValidationMessage) {            \
    return nsConstraintValidation::GetValidationMessage(aValidationMessage, this); \
  }                                                                           \
  NS_IMETHOD SetCustomValidity(const nsAString& aError) {                     \
    return nsConstraintValidation::SetCustomValidity(aError);                 \
  }                                                                           \
  NS_IMETHOD CheckValidity(PRBool* aValidity) {                               \
    return nsConstraintValidation::CheckValidity(aValidity, this);            \
  }



#define NS_IMPL_NSCONSTRAINTVALIDATION(_from)                                 \
  NS_IMETHODIMP _from::GetValidity(nsIDOMValidityState** aValidity) {         \
    return nsConstraintValidation::GetValidity(aValidity);                    \
  }                                                                           \
  NS_IMETHODIMP _from::GetWillValidate(PRBool* aWillValidate) {               \
    return nsConstraintValidation::GetWillValidate(aWillValidate, this);      \
  }                                                                           \
  NS_IMETHODIMP _from::GetValidationMessage(nsAString& aValidationMessage) {  \
    return nsConstraintValidation::GetValidationMessage(aValidationMessage, this); \
  }                                                                           \
  NS_IMETHODIMP _from::SetCustomValidity(const nsAString& aError) {           \
    return nsConstraintValidation::SetCustomValidity(aError);                 \
  }                                                                           \
  NS_IMETHODIMP _from::CheckValidity(PRBool* aValidity) {                     \
    return nsConstraintValidation::CheckValidity(aValidity, this);            \
  }


#endif 

