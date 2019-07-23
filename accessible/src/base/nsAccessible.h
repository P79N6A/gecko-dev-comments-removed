





































#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsAccessNodeWrap.h"

#include "nsARIAMap.h"
#include "nsRelUtils.h"
#include "nsTextEquivUtils.h"

#include "nsIAccessible.h"
#include "nsIAccessibleHyperLink.h"
#include "nsIAccessibleSelectable.h"
#include "nsIAccessibleValue.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"
#include "nsIAccessibleEvent.h"

#include "nsIDOMNodeList.h"
#include "nsINameSpaceManager.h"
#include "nsWeakReference.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIDOMDOMStringList.h"

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


#define NS_OK_NAME_FROM_TOOLTIP \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x25)



enum { eChildCountUninitialized = -1 };

class nsAccessibleDOMStringList : public nsIDOMDOMStringList
{
public:
  nsAccessibleDOMStringList();
  virtual ~nsAccessibleDOMStringList();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  PRBool Add(const nsAString& aName) {
    return mNames.AppendElement(aName) != nsnull;
  }

private:
  nsTArray<nsString> mNames;
};


#define NS_ACCESSIBLE_IMPL_CID                          \
{  /* 53cfa871-be42-47fc-b416-0033653b3151 */           \
  0x53cfa871,                                           \
  0xbe42,                                               \
  0x47fc,                                               \
  { 0xb4, 0x16, 0x00, 0x33, 0x65, 0x3b, 0x31, 0x51 }    \
}

class nsAccessible : public nsAccessNodeWrap, 
                     public nsIAccessible, 
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
  NS_DECL_NSIACCESSIBLEHYPERLINK
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLEVALUE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSIBLE_IMPL_CID)

  
  

  virtual nsresult Shutdown();

  
  

  


  nsresult GetARIAName(nsAString& aName);

  







  virtual nsresult GetARIAState(PRUint32 *aState, PRUint32 *aExtraState);

  









  virtual nsresult GetNameInternal(nsAString& aName);

  





  virtual nsresult GetRoleInternal(PRUint32 *aRole);

  





  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  



  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  







  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

  
  

  


  void SetParent(nsIAccessible *aParent);

  


  void SetFirstChild(nsIAccessible *aFirstChild);

  


  void SetNextSibling(nsIAccessible *aNextSibling);

  






  virtual void SetRoleMapEntry(nsRoleMapEntry *aRoleMapEntry);

  


  virtual void InvalidateChildren();

  


  already_AddRefed<nsIAccessible> GetCachedParent();

  


  already_AddRefed<nsIAccessible> GetCachedFirstChild();

  


  void TestChildCache(nsIAccessible *aCachedChild);

  
  

  


  virtual nsresult FireAccessibleEvent(nsIAccessibleEvent *aAccEvent);

  


  virtual PRBool GetAllowsAnonChildAccessibles();

  







  virtual nsresult AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                PRUint32 aLength);

  
  
  
  already_AddRefed<nsIAccessible> GetParent() {
    nsIAccessible *parent = nsnull;
    GetParent(&parent);
    return parent;
  }

protected:
  virtual nsIFrame* GetBoundsFrame();
  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
  PRBool IsVisible(PRBool *aIsOffscreen); 

  
  

  


  nsresult GetHTMLName(nsAString& aName);

  


  nsresult GetXULName(nsAString& aName);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);

  











  nsresult AppendFlatStringFromSubtreeRecurse(nsIContent *aContent,
                                              nsAString *aFlatString,
                                              PRBool aIsRootHidden);

  
  virtual void CacheChildren();
  
  
  
  nsIAccessible *NextChild(nsCOMPtr<nsIAccessible>& aAccessible);
    
  already_AddRefed<nsIAccessible> GetNextWithState(nsIAccessible *aStart, PRUint32 matchState);

  






   
  already_AddRefed<nsIAccessible>
    GetFirstAvailableAccessible(nsIDOMNode *aStartNode);

  
  virtual nsresult GetLinkOffset(PRInt32* aStartOffset, PRInt32* aEndOffset);

  
  

  


  struct nsCommandClosure
  {
    nsCommandClosure(nsAccessible *aAccessible, nsIContent *aContent,
                     PRUint32 aActionIndex) :
      accessible(aAccessible), content(aContent), actionIndex(aActionIndex) {}

    nsRefPtr<nsAccessible> accessible;
    nsCOMPtr<nsIContent> content;
    PRUint32 actionIndex;
  };

  











  nsresult DoCommand(nsIContent *aContent = nsnull, PRUint32 aActionIndex = 0);

  





  static void DoCommandCallback(nsITimer *aTimer, void *aClosure);

  


  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);

  
  

  
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

