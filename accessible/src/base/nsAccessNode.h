









































#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsCOMPtr.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessNode.h"
#include "nsIContent.h"
#include "nsPIAccessNode.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"
#include "nsIStringBundle.h"
#include "nsWeakReference.h"
#include "nsInterfaceHashtable.h"
#include "nsIAccessibilityService.h"

class nsIPresShell;
class nsPresContext;
class nsIAccessibleDocument;
class nsIFrame;
class nsIDOMNodeList;
class nsITimer;
class nsRootAccessible;

#define ACCESSIBLE_BUNDLE_URL "chrome://global-platform/locale/accessible.properties"
#define PLATFORM_KEYS_BUNDLE_URL "chrome://global-platform/locale/platformKeys.properties"





class nsVoidHashKey : public PLDHashEntryHdr
{
public:
  typedef const void* KeyType;
  typedef const void* KeyTypePointer;
  
  nsVoidHashKey(KeyTypePointer aKey) : mValue(aKey) { }
  nsVoidHashKey(const nsVoidHashKey& toCopy) : mValue(toCopy.mValue) { }
  ~nsVoidHashKey() { }

  KeyType GetKey() const { return mValue; }
  KeyTypePointer GetKeyPointer() const { return mValue; }
  PRBool KeyEquals(KeyTypePointer aKey) const { return aKey == mValue; }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) { return NS_PTR_TO_INT32(aKey) >> 2; }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const void* mValue;
};

class nsAccessNode: public nsIAccessNode, public nsPIAccessNode
{
  public: 
    nsAccessNode(nsIDOMNode *, nsIWeakReference* aShell);
    virtual ~nsAccessNode();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIACCESSNODE
    NS_DECL_NSPIACCESSNODE

    static void InitXPAccessibility();
    static void ShutdownXPAccessibility();

    
    static void PutCacheEntry(nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode>& aCache, 
                              void* aUniqueID, nsIAccessNode *aAccessNode);
    static void GetCacheEntry(nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode>& aCache, void* aUniqueID, 
                              nsIAccessNode **aAccessNode);
    static void ClearCache(nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode>& aCache);

    static PLDHashOperator PR_CALLBACK ClearCacheEntry(const void* aKey, nsCOMPtr<nsIAccessNode>& aAccessNode, void* aUserArg);

    
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIWeakReference *aPresShell);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsISupports *aContainer, PRBool aCanCreate = PR_FALSE);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIDOMNode *aNode);

    static already_AddRefed<nsIDocShellTreeItem> GetDocShellTreeItemFor(nsIDOMNode *aStartNode);
    static already_AddRefed<nsIDOMNode> GetDOMNodeForContainer(nsISupports *aContainer);
    static already_AddRefed<nsIPresShell> GetPresShellFor(nsIDOMNode *aStartNode);
    
    
    static PRBool HasRoleAttribute(nsIContent *aContent)
    {
      return (aContent->IsNodeOfType(nsINode::eHTML) && aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::role)) ||
              aContent->HasAttr(kNameSpaceID_XHTML, nsAccessibilityAtoms::role) ||
              aContent->HasAttr(kNameSpaceID_XHTML2_Unofficial, nsAccessibilityAtoms::role);
    }

    
    static PRBool GetRoleAttribute(nsIContent *aContent, nsAString& aRole)
    {
      aRole.Truncate();
      return (aContent->IsNodeOfType(nsINode::eHTML) && aContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::role, aRole)) ||
              aContent->GetAttr(kNameSpaceID_XHTML, nsAccessibilityAtoms::role, aRole) ||
              aContent->GetAttr(kNameSpaceID_XHTML2_Unofficial, nsAccessibilityAtoms::role, aRole);
    }

    static void GetComputedStyleDeclaration(const nsAString& aPseudoElt,
                                            nsIDOMElement *aElement,
                                            nsIDOMCSSStyleDeclaration **aCssDecl);

    already_AddRefed<nsRootAccessible> GetRootAccessible();

    static nsIDOMNode *gLastFocusedNode;
    static nsIAccessibilityService* GetAccService();
    already_AddRefed<nsIDOMNode> GetCurrentFocus();

protected:
    nsresult MakeAccessNode(nsIDOMNode *aNode, nsIAccessNode **aAccessNode);
    already_AddRefed<nsIPresShell> GetPresShell();
    nsPresContext* GetPresContext();
    already_AddRefed<nsIAccessibleDocument> GetDocAccessible();

    nsCOMPtr<nsIDOMNode> mDOMNode;
    nsCOMPtr<nsIWeakReference> mWeakShell;

#ifdef DEBUG_A11Y
    PRBool mIsInitialized;
#endif

    
    static nsIStringBundle *gStringBundle;
    static nsIStringBundle *gKeyStringBundle;
    static nsITimer *gDoCommandTimer;
    static PRBool gIsAccessibilityActive;
    static PRBool gIsCacheDisabled;
    static PRBool gIsFormFillEnabled;

    static nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> gGlobalDocAccessibleCache;

private:
  static nsIAccessibilityService *sAccService;
};

#endif

