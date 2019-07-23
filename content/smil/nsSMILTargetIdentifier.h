




































#ifndef NS_SMILTARGETIDENTIFIER_H_
#define NS_SMILTARGETIDENTIFIER_H_

#include "nsIContent.h"
#include "nsAutoPtr.h"
#include "prtypes.h"














struct nsSMILTargetIdentifier
{
  nsSMILTargetIdentifier()
    : mElement(nsnull), mAttributeName(nsnull), mIsCSS(PR_FALSE) {}

  inline PRBool Equals(const nsSMILTargetIdentifier& aOther) const
  {
    return (aOther.mElement       == mElement &&
            aOther.mAttributeName == mAttributeName &&
            aOther.mIsCSS         == mIsCSS);
  }

  nsRefPtr<nsIContent> mElement;
  nsRefPtr<nsIAtom>    mAttributeName; 
  PRPackedBool         mIsCSS;
};











class nsSMILWeakTargetIdentifier
{
public:
  
  nsSMILWeakTargetIdentifier()
    : mElement(nsnull), mAttributeName(nsnull), mIsCSS(PR_FALSE) {}

  
  nsSMILWeakTargetIdentifier&
    operator=(const nsSMILTargetIdentifier& aOther)
  {
    mElement = aOther.mElement;
    mAttributeName = aOther.mAttributeName;
    mIsCSS = aOther.mIsCSS;
    return *this;
  }

  
  inline PRBool Equals(const nsSMILTargetIdentifier& aOther) const
  {
    return (aOther.mElement       == mElement &&
            aOther.mAttributeName == mAttributeName &&
            aOther.mIsCSS         == mIsCSS);
  }

private:
  const nsIContent* mElement;
  const nsIAtom*    mAttributeName;
  PRPackedBool      mIsCSS;
};

#endif 
