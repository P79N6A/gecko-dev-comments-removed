





































#ifndef nsFind_h__
#define nsFind_h__

#include "nsIFind.h"

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIContentIterator.h"
#include "nsIParserService.h"
#include "nsIWordBreaker.h"

class nsIPresShell;
class nsIAtom;

#define NS_FIND_CONTRACTID "@mozilla.org/embedcomp/rangefind;1"

#define NS_FIND_CID \
 {0x471f4944, 0x1dd2, 0x11b2, {0x87, 0xac, 0x90, 0xbe, 0x0a, 0x51, 0xd6, 0x09}}

class nsFind : public nsIFind
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFIND

  nsFind();
  virtual ~nsFind();

protected:
  static PRInt32 sInstanceCount;

  
  static nsIAtom* sImgAtom;
  static nsIAtom* sHRAtom;
  
  static nsIAtom* sScriptAtom;
  static nsIAtom* sNoframesAtom;
  static nsIAtom* sSelectAtom;
  static nsIAtom* sTextareaAtom;
  static nsIAtom* sThAtom;
  static nsIAtom* sTdAtom;

  
  
  PRPackedBool mFindBackward;
  PRPackedBool mCaseSensitive;

  nsCOMPtr<nsIWordBreaker> mWordBreaker;
  nsCOMPtr<nsIParserService> mParserService;

  PRInt32 mIterOffset;
  nsCOMPtr<nsIDOMNode> mIterNode;

  
  nsCOMPtr<nsIDOMNode> mLastBlockParent;
  nsresult GetBlockParent(nsIDOMNode* aNode, nsIDOMNode** aParent);

  
  PRBool IsTextNode(nsIDOMNode* aNode);
  PRBool IsBlockNode(nsIContent* aNode);
  PRBool SkipNode(nsIContent* aNode);
  PRBool IsVisibleNode(nsIDOMNode *aNode);

  
  nsresult NextNode(nsIDOMRange* aSearchRange,
                    nsIDOMRange* aStartPoint, nsIDOMRange* aEndPoint,
                    PRBool aContinueOk);

  
  void ResetAll();

  
  nsresult InitIterator(nsIDOMRange* aSearchRange);
  nsCOMPtr<nsIContentIterator> mIterator;
};

#endif 
