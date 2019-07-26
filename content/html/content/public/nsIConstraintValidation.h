




#ifndef nsIConstraintValidition_h___
#define nsIConstraintValidition_h___

#include "nsISupports.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsIDOMValidityState;

namespace mozilla {
namespace dom {
class ValidityState;
}
}

#define NS_ICONSTRAINTVALIDATION_IID \
{ 0x983829da, 0x1aaf, 0x449c, \
 { 0xa3, 0x06, 0x85, 0xd4, 0xf0, 0x31, 0x1c, 0xf6 } }








class nsIConstraintValidation : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONSTRAINTVALIDATION_IID);

  friend class mozilla::dom::ValidityState;

  static const uint16_t sContentSpecifiedMaxLengthMessage;

  virtual ~nsIConstraintValidation();

  bool IsValid() const { return mValidityBitField == 0; }

  bool IsCandidateForConstraintValidation() const {
           return !mBarredFromConstraintValidation;
         }

  NS_IMETHOD GetValidationMessage(nsAString& aValidationMessage);

  enum ValidityStateType
  {
    VALIDITY_STATE_VALUE_MISSING    = 0x1 <<  0,
    VALIDITY_STATE_TYPE_MISMATCH    = 0x1 <<  1,
    VALIDITY_STATE_PATTERN_MISMATCH = 0x1 <<  2,
    VALIDITY_STATE_TOO_LONG         = 0x1 <<  3,
  
    VALIDITY_STATE_RANGE_UNDERFLOW  = 0x1 <<  5,
    VALIDITY_STATE_RANGE_OVERFLOW   = 0x1 <<  6,
    VALIDITY_STATE_STEP_MISMATCH    = 0x1 <<  7,
    VALIDITY_STATE_BAD_INPUT        = 0x1 <<  8,
    VALIDITY_STATE_CUSTOM_ERROR     = 0x1 <<  9,
  };

  void SetValidityState(ValidityStateType mState,
                        bool mValue);

  
  bool WillValidate() const {
    return IsCandidateForConstraintValidation();
  }
  mozilla::dom::ValidityState* Validity();
  bool CheckValidity();

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

protected:
  


  nsRefPtr<mozilla::dom::ValidityState>  mValidity;

private:

  



  int16_t                       mValidityBitField;

  


  bool                          mBarredFromConstraintValidation;

  


  nsString                      mCustomValidity;
};





#define NS_FORWARD_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY           \
  NS_IMETHOD GetValidity(nsIDOMValidityState** aValidity) {                   \
    return nsIConstraintValidation::GetValidity(aValidity);                   \
  }                                                                           \
  NS_IMETHOD GetWillValidate(bool* aWillValidate) {                           \
    *aWillValidate = WillValidate();                                          \
    return NS_OK;                                                             \
  }                                                                           \
  NS_IMETHOD GetValidationMessage(nsAString& aValidationMessage) {            \
    return nsIConstraintValidation::GetValidationMessage(aValidationMessage); \
  }                                                                           \
  using nsIConstraintValidation::CheckValidity;                               \
  NS_IMETHOD CheckValidity(bool* aValidity) {                                 \
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
  NS_IMETHODIMP _from::GetWillValidate(bool* aWillValidate) {                 \
    *aWillValidate = WillValidate();                                          \
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

