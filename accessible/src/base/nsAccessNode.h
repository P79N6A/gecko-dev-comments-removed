









































#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsCOMPtr.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibleTypes.h"
#include "nsIAccessNode.h"
#include "nsIContent.h"
#include "nsPIAccessNode.h"
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
class nsApplicationAccessibleWrap;

#define ACCESSIBLE_BUNDLE_URL "chrome://global-platform/locale/accessible.properties"
#define PLATFORM_KEYS_BUNDLE_URL "chrome://global-platform/locale/platformKeys.properties"

typedef nsInterfaceHashtable<nsVoidPtrHashKey, nsIAccessNode>
        nsAccessNodeHashtable;








#define ARIARoleEquals(aContent, aRoleName) \
  nsAccessNode::ARIARoleEqualsImpl(aContent, aRoleName, NS_ARRAY_LENGTH(aRoleName) - 1)

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

    


    static already_AddRefed<nsApplicationAccessibleWrap> GetApplicationAccessible();

    
    static void PutCacheEntry(nsAccessNodeHashtable& aCache,
                              void* aUniqueID, nsIAccessNode *aAccessNode);
    static void GetCacheEntry(nsAccessNodeHashtable& aCache,
                              void* aUniqueID, nsIAccessNode **aAccessNode);
    static void ClearCache(nsAccessNodeHashtable& aCache);

    static PLDHashOperator PR_CALLBACK ClearCacheEntry(const void* aKey, nsCOMPtr<nsIAccessNode>& aAccessNode, void* aUserArg);

    
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIWeakReference *aPresShell);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsISupports *aContainer, PRBool aCanCreate = PR_FALSE);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIDOMNode *aNode);

    static already_AddRefed<nsIDOMNode> GetDOMNodeForContainer(nsISupports *aContainer);
    static already_AddRefed<nsIPresShell> GetPresShellFor(nsIDOMNode *aStartNode);
    
    
    static PRBool HasRoleAttribute(nsIContent *aContent)
    {
      return (aContent->IsNodeOfType(nsINode::eHTML) && aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::role)) ||
              aContent->HasAttr(kNameSpaceID_XHTML, nsAccessibilityAtoms::role) ||
              aContent->HasAttr(kNameSpaceID_XHTML2_Unofficial, nsAccessibilityAtoms::role);
    }

    





    static PRBool GetARIARole(nsIContent *aContent, nsString& aRole);

    static PRBool ARIARoleEqualsImpl(nsIContent* aContent, const char* aRoleName, PRUint32 aLen)
      { nsAutoString role; return GetARIARole(aContent, role) && role.EqualsASCII(aRoleName, aLen); }

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
    void LastRelease();

    nsCOMPtr<nsIDOMNode> mDOMNode;
    nsCOMPtr<nsIWeakReference> mWeakShell;

#ifdef DEBUG_A11Y
    PRBool mIsInitialized;
#endif

    


    static void NotifyA11yInitOrShutdown();

    
    static nsIStringBundle *gStringBundle;
    static nsIStringBundle *gKeyStringBundle;
    static nsITimer *gDoCommandTimer;
    static PRBool gIsAccessibilityActive;
    static PRBool gIsCacheDisabled;
    static PRBool gIsFormFillEnabled;

    static nsAccessNodeHashtable gGlobalDocAccessibleCache;

private:
  static nsIAccessibilityService *sAccService;
  static nsApplicationAccessibleWrap *gApplicationAccessible;
};

#endif

