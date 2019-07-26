








#ifndef _nsAccessNode_H_
#define _nsAccessNode_H_

#include "nsIAccessibleTypes.h"
#include "nsINode.h"

class nsIContent;
class nsIDocShellTreeItem;
class nsIFrame;
class nsIPresShell;
class nsPresContext;

namespace mozilla {
namespace a11y {

class DocAccessible;
class RootAccessible;

class nsAccessNode : public nsISupports
{
public:

  nsAccessNode(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsAccessNode();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsAccessNode)

  


  DocAccessible* Document() const { return mDoc; }

  


  a11y::RootAccessible* RootAccessible() const;

  


  virtual void Shutdown();

  


  virtual nsIFrame* GetFrame() const;

  


  virtual nsINode* GetNode() const;
  nsIContent* GetContent() const { return mContent; }

  


  bool IsContent() const
  {
    return GetNode() && GetNode()->IsNodeOfType(nsINode::eCONTENT);
  }

  


  void* UniqueID() { return static_cast<void*>(this); }

  


  void Language(nsAString& aLocale);

protected:
  void LastRelease();

  nsCOMPtr<nsIContent> mContent;
  DocAccessible* mDoc;

private:
  nsAccessNode() MOZ_DELETE;
  nsAccessNode(const nsAccessNode&) MOZ_DELETE;
  nsAccessNode& operator =(const nsAccessNode&) MOZ_DELETE;
};

} 
} 

#endif

