





































 




#ifndef nsTraversal_h___
#define nsTraversal_h___

#include "nsCOMPtr.h"

class nsINode;
class nsIDOMNodeFilter;

class nsTraversal
{
public:
    nsTraversal(nsINode *aRoot,
                PRUint32 aWhatToShow,
                nsIDOMNodeFilter *aFilter);
    virtual ~nsTraversal();

protected:
    nsCOMPtr<nsINode> mRoot;
    PRUint32 mWhatToShow;
    nsCOMPtr<nsIDOMNodeFilter> mFilter;
    bool mInAcceptNode;

    






    nsresult TestNode(nsINode* aNode, PRInt16* _filtered);
};

#endif

