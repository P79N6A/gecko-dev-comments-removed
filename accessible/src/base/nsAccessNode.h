









































#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsIAccessNode.h"
#include "nsIAccessibleTypes.h"

#include "a11yGeneric.h"

#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"
#include "nsIStringBundle.h"
#include "nsRefPtrHashtable.h"
#include "nsWeakReference.h"

class nsAccessNode;
class nsApplicationAccessible;
class nsDocAccessible;
class nsIAccessibleDocument;
class nsRootAccessible;

class nsIPresShell;
class nsPresContext;
class nsIFrame;
class nsIDocShellTreeItem;

typedef nsRefPtrHashtable<nsVoidPtrHashKey, nsAccessNode>
  nsAccessNodeHashtable;

#define ACCESSIBLE_BUNDLE_URL "chrome://global-platform/locale/accessible.properties"
#define PLATFORM_KEYS_BUNDLE_URL "chrome://global-platform/locale/platformKeys.properties"

#define NS_ACCESSNODE_IMPL_CID                          \
{  /* 2b07e3d7-00b3-4379-aa0b-ea22e2c8ffda */           \
  0x2b07e3d7,                                           \
  0x00b3,                                               \
  0x4379,                                               \
  { 0xaa, 0x0b, 0xea, 0x22, 0xe2, 0xc8, 0xff, 0xda }    \
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

  


  static nsApplicationAccessible* GetApplicationAccessible();

  


  nsDocAccessible* GetDocAccessible() const;

  


  already_AddRefed<nsRootAccessible> GetRootAccessible();

    static nsIDOMNode *gLastFocusedNode;

    already_AddRefed<nsIDOMNode> GetCurrentFocus();

    


    virtual PRBool IsDefunct();

    


    virtual nsresult Init();

    


    virtual nsresult Shutdown();

    


    virtual nsIFrame* GetFrame();

  


  already_AddRefed<nsIPresShell> GetPresShell();

  



  PRBool HasWeakShell() const { return !!mWeakShell; }

#ifdef DEBUG
  


  PRBool IsInCache();
#endif

  


  static nsDocAccessible* GetDocAccessibleFor(nsIDocument *aDocument);
  static nsDocAccessible* GetDocAccessibleFor(nsIWeakReference *aWeakShell);
  static nsDocAccessible* GetDocAccessibleFor(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIAccessibleDocument>
    GetDocAccessibleFor(nsIDocShellTreeItem *aContainer,
                        PRBool aCanCreate = PR_FALSE);

protected:
    nsresult MakeAccessNode(nsIDOMNode *aNode, nsIAccessNode **aAccessNode);

    nsPresContext* GetPresContext();

    void LastRelease();

    nsCOMPtr<nsIDOMNode> mDOMNode;
    nsCOMPtr<nsIWeakReference> mWeakShell;

#ifdef DEBUG_A11Y
    PRBool mIsInitialized;
#endif

    


    static void NotifyA11yInitOrShutdown(PRBool aIsInit);

    
    static nsIStringBundle *gStringBundle;
    static nsIStringBundle *gKeyStringBundle;

    static PRBool gIsCacheDisabled;
    static PRBool gIsFormFillEnabled;

  static nsRefPtrHashtable<nsVoidPtrHashKey, nsDocAccessible>
    gGlobalDocAccessibleCache;

private:
  static nsApplicationAccessible *gApplicationAccessible;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessNode,
                              NS_ACCESSNODE_IMPL_CID)

#endif

