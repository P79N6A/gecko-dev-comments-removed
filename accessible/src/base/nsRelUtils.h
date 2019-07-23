





































#ifndef _nsRelUtils_H_
#define _nsRelUtils_H_

#include "nsAccessibleRelationWrap.h"

#include "nsIAtom.h"
#include "nsIContent.h"


#define NS_OK_NO_RELATION_TARGET \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x24)




class nsRelUtils
{
public:
  







  static already_AddRefed<nsIAccessible>
    GetRelatedAccessible(nsIAccessible *aAccessible, PRUint32 aRelationType);

  







  static nsresult AddTarget(PRUint32 aRelationType,
                            nsIAccessibleRelation **aRelation,
                            nsIAccessible *aTarget);

  







  static nsresult AddTargetFromContent(PRUint32 aRelationType,
                                       nsIAccessibleRelation **aRelation,
                                       nsIContent *aContent);

  











  static nsresult AddTargetFromIDRefAttr(PRUint32 aRelationType,
                                         nsIAccessibleRelation **aRelation,
                                         nsIContent *aContent, nsIAtom *aAttr,
                                         PRBool aMayBeAnon = PR_FALSE);

  








  static nsresult AddTargetFromIDRefsAttr(PRUint32 aRelationType,
                                          nsIAccessibleRelation **aRelation,
                                          nsIContent *aContent, nsIAtom *aAttr);

  












  static nsresult AddTargetFromNeighbour(PRUint32 aRelationType,
                                         nsIAccessibleRelation **aRelation,
                                         nsIContent *aContent,
                                         nsIAtom *aNeighboutAttr,
                                         nsIAtom *aNeighboutTagName = nsnull);

  









  static nsresult
    AddTargetFromChildrenHavingIDRefsAttr(PRUint32 aRelationType,
                                          nsIAccessibleRelation **aRelation,
                                          nsIContent *aRootContent,
                                          nsIContent *aContent,
                                          nsIAtom *aIDRefsAttr);

  


  static already_AddRefed<nsAccessibleRelation>
  QueryAccRelation(nsIAccessibleRelation *aRelation)
  {
    nsAccessibleRelation* relation = nsnull;
    if (aRelation)
      CallQueryInterface(aRelation, &relation);

    return relation;
  }
};

#endif
