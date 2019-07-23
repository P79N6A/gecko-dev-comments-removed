




































#ifndef nsIRange_h___
#define nsIRange_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsIDOMRange.h"


#define NS_IRANGE_IID \
{ 0xbf5c5799, 0xe5b0, 0x49b5, \
 { 0xbd, 0x45, 0x3d, 0x9a, 0x0f, 0xb4, 0x97, 0x89 } }

class nsIRange : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRANGE_IID)

  nsIRange()
    : mRoot(nsnull),
      mStartOffset(0),
      mEndOffset(0),
      mIsPositioned(PR_FALSE),
      mIsDetached(PR_FALSE),
      mMaySpanAnonymousSubtrees(PR_FALSE)
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

  void SetMaySpanAnonymousSubtrees(PRBool aMaySpanAnonymousSubtrees)
  {
    mMaySpanAnonymousSubtrees = aMaySpanAnonymousSubtrees;
  }

  virtual nsINode* GetCommonAncestor() = 0;

  virtual void Reset() = 0;

  
  
  virtual nsresult SetStart(nsINode* aParent, PRInt32 aOffset) = 0;
  virtual nsresult SetEnd(nsINode* aParent, PRInt32 aOffset) = 0;
  virtual nsresult CloneRange(nsIRange** aNewRange) = 0;

protected:
  nsCOMPtr<nsINode> mRoot;
  nsCOMPtr<nsINode> mStartParent;
  nsCOMPtr<nsINode> mEndParent;
  PRInt32 mStartOffset;
  PRInt32 mEndOffset;

  PRPackedBool mIsPositioned;
  PRPackedBool mIsDetached;
  PRPackedBool mMaySpanAnonymousSubtrees;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRange, NS_IRANGE_IID)

#endif 
