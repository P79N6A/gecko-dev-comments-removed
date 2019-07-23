





































 




#ifndef nsTreeWalker_h___
#define nsTreeWalker_h___

#include "nsIDOMTreeWalker.h"
#include "nsTraversal.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

class nsINode;
class nsIDOMNode;
class nsIDOMNodeFilter;

class nsTreeWalker : public nsIDOMTreeWalker, public nsTraversal
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMTREEWALKER

    nsTreeWalker(nsINode *aRoot,
                 PRUint32 aWhatToShow,
                 nsIDOMNodeFilter *aFilter,
                 PRBool aExpandEntityReferences);
    virtual ~nsTreeWalker();

    NS_DECL_CYCLE_COLLECTION_CLASS(nsTreeWalker)

private:
    nsCOMPtr<nsINode> mCurrentNode;
    
    



    nsAutoTArray<PRInt32, 8> mPossibleIndexes;
    
    


    PRInt32 mPossibleIndexesPos;
    
    









    nsresult FirstChildOf(nsINode* aNode,
                          PRBool aReversed,
                          PRInt32 aIndexPos,
                          nsINode** _retval);

    









    nsresult NextSiblingOf(nsINode* aNode,
                           PRBool aReversed,
                           PRInt32 aIndexPos,
                           nsINode** _retval);
                           
    









    nsresult NextInDocumentOrderOf(nsINode* aNode,
                                   PRBool aReversed,
                                   PRInt32 aIndexPos,
                                   nsINode** _retval);

    











    nsresult ChildOf(nsINode* aNode,
                     PRInt32 childNum,
                     PRBool aReversed,
                     PRInt32 aIndexPos,
                     nsINode** _retval);

    









    PRInt32 IndexOf(nsINode* aParent,
                    nsINode* aChild,
                    PRInt32 aIndexPos);

    





    void SetChildIndex(PRInt32 aIndexPos, PRInt32 aChildIndex)
    {
        if (aIndexPos >= 0 &&
            mPossibleIndexes.EnsureLengthAtLeast(aIndexPos+1)) {
            mPossibleIndexes.ElementAt(aIndexPos) = aChildIndex;
        }
    }
};

#endif

