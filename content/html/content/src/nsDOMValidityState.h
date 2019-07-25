




































#ifndef nsDOMValidityState_h__
#define nsDOMValidityState_h__

#include "nsIDOMValidityState.h"
#include "nsIConstraintValidation.h"


class nsDOMValidityState : public nsIDOMValidityState
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMVALIDITYSTATE

  friend class nsIConstraintValidation;

protected:
  nsDOMValidityState(nsIConstraintValidation* aConstraintValidation);

  




  inline void Disconnect()
  {
    mConstraintValidation = nsnull;
  }

  


  inline bool GetValidityState(nsIConstraintValidation::ValidityStateType aState) const
  {
    return mConstraintValidation &&
           mConstraintValidation->GetValidityState(aState);
  }

  
  nsIConstraintValidation*       mConstraintValidation;
};

#endif 

