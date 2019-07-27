




#ifndef mozilla_IMEStateManager_h_
#define mozilla_IMEStateManager_h_

#include "mozilla/EventForwards.h"
#include "nsIWidget.h"

class nsIContent;
class nsIDOMMouseEvent;
class nsINode;
class nsPIDOMWindow;
class nsPresContext;
class nsISelection;

namespace mozilla {

class EventDispatchingCallback;
class IMEContentObserver;
class TextCompositionArray;
class TextComposition;







class IMEStateManager
{
  typedef widget::IMEMessage IMEMessage;
  typedef widget::IMEState IMEState;
  typedef widget::InputContext InputContext;
  typedef widget::InputContextAction InputContextAction;

public:
  static void Init();
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

  
  
  
  static bool OnMouseButtonEventInEditor(nsPresContext* aPresContext,
                                         nsIContent* aContent,
                                         nsIDOMMouseEvent* aMouseEvent);

  
  
  
  
  
  static void OnClickInEditor(nsPresContext* aPresContext,
                              nsIContent* aContent,
                              nsIDOMMouseEvent* aMouseEvent);

  
  
  
  
  
  static void OnFocusInEditor(nsPresContext* aPresContext,
                              nsIContent* aContent);

  






  static void DispatchCompositionEvent(nsINode* aEventTargetNode,
                                       nsPresContext* aPresContext,
                                       WidgetEvent* aEvent,
                                       nsEventStatus* aStatus,
                                       EventDispatchingCallback* aCallBack);

  


  static already_AddRefed<TextComposition>
    GetTextCompositionFor(nsIWidget* aWidget);

  





  static already_AddRefed<TextComposition>
    GetTextCompositionFor(WidgetGUIEvent* aEvent);

  



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
  static void DestroyIMEContentObserver();

  static bool IsEditable(nsINode* node);

  static bool IsEditableIMEState(nsIWidget* aWidget);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static bool           sInstalledMenuKeyboardListener;
  static bool           sIsTestingIME;
  static bool           sIsGettingNewIMEState;

  class MOZ_STACK_CLASS GettingNewIMEStateBlocker MOZ_FINAL
  {
  public:
    GettingNewIMEStateBlocker()
      : mOldValue(IMEStateManager::sIsGettingNewIMEState)
    {
      IMEStateManager::sIsGettingNewIMEState = true;
    }
    ~GettingNewIMEStateBlocker()
    {
      IMEStateManager::sIsGettingNewIMEState = mOldValue;
    }
  private:
    bool mOldValue;
  };

  static IMEContentObserver* sActiveIMEContentObserver;

  
  
  
  
  static TextCompositionArray* sTextCompositions;
};

} 

#endif 
