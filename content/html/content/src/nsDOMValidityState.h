




































#ifndef nsDOMValidityState_h__
#define nsDOMValidityState_h__

#include "nsIDOMValidityState.h"
#include "nsConstraintValidation.h"


class nsDOMValidityState : public nsIDOMValidityState
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMVALIDITYSTATE

  friend class nsConstraintValidation;

protected:
  nsDOMValidityState(nsConstraintValidation* aConstraintValidation);

  




  inline void Disconnect()
  {
    mConstraintValidation = nsnull;
  }

  


  inline PRBool GetValidityState(nsConstraintValidation::ValidityStateType aState) const
  {
    return mConstraintValidation &&
           mConstraintValidation->GetValidityState(aState);
  }

  
  nsConstraintValidation*       mConstraintValidation;
};

#endif 

