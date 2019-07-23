









































#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsCOMPtr.h"
#include "nsAccessibilityAtoms.h"
#include "nsCoreUtils.h"
#include "nsAccUtils.h"

#include "nsIAccessibleTypes.h"
#include "nsIAccessNode.h"
#include "nsIContent.h"
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
class nsIDocShellTreeItem;

#define ACCESSIBLE_BUNDLE_URL "chrome://global-platform/locale/accessible.properties"
#define PLATFORM_KEYS_BUNDLE_URL "chrome://global-platform/locale/platformKeys.properties"

typedef nsInterfaceHashtable<nsVoidPtrHashKey, nsIAccessNode>
        nsAccessNodeHashtable;






#define NS_INTERFACE_MAP_STATIC_AMBIGUOUS(_class) \
  if (aIID.Equals(NS_GET_IID(_class))) { \
  NS_ADDREF(this); \
  *aInstancePtr = this; \
  return NS_OK; \
  } else

#define NS_OK_DEFUNCT_OBJECT \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x22)

#define NS_ENSURE_A11Y_SUCCESS(res, ret)                                  \
  PR_BEGIN_MACRO                                                          \
    nsresult __rv = res; /* Don't evaluate |res| more than once */        \
    if (NS_FAILED(__rv)) {                                                \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                    \
      return ret;                                                         \
    }                                                                     \
    if (__rv == NS_OK_DEFUNCT_OBJECT)                                     \
      return ret;                                                         \
  PR_END_MACRO

#define NS_ACCESSNODE_IMPL_CID                          \
{  /* 13555f6e-0c0f-4002-84f6-558d47b8208e */           \
  0x13555f6e,                                           \
  0xc0f,                                                \
  0x4002,                                               \
  { 0x84, 0xf6, 0x55, 0x8d, 0x47, 0xb8, 0x20, 0x8e }    \
}

class nsAccessNode: public nsIAccessNode
{
  public: 
    nsAccessNode(nsIDOMNode *, nsIWeakReference* aShell);
    virtual ~nsAccessNode();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsAccessNode, nsIAccessNode)

    NS_DECL_NSIACCESSNODE
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSNODE_IMPL_CID)

    static void InitXPAccessibility();
    static void ShutdownXPAccessibility();

    


    static already_AddRefed<nsApplicationAccessibleWrap> GetApplicationAccessible();

    
    static void PutCacheEntry(nsAccessNodeHashtable& aCache,
                              void* aUniqueID, nsIAccessNode *aAccessNode);
    static void GetCacheEntry(nsAccessNodeHashtable& aCache,
                              void* aUniqueID, nsIAccessNode **aAccessNode);
    static void ClearCache(nsAccessNodeHashtable& aCache);

    static PLDHashOperator ClearCacheEntry(const void* aKey, nsCOMPtr<nsIAccessNode>& aAccessNode, void* aUserArg);

    
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIDocument *aDocument);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIWeakReference *aWeakShell);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIDocShellTreeItem *aContainer, PRBool aCanCreate = PR_FALSE);
    static already_AddRefed<nsIAccessibleDocument> GetDocAccessibleFor(nsIDOMNode *aNode);

    already_AddRefed<nsRootAccessible> GetRootAccessible();

    static nsIDOMNode *gLastFocusedNode;
    static nsIAccessibilityService* GetAccService();
    already_AddRefed<nsIDOMNode> GetCurrentFocus();

    


    virtual PRBool IsDefunct();

    


    virtual nsresult Init();

    


    virtual nsresult Shutdown();

    


    virtual nsIFrame* GetFrame();

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

    


    static void NotifyA11yInitOrShutdown(PRBool aIsInit);

    
    static nsIStringBundle *gStringBundle;
    static nsIStringBundle *gKeyStringBundle;
    static nsITimer *gDoCommandTimer;
#ifdef DEBUG
    static PRBool gIsAccessibilityActive;
#endif
    static PRBool gIsCacheDisabled;
    static PRBool gIsFormFillEnabled;

    static nsAccessNodeHashtable gGlobalDocAccessibleCache;

private:
  static nsApplicationAccessibleWrap *gApplicationAccessible;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessNode,
                              NS_ACCESSNODE_IMPL_CID)

#endif

