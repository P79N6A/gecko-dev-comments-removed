




































#ifndef nsIRange_h___
#define nsIRange_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsINode.h"


#define NS_IRANGE_IID \
{ 0x267c8c4e, 0x7c97, 0x4a35, \
  { 0xaa, 0x08, 0x55, 0xa5, 0xbe, 0x3a, 0xc5, 0x74 } }

class nsIRange : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRANGE_IID)

  nsIRange()
    : mRoot(nsnull),
      mStartOffset(0),
      mEndOffset(0),
      mIsPositioned(PR_FALSE),
      mIsDetached(PR_FALSE)
  {
  }

  nsINode* GetRoot()
  {
    return mRoot;
  }

  nsINode* GetStartParent()
  {
    return mStartParent;
  }

  nsINode* GetEndParent()
  {
    return mEndParent;
  }

  PRInt32 StartOffset()
  {
    return mStartOffset;
  }

  PRInt32 EndOffset()
  {
    return mEndOffset;
  }
  
  PRBool IsPositioned()
  {
    return mIsPositioned;
  }

  PRBool IsDetached()
  {
    return mIsDetached;
  }
  
  PRBool Collapsed()
  {
    return mIsPositioned && mStartParent == mEndParent &&
           mStartOffset == mEndOffset;
  }

  virtual nsINode* GetCommonAncestor() = 0;

  virtual void Reset() = 0;

protected:
  nsINode* mRoot;
  nsCOMPtr<nsINode> mStartParent;
  nsCOMPtr<nsINode> mEndParent;
  PRInt32 mStartOffset;
  PRInt32 mEndOffset;

  PRPackedBool mIsPositioned;
  PRPackedBool mIsDetached;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRange, NS_IRANGE_IID)

#endif 
