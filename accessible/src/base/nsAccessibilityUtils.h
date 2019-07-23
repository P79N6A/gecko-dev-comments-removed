





































#ifndef nsAccessibilityUtils_h_
#define nsAccessibilityUtils_h_

#include "nsAccessibilityAtoms.h"
#include "nsIAccessible.h"

#include "nsIDOMNode.h"
#include "nsIPersistentProperties2.h"
#include "nsIContent.h"
#include "nsIFrame.h"

class nsAccUtils
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
                               PRInt32 *aSetSize);

  


  static PRBool HasAccGroupAttrs(nsIPersistentProperties *aAttributes);

  


  static void SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               PRInt32 aLevel,
                               PRInt32 aPosInSet,
                               PRInt32 aSetSize);

  






  static void SetAccAttrsForXULSelectControlItem(nsIDOMNode *aNode,
                                                 nsIPersistentProperties *aAttributes);

  



  static PRBool HasListener(nsIContent *aContent, const nsAString& aEventType);

  





  static PRUint32 GetAccessKeyFor(nsIContent *aContent);

  


  static nsresult FireAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                               PRBool aIsAsynch = PR_FALSE);

  






   static PRBool IsAncestorOf(nsIDOMNode *aPossibleAncestorNode,
                              nsIDOMNode *aPossibleDescendantNode);

  





   static already_AddRefed<nsIAccessible>
     GetAncestorWithRole(nsIAccessible *aDescendant, PRUint32 aRole);

  










  static nsresult ScrollSubstringTo(nsIFrame *aFrame,
                                    nsIDOMNode *aStartNode, PRInt32 aStartIndex,
                                    nsIDOMNode *aEndNode, PRInt32 aEndIndex,
                                    PRUint32 aScrollType);

  



  static void ConvertScrollTypeToPercents(PRUint32 aScrollType,
                                          PRInt16 *aVPercent,
                                          PRInt16 *aHPercent);
};

#endif

