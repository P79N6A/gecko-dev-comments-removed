





































#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsAccessNodeWrap.h"

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


#define NS_OK_NO_ARIA_VALUE \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x21)


#define NS_OK_EMPTY_NAME \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x23)



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


#define NS_ACCESSIBLE_IMPL_CID                          \
{  /* 4E36C7A8-9203-4ef9-B619-271DDF6BB839 */           \
  0x4e36c7a8,                                           \
  0x9203,                                               \
  0x4ef9,                                               \
  { 0xb6, 0x19, 0x27, 0x1d, 0xdf, 0x6b, 0xb8, 0x39 }    \
}

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
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsAccessible, nsAccessNode)

  NS_DECL_NSIACCESSIBLE
  NS_DECL_NSPIACCESSIBLE
  NS_DECL_NSIACCESSIBLEHYPERLINK
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLEVALUE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSIBLE_IMPL_CID)

  
  

  virtual nsresult Shutdown();

  
  

  


  nsresult GetARIAName(nsAString& aName);

  









  virtual nsresult GetNameInternal(nsAString& aName);

  





  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  



  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  
  
  
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

  

  







  nsresult GetTextFromRelationID(nsIAtom *aIDProperty, nsString &aName);

  
  

  


  nsresult GetHTMLName(nsAString& aName);

  


  nsresult GetXULName(nsAString& aName);

  
  
  nsresult AppendFlatStringFromSubtree(nsIContent *aContent, nsAString *aFlatString);
  nsresult AppendNameFromAccessibleFor(nsIContent *aContent, nsAString *aFlatString,
                                       PRBool aFromValue = PR_FALSE);
  nsresult AppendFlatStringFromContentNode(nsIContent *aContent, nsAString *aFlatString);
  nsresult AppendStringWithSpaces(nsAString *aFlatString, const nsAString& textEquivalent);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);

  











  nsresult AppendFlatStringFromSubtreeRecurse(nsIContent *aContent,
                                              nsAString *aFlatString,
                                              PRBool aIsRootHidden);

  
  virtual void CacheChildren();
  
  
  
  nsIAccessible *NextChild(nsCOMPtr<nsIAccessible>& aAccessible);
    
  already_AddRefed<nsIAccessible> GetNextWithState(nsIAccessible *aStart, PRUint32 matchState);

  





   
  already_AddRefed<nsIAccessible> GetFirstAvailableAccessible(nsIDOMNode *aStartNode, PRBool aRequireLeaf = PR_FALSE);

  
  virtual nsresult GetLinkOffset(PRInt32* aStartOffset, PRInt32* aEndOffset);

  
  static void DoCommandCallback(nsITimer *aTimer, void *aClosure);
  nsresult DoCommand(nsIContent *aContent = nsnull);

  
  PRBool CheckVisibilityInParentChain(nsIDocument* aDocument, nsIView* aView);

  



  nsIDOMNode* GetAtomicRegion();

  







  nsresult GetAttrValue(nsIAtom *aAriaProperty, double *aValue);

  





  PRUint32 GetActionRule(PRUint32 aStates);

  







  nsresult ComputeGroupAttributes(PRUint32 aRole,
                                  nsIPersistentProperties *aAttributes);

  







  virtual nsresult FirePlatformEvent(nsIAccessibleEvent *aEvent) = 0;

  
  nsCOMPtr<nsIAccessible> mParent;
  nsCOMPtr<nsIAccessible> mFirstChild;
  nsCOMPtr<nsIAccessible> mNextSibling;

  nsRoleMapEntry *mRoleMapEntry; 
  PRInt32 mAccChildCount;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessible,
                              NS_ACCESSIBLE_IMPL_CID)

#endif  

