





































#ifndef _nsAccessible_H_
#define _nsAccessible_H_

#include "nsAccessNodeWrap.h"

#include "nsIAccessible.h"
#include "nsIAccessibleHyperLink.h"
#include "nsIAccessibleSelectable.h"
#include "nsIAccessibleValue.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"

#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsRefPtrHashtable.h"

class nsAccessible;
class nsAccEvent;
struct nsRoleMapEntry;

struct nsRect;
class nsIContent;
class nsIFrame;
class nsIAtom;
class nsIView;

typedef nsRefPtrHashtable<nsVoidPtrHashKey, nsAccessible>
  nsAccessibleHashtable;


#define NS_OK_NO_ARIA_VALUE \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x21)


#define NS_OK_EMPTY_NAME \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x23)


#define NS_OK_NAME_FROM_TOOLTIP \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x25)


#define NS_ACCESSIBLE_IMPL_IID                          \
{  /* 133c8bf4-4913-4355-bd50-426bd1d6e1ad */           \
  0x133c8bf4,                                           \
  0x4913,                                               \
  0x4355,                                               \
  { 0xbd, 0x50, 0x42, 0x6b, 0xd1, 0xd6, 0xe1, 0xad }    \
}

class nsAccessible : public nsAccessNodeWrap, 
                     public nsIAccessible, 
                     public nsIAccessibleHyperLink,
                     public nsIAccessibleSelectable,
                     public nsIAccessibleValue
{
public:
  nsAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsAccessible();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsAccessible, nsAccessNode)

  NS_DECL_NSIACCESSIBLE
  NS_DECL_NSIACCESSIBLEHYPERLINK
  NS_DECL_NSIACCESSIBLESELECTABLE
  NS_DECL_NSIACCESSIBLEVALUE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSIBLE_IMPL_IID)

  
  

  virtual PRBool Init();
  virtual void Shutdown();

  
  

  


  nsresult GetARIAName(nsAString& aName);

  







  virtual nsresult GetARIAState(PRUint32 *aState, PRUint32 *aExtraState);

  









  virtual nsresult GetNameInternal(nsAString& aName);

  





  virtual nsresult GetRoleInternal(PRUint32 *aRole);

  





  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  



  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  







  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

  


  virtual PRInt32 GetLevelInternal();

  






  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  
  

  






  virtual void SetRoleMapEntry(nsRoleMapEntry *aRoleMapEntry);

  


  PRBool EnsureChildren();

  





  virtual void InvalidateChildren();

  


  virtual PRBool AppendChild(nsAccessible* aChild);
  virtual PRBool InsertChildAt(PRUint32 aIndex, nsAccessible* aChild);
  virtual PRBool RemoveChild(nsAccessible* aChild);

  
  

  


  nsAccessible* GetParent();

  


  virtual nsAccessible* GetChildAt(PRUint32 aIndex);

  


  virtual PRInt32 GetChildCount();

  


  virtual PRInt32 GetIndexOf(nsAccessible* aChild);

  


  virtual PRInt32 GetIndexInParent();

  


  PRBool HasChildren() { return !!GetChildAt(0); }

  


  nsAccessible* GetCachedParent() const { return mParent; }
  nsAccessible* GetCachedNextSibling() const
  {
    return mParent ?
      mParent->mChildren.SafeElementAt(mIndexInParent + 1, nsnull) : nsnull;
  }
  nsAccessible* GetCachedPrevSibling() const
  {
    return mParent ?
      mParent->mChildren.SafeElementAt(mIndexInParent - 1, nsnull) : nsnull;
  }
  PRUint32 GetCachedChildCount() const { return mChildren.Length(); }
  PRBool AreChildrenCached() const { return mAreChildrenInitialized; }

#ifdef DEBUG
  


  PRBool IsInCache();
#endif

  
  

  



  virtual nsresult HandleAccEvent(nsAccEvent *aAccEvent);

  


  virtual PRBool GetAllowsAnonChildAccessibles();

  







  virtual nsresult AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                PRUint32 aLength);

  



  void TestChildCache(nsAccessible *aCachedChild);

protected:

  
  

  


  virtual void CacheChildren();

  


  void BindToParent(nsAccessible* aParent, PRUint32 aIndexInParent);
  void UnbindFromParent() { mParent = nsnull; mIndexInParent = -1; }

  


  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull);

  
  

  virtual nsIFrame* GetBoundsFrame();
  virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
  PRBool IsVisible(PRBool *aIsOffscreen); 

  
  

  


  nsresult GetHTMLName(nsAString& aName);

  


  nsresult GetXULName(nsAString& aName);

  
  static nsresult GetFullKeyName(const nsAString& aModifierName, const nsAString& aKeyName, nsAString& aStringOut);
  static nsresult GetTranslatedString(const nsAString& aKey, nsAString& aStringOut);

  







  nsAccessible *GetFirstAvailableAccessible(nsINode *aStartNode) const;

  
  virtual nsresult GetLinkOffset(PRInt32* aStartOffset, PRInt32* aEndOffset);

  
  

  











  void DoCommand(nsIContent *aContent = nsnull, PRUint32 aActionIndex = 0);

  


  virtual void DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex);

  NS_DECL_RUNNABLEMETHOD_ARG2(nsAccessible, DispatchClickEvent,
                              nsCOMPtr<nsIContent>, PRUint32)

  
  

  
  PRBool CheckVisibilityInParentChain(nsIDocument* aDocument, nsIView* aView);

  



  nsIDOMNode* GetAtomicRegion();

  







  nsresult GetAttrValue(nsIAtom *aAriaProperty, double *aValue);

  





  PRUint32 GetActionRule(PRUint32 aStates);

  







  virtual nsresult FirePlatformEvent(nsAccEvent *aEvent) = 0;

  
  nsRefPtr<nsAccessible> mParent;
  nsTArray<nsRefPtr<nsAccessible> > mChildren;
  PRBool mAreChildrenInitialized;
  PRInt32 mIndexInParent;

  nsRoleMapEntry *mRoleMapEntry; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessible,
                              NS_ACCESSIBLE_IMPL_IID)

#endif  

