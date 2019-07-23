 





































#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsAccessNodeWrap.h"
#include "nsAccessibilityUtils.h"

#include "nsIAccessible.h"
#include "nsPIAccessible.h"
#include "nsIAccessibleHyperLink.h"
#include "nsIAccessibleSelectable.h"
#include "nsIAccessibleValue.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"
#include "nsAccessibleRelationWrap.h"
#include "nsIAccessibleEvent.h"

#include "nsIDOMNodeList.h"
#include "nsINameSpaceManager.h"
#include "nsWeakReference.h"
#include "nsString.h"
#include "nsIDOMDOMStringList.h"
#include "nsARIAMap.h"

struct nsRect;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsIDOMNode;
class nsIAtom;
class nsIView;


#define DEAD_END_ACCESSIBLE NS_STATIC_CAST(nsIAccessible*, (void*)1)



enum { eChildCountUninitialized = -1 };

class nsAccessibleDOMStringList : public nsIDOMDOMStringList
{
public:
  nsAccessibleDOMStringList();
  virtual ~nsAccessibleDOMStringList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  PRBool Add(const nsAString& aName) {
    return mNames.AppendString(aName);
  }

private:
  nsStringArray mNames;
};


class nsAccessible : public nsAccessNodeWrap, 
                     public nsIAccessible, 
                     public nsPIAccessible,
                     public nsIAccessibleHyperLink,
                     public nsIAccessibleSelectable,
                     public nsIAccessibleValue
{
public:
  nsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  virtual ~nsAccessible();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLE
  NS_DECL_NSPIACCESSIBLE
  NS_DECL_NSIACCESSIBLEHYPERLINK
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLEVALUE

  
  NS_IMETHOD Init();
  NS_IMETHOD Shutdown();

  





  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  



  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  




  nsresult GetARIAState(PRUint32 *aState);

#ifdef MOZ_ACCESSIBILITY_ATK
  static PRBool FindTextFrame(PRInt32 &index, nsPresContext *aPresContext, nsIFrame *aCurFrame, 
                                   nsIFrame **aFirstTextFrame, const nsIFrame *aTextFrame);
#endif

#ifdef DEBUG_A11Y
  static PRBool IsTextInterfaceSupportCorrect(nsIAccessible *aAccessible);
#endif

  static PRBool IsCorrectFrameType(nsIFrame* aFrame, nsIAtom* aAtom);
  static PRUint32 State(nsIAccessible *aAcc) { PRUint32 state; aAcc->GetFinalState(&state, nsnull); return state; }
  static PRUint32 Role(nsIAccessible *aAcc) { PRUint32 role; aAcc->GetFinalRole(&role); return role; }
  static PRBool IsText(nsIAccessible *aAcc) { PRUint32 role = Role(aAcc); return role == nsIAccessibleRole::ROLE_TEXT_LEAF || role == nsIAccessibleRole::ROLE_STATICTEXT; }
  static PRBool IsEmbeddedObject(nsIAccessible *aAcc) { PRUint32 role = Role(aAcc); return role != nsIAccessibleRole::ROLE_TEXT_LEAF && role != nsIAccessibleRole::ROLE_WHITESPACE && role != nsIAccessibleRole::ROLE_STATICTEXT; }
  static PRInt32 TextLength(nsIAccessible *aAccessible);
  static PRBool IsLeaf(nsIAccessible *aAcc) { PRInt32 numChildren; aAcc->GetChildCount(&numChildren); return numChildren > 0; }
  static PRBool IsNodeRelevant(nsIDOMNode *aNode); 
  
  already_AddRefed<nsIAccessible> GetParent() {
    nsIAccessible *parent = nsnull;
    GetParent(&parent);
    return parent;
  }

protected:
  PRBool MappedAttrState(nsIContent *aContent, PRUint32 *aStateInOut, nsStateMapEntry *aStateMapEntry);
  virtual nsIFrame* GetBoundsFrame();
  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
  PRBool IsVisible(PRBool *aIsOffscreen); 

  
  nsresult GetTextFromRelationID(nsIAtom *aIDAttrib, nsString &aName);

  











  already_AddRefed<nsIDOMNode> FindNeighbourPointingToThis(nsIAtom *aRelationAttr,
                                                           PRUint32 aRelationNameSpaceID = kNameSpaceID_None,
                                                           PRUint32 aAncestorLevelsToSearch = 0);

  













  static nsIContent *FindNeighbourPointingToNode(nsIContent *aForNode,
                                                 nsIAtom *aTagName,
                                                 nsIAtom *aRelationAttr,
                                                 PRUint32 aRelationNameSpaceID = kNameSpaceID_None,
                                                 PRUint32 aAncestorLevelsToSearch = 5);

  












  static nsIContent *FindDescendantPointingToID(const nsAString *aId,
                                                nsIContent *aLookContent,
                                                nsIAtom *aRelationAttr,
                                                PRUint32 aRelationNamespaceID = kNameSpaceID_None,
                                                nsIContent *aExcludeContent = nsnull,
                                                nsIAtom *aTagType = nsAccessibilityAtoms::label);

  static nsIContent *GetHTMLLabelContent(nsIContent *aForNode);
  static nsIContent *GetLabelContent(nsIContent *aForNode);
  static nsIContent *GetRoleContent(nsIDOMNode *aDOMNode);

  
  nsresult GetHTMLName(nsAString& _retval, PRBool aCanAggregateSubtree = PR_TRUE);
  nsresult GetXULName(nsAString& aName, PRBool aCanAggregateSubtree = PR_TRUE);
  
  
  nsresult AppendFlatStringFromSubtree(nsIContent *aContent, nsAString *aFlatString);
  nsresult AppendNameFromAccessibleFor(nsIContent *aContent, nsAString *aFlatString,
                                       PRBool aFromValue = PR_FALSE);
  nsresult AppendFlatStringFromContentNode(nsIContent *aContent, nsAString *aFlatString);
  nsresult AppendStringWithSpaces(nsAString *aFlatString, const nsAString& textEquivalent);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);
  nsresult AppendFlatStringFromSubtreeRecurse(nsIContent *aContent, nsAString *aFlatString);

  
  virtual void CacheChildren();
  
  
  
  nsIAccessible *NextChild(nsCOMPtr<nsIAccessible>& aAccessible);
    
  already_AddRefed<nsIAccessible> GetNextWithState(nsIAccessible *aStart, PRUint32 matchState);

  





   
  already_AddRefed<nsIAccessible> GetFirstAvailableAccessible(nsIDOMNode *aStartNode, PRBool aRequireLeaf = PR_FALSE);

  
  static already_AddRefed<nsIAccessible> GetMultiSelectFor(nsIDOMNode *aNode);

  
  virtual nsresult GetLinkOffset(PRInt32* aStartOffset, PRInt32* aEndOffset);

  
  static void DoCommandCallback(nsITimer *aTimer, void *aClosure);
  nsresult DoCommand(nsIContent *aContent = nsnull);

  
  PRBool CheckVisibilityInParentChain(nsIDocument* aDocument, nsIView* aView);

  
  nsCOMPtr<nsIAccessible> mParent;
  nsIAccessible *mFirstChild, *mNextSibling;
  nsRoleMapEntry *mRoleMapEntry; 
  PRInt32 mAccChildCount;
};


#endif  

