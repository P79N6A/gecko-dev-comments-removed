





































#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"
#include "nsIObserver.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsIFrame;
class nsIWeakReference;
class nsIDOMNode;
class nsObjectFrame;
class nsIDocShell;
class nsIPresShell;
class nsIContent;

class nsAccessibilityService : public nsIAccessibilityService, 
                               public nsIObserver,
                               public nsIWebProgressListener,
                               public nsSupportsWeakReference
{
public:
  nsAccessibilityService();
  virtual ~nsAccessibilityService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLERETRIEVAL
  NS_DECL_NSIACCESSIBILITYSERVICE
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER

  static nsresult GetShellFromNode(nsIDOMNode *aNode, nsIWeakReference **weakShell);
  static nsresult GetAccessibilityService(nsIAccessibilityService** aResult);

private:
  nsresult GetInfo(nsISupports* aFrame, nsIFrame** aRealFrame, nsIWeakReference** aShell, nsIDOMNode** aContent);
  void GetOwnerFor(nsIPresShell *aPresShell, nsIPresShell **aOwnerShell, nsIContent **aOwnerContent);
  nsIContent* FindContentForDocShell(nsIPresShell* aPresShell, nsIContent* aContent, nsIDocShell*  aDocShell);
  static nsAccessibilityService *gAccessibilityService;
  nsresult InitAccessible(nsIAccessible *aAccessibleIn, nsIAccessible **aAccessibleOut);

  



  nsresult GetAccessibleByType(nsIDOMNode *aNode, nsIAccessible **aAccessible);
};

#endif 
