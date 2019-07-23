





































#ifndef nsAccessibilityUtils_h_
#define nsAccessibilityUtils_h_

#include "nsAccessibilityAtoms.h"

#include "nsIDOMNode.h"
#include "nsIPersistentProperties2.h"

class nsAccessibilityUtils
{
public:
  






  static void GetAccAttr(nsIPersistentProperties *aAttributes,
                         nsIAtom *aAttrName,
                         nsAString& aAttrValue);

  






  static void SetAccAttr(nsIPersistentProperties *aAttributes,
                         nsIAtom *aAttrName,
                         const nsAString& aAttrValue);

  


  static void GetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               PRInt32 *aLevel,
                               PRInt32 *aPosInSet,
                               PRInt32 *aSizeSet);

  


  static void SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               PRInt32 aLevel,
                               PRInt32 aPosInSet,
                               PRInt32 aSizeSet);

  






  static void SetAccAttrsForXULSelectControlItem(nsIDOMNode *aNode,
                                                 nsIPersistentProperties *aAttributes);
};

#endif