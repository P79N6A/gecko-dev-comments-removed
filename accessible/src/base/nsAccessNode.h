









































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

class nsAccessNode: public nsISupports
{
public:

  nsAccessNode(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsAccessNode();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsAccessNode)

    static void InitXPAccessibility();
    static void ShutdownXPAccessibility();

  


  static nsApplicationAccessible* GetApplicationAccessible();

  


  nsDocAccessible* Document() const { return mDoc; }

  


  nsRootAccessible* RootAccessible() const;

  





  already_AddRefed<nsINode> GetCurrentFocus();

  


  virtual bool Init();

  


  virtual void Shutdown();

  


  virtual bool IsDefunct() const;

  


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
  void ScrollTo(PRUint32 aType);

protected:
    nsPresContext* GetPresContext();

    void LastRelease();

  nsCOMPtr<nsIContent> mContent;
  nsDocAccessible* mDoc;

    


    static void NotifyA11yInitOrShutdown(bool aIsInit);

    
    static nsIStringBundle *gStringBundle;

    static bool gIsFormFillEnabled;

private:
  nsAccessNode() MOZ_DELETE;
  nsAccessNode(const nsAccessNode&) MOZ_DELETE;
  nsAccessNode& operator =(const nsAccessNode&) MOZ_DELETE;
  
  static nsApplicationAccessible *gApplicationAccessible;
};

#endif

