




#ifndef mozilla_dom_ValidityState_h
#define mozilla_dom_ValidityState_h

#include "nsIDOMValidityState.h"
#include "nsIConstraintValidation.h"

namespace mozilla {
namespace dom {

class ValidityState MOZ_FINAL : public nsIDOMValidityState
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMVALIDITYSTATE

  friend class ::nsIConstraintValidation;

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

