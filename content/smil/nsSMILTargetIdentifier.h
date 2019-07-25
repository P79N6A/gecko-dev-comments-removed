




































#ifndef NS_SMILTARGETIDENTIFIER_H_
#define NS_SMILTARGETIDENTIFIER_H_

#include "mozilla/dom/Element.h"
#include "nsAutoPtr.h"
#include "prtypes.h"














struct nsSMILTargetIdentifier
{
  nsSMILTargetIdentifier()
    : mElement(nsnull), mAttributeName(nsnull),
      mAttributeNamespaceID(kNameSpaceID_Unknown), mIsCSS(PR_FALSE) {}

  inline PRBool Equals(const nsSMILTargetIdentifier& aOther) const
  {
    return (aOther.mElement              == mElement &&
            aOther.mAttributeName        == mAttributeName &&
            aOther.mAttributeNamespaceID == mAttributeNamespaceID &&
            aOther.mIsCSS                == mIsCSS);
  }

  nsRefPtr<mozilla::dom::Element> mElement;
  nsRefPtr<nsIAtom>    mAttributeName;
  PRInt32              mAttributeNamespaceID;
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
