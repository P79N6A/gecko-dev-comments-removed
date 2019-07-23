




































#ifndef nsFilteredContentIterator_h__
#define nsFilteredContentIterator_h__

#include "nsIContentIterator.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsITextServicesFilter.h"
#include "nsIDOMNSRange.h"
#include "nsIRangeUtils.h"




class nsFilteredContentIterator : public nsIContentIterator
{
public:

  
  NS_DECL_ISUPPORTS

  nsFilteredContentIterator(nsITextServicesFilter* aFilter);

  virtual ~nsFilteredContentIterator();

  
  virtual nsresult Init(nsINode* aRoot);
  virtual nsresult Init(nsIDOMRange* aRange);
  virtual nsresult Init(nsIRange* aRange);
  virtual void First();
  virtual void Last();
  virtual void Next();
  virtual void Prev();
  virtual nsINode *GetCurrentNode();
  virtual PRBool IsDone();
  virtual nsresult PositionAt(nsINode* aCurNode);

  
  PRPackedBool DidSkip()      { return mDidSkip; }
  void         ClearDidSkip() {  mDidSkip = PR_FALSE; }

protected:
  nsFilteredContentIterator() { }

  
  typedef enum {eDirNotSet, eForward, eBackward} eDirectionType;
  nsresult AdvanceNode(nsIDOMNode* aNode, nsIDOMNode*& aNewNode, eDirectionType aDir);
  void CheckAdvNode(nsIDOMNode* aNode, PRPackedBool& aDidSkip, eDirectionType aDir);
  nsresult SwitchDirections(PRPackedBool aChangeToForward);

  nsCOMPtr<nsIContentIterator> mCurrentIterator;
  nsCOMPtr<nsIContentIterator> mIterator;
  nsCOMPtr<nsIContentIterator> mPreIterator;

  nsCOMPtr<nsIAtom> mBlockQuoteAtom;
  nsCOMPtr<nsIAtom> mScriptAtom;
  nsCOMPtr<nsIAtom> mTextAreaAtom;
  nsCOMPtr<nsIAtom> mSelectAreaAtom;
  nsCOMPtr<nsIAtom> mMapAtom;

  nsCOMPtr<nsITextServicesFilter> mFilter;
  nsCOMPtr<nsIDOMNSRange>         mRange;
  PRPackedBool                    mDidSkip;
  PRPackedBool                    mIsOutOfRange;
  eDirectionType                  mDirection;
};

#endif
