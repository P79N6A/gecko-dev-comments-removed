




#ifndef mozilla_dom_ValidityState_h
#define mozilla_dom_ValidityState_h

#include "nsIDOMValidityState.h"
#include "nsIConstraintValidation.h"
#include "nsWrapperCache.h"

class JSObject;
struct JSContext;

namespace mozilla {
namespace dom {

class ValidityState MOZ_FINAL : public nsIDOMValidityState,
                                public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ValidityState)
  NS_DECL_NSIDOMVALIDITYSTATE

  friend class ::nsIConstraintValidation;

  nsIConstraintValidation* GetParentObject() const {
    return mConstraintValidation;
  }

  virtual JSObject* WrapObject(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  
  bool ValueMissing() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_VALUE_MISSING);
  }
  bool TypeMismatch() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_TYPE_MISMATCH);
  }
  bool PatternMismatch() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_PATTERN_MISMATCH);
  }
  bool TooLong() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_TOO_LONG);
  }
  bool RangeUnderflow() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_RANGE_UNDERFLOW);
  }
  bool RangeOverflow() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_RANGE_OVERFLOW);
  }
  bool StepMismatch() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_STEP_MISMATCH);
  }
  bool CustomError() const
  {
    return GetValidityState(nsIConstraintValidation::VALIDITY_STATE_CUSTOM_ERROR);
  }
  bool Valid() const
  {
    return !mConstraintValidation || mConstraintValidation->IsValid();
  }

protected:
  ValidityState(nsIConstraintValidation* aConstraintValidation);

  




  inline void Disconnect()
  {
    mConstraintValidation = nullptr;
  }

  


  inline bool GetValidityState(nsIConstraintValidation::ValidityStateType aState) const
  {
    return mConstraintValidation &&
           mConstraintValidation->GetValidityState(aState);
  }

  
  nsIConstraintValidation*       mConstraintValidation;
};

} 
} 

#endif 

