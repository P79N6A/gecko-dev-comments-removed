









































#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsIAccessNode.h"
#include "nsIAccessibleTypes.h"

#include "a11yGeneric.h"

#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"
#include "nsIStringBundle.h"
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

  nsAccessNode(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsAccessNode();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsAccessNode, nsIAccessNode)

    NS_DECL_NSIACCESSNODE
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCESSNODE_IMPL_CID)

    static void InitXPAccessibility();
    static void ShutdownXPAccessibility();

  


  static nsApplicationAccessible* GetApplicationAccessible();

  


  nsDocAccessible *GetDocAccessible() const;

  


  nsRootAccessible* RootAccessible() const;

  


  static nsINode *gLastFocusedNode;

  





  already_AddRefed<nsINode> GetCurrentFocus();

  


  virtual PRBool IsDefunct() { return !mContent; }

  


  virtual PRBool Init();

  


  virtual void Shutdown();

  


  virtual nsIFrame* GetFrame() const;

  


  already_AddRefed<nsIDOMNode> GetDOMNode() const
  {
    nsIDOMNode *DOMNode = nsnull;
    if (GetNode())
      CallQueryInterface(GetNode(), &DOMNode);
    return DOMNode;
  }

  


  virtual nsINode* GetNode() const { return mContent; }
  nsIContent* GetContent() const { return mContent; }
  virtual nsIDocument* GetDocumentNode() const
    { return mContent ? mContent->GetOwnerDoc() : nsnull; }

  


  PRBool IsContent() const
  {
    return GetNode() && GetNode()->IsNodeOfType(nsINode::eCONTENT);
  }
  bool IsElement() const
  {
    nsINode* node = GetNode();
    return node && node->IsElement();
  }
  bool IsDocumentNode() const
  {
    return GetNode() && GetNode()->IsNodeOfType(nsINode::eDOCUMENT);
  }

  


  already_AddRefed<nsIPresShell> GetPresShell();

  


  nsIWeakReference* GetWeakShell() const { return mWeakShell; }

  


  void* UniqueID() { return static_cast<void*>(this); }

  






  virtual bool IsPrimaryForNode() const;

protected:
    nsPresContext* GetPresContext();

    void LastRelease();

  nsCOMPtr<nsIContent> mContent;
  nsCOMPtr<nsIWeakReference> mWeakShell;

    


    static void NotifyA11yInitOrShutdown(PRBool aIsInit);

    
    static nsIStringBundle *gStringBundle;
    static nsIStringBundle *gKeyStringBundle;

    static PRBool gIsFormFillEnabled;

private:
  static nsApplicationAccessible *gApplicationAccessible;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessNode,
                              NS_ACCESSNODE_IMPL_CID)

#endif

