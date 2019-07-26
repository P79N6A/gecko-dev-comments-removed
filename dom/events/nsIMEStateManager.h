




#ifndef nsIMEStateManager_h__
#define nsIMEStateManager_h__

#include "mozilla/EventForwards.h"
#include "nsIWidget.h"

class nsDispatchingCallback;
class nsIContent;
class nsIDOMMouseEvent;
class nsINode;
class nsPIDOMWindow;
class nsPresContext;
class nsISelection;

namespace mozilla {
class IMEContentObserver;
class TextCompositionArray;
class TextComposition;
} 





class nsIMEStateManager
{
  friend class nsTextStateManager;
protected:
  typedef mozilla::widget::IMEMessage IMEMessage;
  typedef mozilla::widget::IMEState IMEState;
  typedef mozilla::widget::InputContext InputContext;
  typedef mozilla::widget::InputContextAction InputContextAction;

public:
  static void Shutdown();

  static nsresult OnDestroyPresContext(nsPresContext* aPresContext);
  static nsresult OnRemoveContent(nsPresContext* aPresContext,
                                  nsIContent* aContent);
  




  static nsresult OnChangeFocus(nsPresContext* aPresContext,
                                nsIContent* aContent,
                                InputContextAction::Cause aCause);
  static void OnInstalledMenuKeyboardListener(bool aInstalling);

  
  
  

  
  static nsresult GetFocusSelectionAndRoot(nsISelection** aSel,
                                           nsIContent** aRoot);
  
  
  
  
  static void UpdateIMEState(const IMEState &aNewIMEState,
                             nsIContent* aContent);

  
  
  
  
  
  static void OnClickInEditor(nsPresContext* aPresContext,
                              nsIContent* aContent,
                              nsIDOMMouseEvent* aMouseEvent);

  
  
  
  
  
  static void OnFocusInEditor(nsPresContext* aPresContext,
                              nsIContent* aContent);

  






  static void DispatchCompositionEvent(nsINode* aEventTargetNode,
                                       nsPresContext* aPresContext,
                                       mozilla::WidgetEvent* aEvent,
                                       nsEventStatus* aStatus,
                                       nsDispatchingCallback* aCallBack);

  


  static already_AddRefed<mozilla::TextComposition>
    GetTextCompositionFor(nsIWidget* aWidget);

  





  static already_AddRefed<mozilla::TextComposition>
    GetTextCompositionFor(mozilla::WidgetGUIEvent* aEvent);

  



  static nsresult NotifyIME(IMEMessage aMessage, nsIWidget* aWidget);
  static nsresult NotifyIME(IMEMessage aMessage, nsPresContext* aPresContext);

  static nsINode* GetRootEditableNode(nsPresContext* aPresContext,
                                      nsIContent* aContent);
  static bool IsTestingIME() { return sIsTestingIME; }

protected:
  static nsresult OnChangeFocusInternal(nsPresContext* aPresContext,
                                        nsIContent* aContent,
                                        InputContextAction aAction);
  static void SetIMEState(const IMEState &aState,
                          nsIContent* aContent,
                          nsIWidget* aWidget,
                          InputContextAction aAction);
  static IMEState GetNewIMEState(nsPresContext* aPresContext,
                                 nsIContent* aContent);

  static void EnsureTextCompositionArray();
  static void CreateIMEContentObserver();
  static void DestroyTextStateManager();

  static bool IsEditable(nsINode* node);

  static bool IsEditableIMEState(nsIWidget* aWidget);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static bool           sInstalledMenuKeyboardListener;
  static bool           sIsTestingIME;

  static mozilla::IMEContentObserver* sActiveIMEContentObserver;

  
  
  
  
  static mozilla::TextCompositionArray* sTextCompositions;
};

#endif 
