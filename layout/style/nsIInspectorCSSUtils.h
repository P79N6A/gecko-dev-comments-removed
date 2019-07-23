








































#ifndef nsIInspectorCSSUtils_h___
#define nsIInspectorCSSUtils_h___

#include "nsISupports.h"
#include "nsCSSProps.h"
class nsRuleNode;
class nsIStyleRule;
class nsIFrame;
struct nsRect;
class nsIContent;
class nsIDOMElement;
class nsIArray;


#define NS_IINSPECTORCSSUTILS_IID \
  { 0xafb608b5, 0x96ac, 0x440e, \
    { 0xa2, 0x03, 0x52, 0xca, 0xc9, 0xf1, 0x88, 0xe1 } }


#define NS_INSPECTORCSSUTILS_CID \
  { 0x7ef2f07f, 0x6e34, 0x410b, \
    {0x83, 0x36, 0x88, 0xac, 0xd1, 0xcd, 0x16, 0xb7 } }

class nsIInspectorCSSUtils : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IINSPECTORCSSUTILS_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIInspectorCSSUtils, NS_IINSPECTORCSSUTILS_IID)

#endif
