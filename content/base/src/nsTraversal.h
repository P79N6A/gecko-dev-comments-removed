




 




#ifndef nsTraversal_h___
#define nsTraversal_h___

#include "nsCOMPtr.h"

class nsINode;
class nsIDOMNodeFilter;

class nsTraversal
{
public:
    nsTraversal(nsINode *aRoot,
                uint32_t aWhatToShow,
                nsIDOMNodeFilter *aFilter);
    virtual ~nsTraversal();

protected:
    nsCOMPtr<nsINode> mRoot;
    uint32_t mWhatToShow;
    nsCOMPtr<nsIDOMNodeFilter> mFilter;
    bool mInAcceptNode;

    






    nsresult TestNode(nsINode* aNode, int16_t* _filtered);
};

#endif

