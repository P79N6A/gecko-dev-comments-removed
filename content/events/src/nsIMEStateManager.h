





































#ifndef nsIMEStateManager_h__
#define nsIMEStateManager_h__

#include "nscore.h"
#include "nsIWidget.h"

class nsIContent;
class nsPIDOMWindow;
class nsPresContext;
class nsTextStateManager;
class nsISelection;





class nsIMEStateManager
{
protected:
  typedef mozilla::widget::InputContext InputContext;
  typedef mozilla::widget::InputContextAction InputContextAction;

public:
  static nsresult OnDestroyPresContext(nsPresContext* aPresContext);
  static nsresult OnRemoveContent(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  




  static nsresult OnChangeFocus(nsPresContext* aPresContext,
                                nsIContent* aContent,
                                InputContextAction::Cause aCause);
  static void OnInstalledMenuKeyboardListener(bool aInstalling);

  
  
  

  
  
  
  
  static nsresult OnTextStateBlur(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  
  
  
  static nsresult OnTextStateFocus(nsPresContext* aPresContext,
                                   nsIContent* aContent);
  
  static nsresult GetFocusSelectionAndRoot(nsISelection** aSel,
                                           nsIContent** aRoot);
  
  
  
  
  
  
  static void UpdateIMEState(PRUint32 aNewIMEState, nsIContent* aContent);

protected:
  static nsresult OnChangeFocusInternal(nsPresContext* aPresContext,
                                        nsIContent* aContent,
                                        InputContextAction aAction);
  static void SetIMEState(PRUint32 aState, nsIContent* aContent,
                          nsIWidget* aWidget,
                          InputContextAction aAction);
  static PRUint32 GetNewIMEState(nsPresContext* aPresContext,
                                 nsIContent* aContent);

  static nsIWidget* GetWidget(nsPresContext* aPresContext);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static bool           sInstalledMenuKeyboardListener;
  static bool           sInSecureInputMode;

  static nsTextStateManager* sTextStateObserver;
};

#endif 
