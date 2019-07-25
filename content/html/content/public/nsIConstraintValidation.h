




































#ifndef nsIConstraintValidition_h___
#define nsIConstraintValidition_h___

#include "nsISupports.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsDOMValidityState;
class nsIDOMValidityState;

#define NS_ICONSTRAINTVALIDATION_IID \
{ 0xca3824dc, 0x4f5c, 0x4878, \
 { 0xa6, 0x8a, 0x95, 0x54, 0x5f, 0xfa, 0x4b, 0xf9 } }








class nsIConstraintValidation : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONSTRAINTVALIDATION_IID);

  friend class nsDOMValidityState;

  static const PRUint16 sContentSpecifiedMaxLengthMessage;

  virtual ~nsIConstraintValidation();

  bool IsValid() const { return mValidityBitField == 0; }

  bool IsCandidateForConstraintValidation() const {
           return !mBarredFromConstraintValidation;
         }

  NS_IMETHOD GetValidationMessage(nsAString& aValidationMessage);

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

  void SetValidityState(ValidityStateType mState,
                        bool mValue);

protected:

  
  nsIConstraintValidation();

  nsresult GetValidity(nsIDOMValidityState** aValidity);
  nsresult CheckValidity(bool* aValidity);
  void     SetCustomValidity(const nsAString& aError);

  bool GetValidityState(ValidityStateType mState) const {
         return mValidityBitField & mState;
       }

  void SetBarredFromConstraintValidation(bool aBarred);

  virtual nsresult GetValidationMessage(nsAString& aValidationMessage,
                                        ValidityStateType aType) {
                     return NS_OK;
                   }

private:

  



  PRInt8                        mValidityBitField;

  


  nsRefPtr<nsDOMValidityState>  mValidity;

  


  bool                          mBarredFromConstraintValidation;

  


  nsString                      mCustomValidity;
};





#define NS_FORWARD_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY           \
  NS_IMETHOD GetValidity(nsIDOMValidityState** aValidity) {                   \
    return nsIConstraintValidation::GetValidity(aValidity);                   \
  }                                                                           \
  NS_IMETHOD GetWillValidate(bool* aWillValidate) {                         \
    *aWillValidate = IsCandidateForConstraintValidation();                    \
    return NS_OK;                                                             \
  }                                                                           \
  NS_IMETHOD GetValidationMessage(nsAString& aValidationMessage) {            \
    return nsIConstraintValidation::GetValidationMessage(aValidationMessage); \
  }                                                                           \
  NS_IMETHOD CheckValidity(bool* aValidity) {                               \
    return nsIConstraintValidation::CheckValidity(aValidity);                 \
  }

#define NS_FORWARD_NSICONSTRAINTVALIDATION                                    \
  NS_FORWARD_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY                 \
  NS_IMETHOD SetCustomValidity(const nsAString& aError) {                     \
    nsIConstraintValidation::SetCustomValidity(aError);                       \
    return NS_OK;                                                             \
  }



#define NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(_from)       \
  NS_IMETHODIMP _from::GetValidity(nsIDOMValidityState** aValidity) {         \
    return nsIConstraintValidation::GetValidity(aValidity);                   \
  }                                                                           \
  NS_IMETHODIMP _from::GetWillValidate(bool* aWillValidate) {               \
    *aWillValidate = IsCandidateForConstraintValidation();                    \
    return NS_OK;                                                             \
  }                                                                           \
  NS_IMETHODIMP _from::GetValidationMessage(nsAString& aValidationMessage) {  \
    return nsIConstraintValidation::GetValidationMessage(aValidationMessage); \
  }                                                                           \
  NS_IMETHODIMP _from::CheckValidity(bool* aValidity) {                     \
    return nsIConstraintValidation::CheckValidity(aValidity);                 \
  }

#define NS_IMPL_NSICONSTRAINTVALIDATION(_from)                                \
  NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(_from)             \
  NS_IMETHODIMP _from::SetCustomValidity(const nsAString& aError) {           \
    nsIConstraintValidation::SetCustomValidity(aError);                       \
    return NS_OK;                                                             \
  }

NS_DEFINE_STATIC_IID_ACCESSOR(nsIConstraintValidation,
                              NS_ICONSTRAINTVALIDATION_IID)

#endif 

