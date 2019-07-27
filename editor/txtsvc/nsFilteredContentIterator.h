




#ifndef nsFilteredContentIterator_h__
#define nsFilteredContentIterator_h__

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIContentIterator.h"
#include "nsISupportsImpl.h"
#include "nscore.h"

class nsIAtom;
class nsIDOMNode;
class nsIDOMRange;
class nsINode;
class nsITextServicesFilter;

class nsFilteredContentIterator MOZ_FINAL : public nsIContentIterator
{
public:

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFilteredContentIterator)

  explicit nsFilteredContentIterator(nsITextServicesFilter* aFilter);

  
  virtual nsresult Init(nsINode* aRoot);
  virtual nsresult Init(nsIDOMRange* aRange);
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

  virtual ~nsFilteredContentIterator();

  
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
  nsCOMPtr<nsIDOMRange>           mRange;
  bool                            mDidSkip;
  bool                            mIsOutOfRange;
  eDirectionType                  mDirection;
};

#endif
