





































 




#ifndef nsTreeWalker_h___
#define nsTreeWalker_h___

#include "nsIDOMTreeWalker.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsJSUtils.h"

class nsINode;
class nsIDOMNode;
class nsIDOMNodeFilter;

class nsTreeWalker : public nsIDOMTreeWalker
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMTREEWALKER

    nsTreeWalker(nsINode *aRoot,
                 PRUint32 aWhatToShow,
                 nsIDOMNodeFilter *aFilter,
                 PRBool aExpandEntityReferences);
    virtual ~nsTreeWalker();
    
private:
    nsCOMPtr<nsINode> mRoot;
    PRUint32 mWhatToShow;
    nsCOMPtr<nsIDOMNodeFilter> mFilter;
    PRBool mExpandEntityReferences;
    nsCOMPtr<nsINode> mCurrentNode;
    
    




    nsAutoVoidArray mPossibleIndexes;
    
    


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

    






    nsresult TestNode(nsINode* aNode, PRInt16* _filtered);
    
    









    PRInt32 IndexOf(nsINode* aParent,
                    nsINode* aChild,
                    PRInt32 aIndexPos);

    





    void SetChildIndex(PRInt32 aIndexPos, PRInt32 aChildIndex)
    {
        mPossibleIndexes.ReplaceElementAt(NS_INT32_TO_PTR(aChildIndex),
                                          aIndexPos);
    }
};


nsresult NS_NewTreeWalker(nsIDOMNode *aRoot,
                          PRUint32 aWhatToShow,
                          nsIDOMNodeFilter *aFilter,
                          PRBool aEntityReferenceExpansion,
                          nsIDOMTreeWalker **aInstancePtrResult);

#endif

