




































#ifndef nsFilteredContentIterator_h__
#define nsFilteredContentIterator_h__

#include "nsIContentIterator.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsITextServicesFilter.h"
#include "nsIDOMNSRange.h"
#include "nsIRangeUtils.h"
#include "nsCycleCollectionParticipant.h"




class nsFilteredContentIterator : public nsIContentIterator
{
public:

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFilteredContentIterator)

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
  virtual bool IsDone();
  virtual nsresult PositionAt(nsINode* aCurNode);

  
  bool DidSkip()      { return mDidSkip; }
  void         ClearDidSkip() {  mDidSkip = false; }

protected:
  nsFilteredContentIterator() { }

  
  typedef enum {eDirNotSet, eForward, eBackward} eDirectionType;
  nsresult AdvanceNode(nsIDOMNode* aNode, nsIDOMNode*& aNewNode, eDirectionType aDir);
  void CheckAdvNode(nsIDOMNode* aNode, bool& aDidSkip, eDirectionType aDir);
  nsresult SwitchDirections(bool aChangeToForward);

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
  bool                            mDidSkip;
  bool                            mIsOutOfRange;
  eDirectionType                  mDirection;
};

#endif
