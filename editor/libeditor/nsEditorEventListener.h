




#ifndef nsEditorEventListener_h__
#define nsEditorEventListener_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsIDOMEventListener.h"
#include "nsISupportsImpl.h"
#include "nscore.h"

class nsCaret;
class nsIDOMEvent;
class nsIPresShell;


#ifdef KeyPress
#undef KeyPress
#endif

#ifdef XP_WIN

#define HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
#endif

class nsEditor;
class nsIDOMDragEvent;

class nsEditorEventListener : public nsIDOMEventListener
{
public:
  nsEditorEventListener();

  virtual nsresult Connect(nsEditor* aEditor);

  void Disconnect();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  void SpellCheckIfNeeded();

protected:
  virtual ~nsEditorEventListener();

  nsresult InstallToEditor();
  void UninstallFromEditor();

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  nsresult KeyDown(nsIDOMEvent* aKeyEvent);
  nsresult KeyUp(nsIDOMEvent* aKeyEvent);
#endif
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);
  nsresult HandleText(nsIDOMEvent* aTextEvent);
  nsresult HandleStartComposition(nsIDOMEvent* aCompositionEvent);
  void HandleEndComposition(nsIDOMEvent* aCompositionEvent);
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent);
  nsresult Focus(nsIDOMEvent* aEvent);
  nsresult Blur(nsIDOMEvent* aEvent);
  nsresult DragEnter(nsIDOMDragEvent* aDragEvent);
  nsresult DragOver(nsIDOMDragEvent* aDragEvent);
  nsresult DragExit(nsIDOMDragEvent* aDragEvent);
  nsresult Drop(nsIDOMDragEvent* aDragEvent);
  nsresult DragGesture(nsIDOMDragEvent* aDragEvent);

  bool CanDrop(nsIDOMDragEvent* aEvent);
  void CleanupDragDropCaret();
  already_AddRefed<nsIPresShell> GetPresShell();
  bool IsFileControlTextBox();
  bool ShouldHandleNativeKeyBindings(nsIDOMEvent* aKeyEvent);

  nsEditor* mEditor; 
  nsRefPtr<nsCaret> mCaret;
  bool mCommitText;
  bool mInTransaction;
#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  bool mHaveBidiKeyboards;
  bool mShouldSwitchTextDirection;
  bool mSwitchToRTL;
#endif
};

#endif 
