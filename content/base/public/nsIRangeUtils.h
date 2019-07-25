








































#ifndef nsIRangeUtils_h___
#define nsIRangeUtils_h___

#include "nsISupports.h"


class nsIDOMRange;
class nsIDOMNode;
class nsIContent;


#define NS_IRANGEUTILS_IID       \
{ 0xa6cf9127, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

class nsIRangeUtils : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRANGEUTILS_IID)

  NS_IMETHOD_(PRInt32) ComparePoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                                     nsIDOMNode* aParent2, PRInt32 aOffset2) = 0;
                               
  NS_IMETHOD CompareNodeToRange(nsIContent* aNode, 
                                nsIDOMRange* aRange,
                                PRBool *outNodeBefore,
                                PRBool *outNodeAfter) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRangeUtils, NS_IRANGEUTILS_IID)

#endif 

