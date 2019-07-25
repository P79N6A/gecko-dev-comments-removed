




































#ifndef nsDOMValidityState_h__
#define nsDOMValidityState_h__

#include "nsIDOMValidityState.h"

class nsConstraintValidation;

class nsDOMValidityState : public nsIDOMValidityState
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMVALIDITYSTATE

  friend class nsConstraintValidation;

protected:
  
  
  void Disconnect()
  {
    mConstraintValidation = nsnull;
  }

  nsDOMValidityState(nsConstraintValidation* aConstraintValidation);

  nsConstraintValidation*       mConstraintValidation;
};

#endif 

