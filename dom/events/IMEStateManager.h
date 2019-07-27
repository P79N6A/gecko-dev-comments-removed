




#ifndef mozilla_IMEStateManager_h_
#define mozilla_IMEStateManager_h_

#include "mozilla/EventForwards.h"
#include "nsIWidget.h"

class nsIContent;
class nsIDOMMouseEvent;
class nsIEditor;
class nsINode;
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
                             nsIContent* aContent,
                             nsIEditor* aEditor);

  
  
  
  static bool OnMouseButtonEventInEditor(nsPresContext* aPresContext,
                                         nsIContent* aContent,
                                         nsIDOMMouseEvent* aMouseEvent);

  
  
  
  
  
  static void OnClickInEditor(nsPresContext* aPresContext,
                              nsIContent* aContent,
                              nsIDOMMouseEvent* aMouseEvent);

  
  
  
  
  
  static void OnFocusInEditor(nsPresContext* aPresContext,
                              nsIContent* aContent,
                              nsIEditor* aEditor);

  





  static void DispatchCompositionEvent(
                nsINode* aEventTargetNode,
                nsPresContext* aPresContext,
                WidgetCompositionEvent* aCompositionEvent,
                nsEventStatus* aStatus,
                EventDispatchingCallback* aCallBack,
                bool aIsSynthesized = false);

  



  static void OnCompositionEventDiscarded(
                const WidgetCompositionEvent* aCompositionEvent);

  


  static already_AddRefed<TextComposition>
    GetTextCompositionFor(nsIWidget* aWidget);

  




  static already_AddRefed<TextComposition>
    GetTextCompositionFor(WidgetGUIEvent* aGUIEvent);

  



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
  static void CreateIMEContentObserver(nsIEditor* aEditor);
  static void DestroyIMEContentObserver();

  static bool IsEditable(nsINode* node);

  static bool IsEditableIMEState(nsIWidget* aWidget);

  static nsIContent*    sContent;
  static nsPresContext* sPresContext;
  static bool           sInstalledMenuKeyboardListener;
  static bool           sIsTestingIME;
  static bool           sIsGettingNewIMEState;

  class MOZ_STACK_CLASS GettingNewIMEStateBlocker final
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
