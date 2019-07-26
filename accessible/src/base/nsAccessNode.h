








#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsIAccessibleTypes.h"
#include "nsINode.h"
#include "a11yGeneric.h"

class nsAccessNode;
class DocAccessible;
class nsIAccessibleDocument;
class nsIContent;

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
  


  virtual nsINode* GetNode() const;
  nsIContent* GetContent() const { return mContent; }
  virtual nsIDocument* GetDocumentNode() const;

  


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

