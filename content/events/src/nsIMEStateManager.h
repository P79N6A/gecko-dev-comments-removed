





































#ifndef nsIMEStateManager_h__
#define nsIMEStateManager_h__

#include "nscore.h"

class nsIContent;
class nsPIDOMWindow;
class nsPresContext;
class nsIWidget;
class nsTextStateManager;
class nsISelection;





class nsIMEStateManager
{
public:
  static nsresult OnDestroyPresContext(nsPresContext* aPresContext);
  static nsresult OnRemoveContent(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  static nsresult OnChangeFocus(nsPresContext* aPresContext,
                                nsIContent* aContent,
                                PRUint32 aReason);
  static void OnInstalledMenuKeyboardListener(PRBool aInstalling);

  
  
  

  
  
  
  
  static nsresult OnTextStateBlur(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  
  
  
  static nsresult OnTextStateFocus(nsPresContext* aPresContext,
                                   nsIContent* aContent);
  
  static nsresult GetFocusSelectionAndRoot(nsISelection** aSel,
                                           nsIContent** aRoot);
  
  
  
  
  
  
  static void UpdateIMEState(PRUint32 aNewIMEState, nsIContent* aContent);

protected:
  static void SetIMEState(PRUint32 aState, nsIContent* aContent,
                          nsIWidget* aWidget, PRUint32 aReason);
  static PRUint32 GetNewIMEState(nsPresContext* aPresContext,
                                 nsIContent* aContent);

  static nsIWidget* GetWidget(nsPresContext* aPresContext);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static PRBool         sInstalledMenuKeyboardListener;
  static PRBool         sInSecureInputMode;

  static nsTextStateManager* sTextStateObserver;
};

#endif 
