








#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsIAccessibleTypes.h"

#include "a11yGeneric.h"

#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsINameSpaceManager.h"
#include "nsIStringBundle.h"
#include "nsWeakReference.h"

class nsAccessNode;
class DocAccessible;
class nsIAccessibleDocument;

namespace mozilla {
namespace a11y {
class ApplicationAccessible;
class RootAccessible;
}
}

class nsIPresShell;
class nsPresContext;
class nsIFrame;
class nsIDocShellTreeItem;

class nsAccessNode: public nsISupports
{
public:

  nsAccessNode(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsAccessNode();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsAccessNode)

  static void ShutdownXPAccessibility();

  


  static mozilla::a11y::ApplicationAccessible* GetApplicationAccessible();

  


  DocAccessible* Document() const { return mDoc; }

  


  mozilla::a11y::RootAccessible* RootAccessible() const;

  


  virtual void Shutdown();

  


  virtual nsIFrame* GetFrame() const;
  


  virtual nsINode* GetNode() const { return mContent; }
  nsIContent* GetContent() const { return mContent; }
  virtual nsIDocument* GetDocumentNode() const
    { return mContent ? mContent->OwnerDoc() : nsnull; }

  


  bool IsContent() const
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

  


  void* UniqueID() { return static_cast<void*>(this); }

  






  virtual bool IsPrimaryForNode() const;

  


  void Language(nsAString& aLocale);

protected:
  void LastRelease();

  nsCOMPtr<nsIContent> mContent;
  DocAccessible* mDoc;

private:
  nsAccessNode() MOZ_DELETE;
  nsAccessNode(const nsAccessNode&) MOZ_DELETE;
  nsAccessNode& operator =(const nsAccessNode&) MOZ_DELETE;
  
  static mozilla::a11y::ApplicationAccessible* gApplicationAccessible;
};

#endif

