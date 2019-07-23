





































#ifndef nsIMEStateManager_h__
#define nsIMEStateManager_h__

#include "nscore.h"

class nsIContent;
class nsPIDOMWindow;
class nsPresContext;
class nsIWidget;
class nsIFocusController;
class nsTextStateManager;
class nsISelection;





class nsIMEStateManager
{
public:
  static nsresult OnDestroyPresContext(nsPresContext* aPresContext);
  static nsresult OnRemoveContent(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  static nsresult OnChangeFocus(nsPresContext* aPresContext,
                                nsIContent* aContent);
  static nsresult OnActivate(nsPresContext* aPresContext);
  static nsresult OnDeactivate(nsPresContext* aPresContext);
  static void OnInstalledMenuKeyboardListener(PRBool aInstalling);

  
  
  

  
  
  
  
  static nsresult OnTextStateBlur(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  
  
  
  static nsresult OnTextStateFocus(nsPresContext* aPresContext,
                                   nsIContent* aContent);
  
  static nsresult GetFocusSelectionAndRoot(nsISelection** aSel,
                                           nsIContent** aRoot);
protected:
  static void SetIMEState(nsPresContext* aPresContext,
                          PRUint32 aState,
                          nsIWidget* aKB);
  static PRUint32 GetNewIMEState(nsPresContext* aPresContext,
                                 nsIContent* aContent);

  static PRBool IsActive(nsPresContext* aPresContext);

  static nsIFocusController* GetFocusController(nsPresContext* aPresContext);
  static nsIWidget* GetWidget(nsPresContext* aPresContext);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static nsPIDOMWindow* sActiveWindow;
  static PRBool         sInstalledMenuKeyboardListener;

  static nsTextStateManager* sTextStateObserver;
};

#endif 
