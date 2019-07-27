




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
class nsRange;

class nsFilteredContentIterator MOZ_FINAL : public nsIContentIterator
{
public:

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFilteredContentIterator)

  explicit nsFilteredContentIterator(nsITextServicesFilter* aFilter);

  
  virtual nsresult Init(nsINode* aRoot) MOZ_OVERRIDE;
  virtual nsresult Init(nsIDOMRange* aRange) MOZ_OVERRIDE;
  virtual void First() MOZ_OVERRIDE;
  virtual void Last() MOZ_OVERRIDE;
  virtual void Next() MOZ_OVERRIDE;
  virtual void Prev() MOZ_OVERRIDE;
  virtual nsINode *GetCurrentNode() MOZ_OVERRIDE;
  virtual bool IsDone() MOZ_OVERRIDE;
  virtual nsresult PositionAt(nsINode* aCurNode) MOZ_OVERRIDE;

  
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
  nsRefPtr<nsRange>               mRange;
  bool                            mDidSkip;
  bool                            mIsOutOfRange;
  eDirectionType                  mDirection;
};

#endif
