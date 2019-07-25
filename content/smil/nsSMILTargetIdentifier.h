




































#ifndef NS_SMILTARGETIDENTIFIER_H_
#define NS_SMILTARGETIDENTIFIER_H_

#include "mozilla/dom/Element.h"
#include "nsAutoPtr.h"
#include "prtypes.h"














struct nsSMILTargetIdentifier
{
  nsSMILTargetIdentifier()
    : mElement(nsnull), mAttributeName(nsnull),
      mAttributeNamespaceID(kNameSpaceID_Unknown), mIsCSS(false) {}

  inline bool Equals(const nsSMILTargetIdentifier& aOther) const
  {
    return (aOther.mElement              == mElement &&
            aOther.mAttributeName        == mAttributeName &&
            aOther.mAttributeNamespaceID == mAttributeNamespaceID &&
            aOther.mIsCSS                == mIsCSS);
  }

  nsRefPtr<mozilla::dom::Element> mElement;
  nsRefPtr<nsIAtom>    mAttributeName;
  PRInt32              mAttributeNamespaceID;
  bool                 mIsCSS;
};











class nsSMILWeakTargetIdentifier
{
public:
  
  nsSMILWeakTargetIdentifier()
    : mElement(nsnull), mAttributeName(nsnull), mIsCSS(false) {}

  
  nsSMILWeakTargetIdentifier&
    operator=(const nsSMILTargetIdentifier& aOther)
  {
    mElement = aOther.mElement;
    mAttributeName = aOther.mAttributeName;
    mIsCSS = aOther.mIsCSS;
    return *this;
  }

  
  inline bool Equals(const nsSMILTargetIdentifier& aOther) const
  {
    return (aOther.mElement       == mElement &&
            aOther.mAttributeName == mAttributeName &&
            aOther.mIsCSS         == mIsCSS);
  }

private:
  const nsIContent* mElement;
  const nsIAtom*    mAttributeName;
  bool              mIsCSS;
};

#endif 
