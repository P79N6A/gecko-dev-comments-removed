




#ifndef nsFind_h__
#define nsFind_h__

#include "nsIFind.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIContentIterator.h"
#include "nsIWordBreaker.h"

class nsIContent;

#define NS_FIND_CONTRACTID "@mozilla.org/embedcomp/rangefind;1"

#define NS_FIND_CID \
 {0x471f4944, 0x1dd2, 0x11b2, {0x87, 0xac, 0x90, 0xbe, 0x0a, 0x51, 0xd6, 0x09}}

class nsFindContentIterator;

class nsFind : public nsIFind
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIFIND
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFind)

  nsFind();

  static already_AddRefed<nsIDOMRange> CreateRange(nsINode* aNode);

protected:
  virtual ~nsFind();

  
  
  bool mFindBackward;
  bool mCaseSensitive;

  nsCOMPtr<nsIWordBreaker> mWordBreaker;

  int32_t mIterOffset;
  nsCOMPtr<nsIDOMNode> mIterNode;

  
  nsCOMPtr<nsIDOMNode> mLastBlockParent;
  nsresult GetBlockParent(nsIDOMNode* aNode, nsIDOMNode** aParent);

  
  bool IsTextNode(nsIDOMNode* aNode);
  bool IsBlockNode(nsIContent* aNode);
  bool SkipNode(nsIContent* aNode);
  bool IsVisibleNode(nsIDOMNode *aNode);

  
  nsresult NextNode(nsIDOMRange* aSearchRange,
                    nsIDOMRange* aStartPoint, nsIDOMRange* aEndPoint,
                    bool aContinueOk);

  
  void ResetAll();

  
  nsresult InitIterator(nsIDOMNode* aStartNode, int32_t aStartOffset,
                        nsIDOMNode* aEndNode, int32_t aEndOffset);
  nsRefPtr<nsFindContentIterator> mIterator;
};

#endif 
