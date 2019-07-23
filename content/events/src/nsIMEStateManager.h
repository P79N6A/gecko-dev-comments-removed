





































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
                                nsIContent* aContent);
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

  static nsIWidget* GetWidget(nsPresContext* aPresContext);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static PRBool         sInstalledMenuKeyboardListener;

  static nsTextStateManager* sTextStateObserver;
};

#endif 
