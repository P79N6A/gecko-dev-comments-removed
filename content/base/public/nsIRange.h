




































#ifndef nsIRange_h___
#define nsIRange_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsIDOMRange.h"


#define NS_IRANGE_IID \
{ 0xee12afa2, 0x4e8e, 0x4e3f, \
 { 0xad, 0x3b, 0x53, 0x50, 0x97, 0x09, 0xc6, 0x8a } }

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

  nsINode* GetRoot() const
  {
    return mRoot;
  }

  nsINode* GetStartParent() const
  {
    return mStartParent;
  }

  nsINode* GetEndParent() const
  {
    return mEndParent;
  }

  PRInt32 StartOffset() const
  {
    return mStartOffset;
  }

  PRInt32 EndOffset() const
  {
    return mEndOffset;
  }
  
  PRBool IsPositioned() const
  {
    return mIsPositioned;
  }

  PRBool IsDetached() const
  {
    return mIsDetached;
  }
  
  PRBool Collapsed() const
  {
    return mIsPositioned && mStartParent == mEndParent &&
           mStartOffset == mEndOffset;
  }

  void SetMaySpanAnonymousSubtrees(PRBool aMaySpanAnonymousSubtrees)
  {
    mMaySpanAnonymousSubtrees = aMaySpanAnonymousSubtrees;
  }

  virtual nsINode* GetCommonAncestor() const = 0;

  virtual void Reset() = 0;

  
  
  virtual nsresult SetStart(nsINode* aParent, PRInt32 aOffset) = 0;
  virtual nsresult SetEnd(nsINode* aParent, PRInt32 aOffset) = 0;
  virtual nsresult CloneRange(nsIRange** aNewRange) const = 0;

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
