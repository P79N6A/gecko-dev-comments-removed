




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

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
#endif
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD HandleText(nsIDOMEvent* aTextEvent);
  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent);
  void       HandleEndComposition(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);

  void SpellCheckIfNeeded();

protected:
  virtual ~nsEditorEventListener();

  nsresult InstallToEditor();
  void UninstallFromEditor();

  bool CanDrop(nsIDOMDragEvent* aEvent);
  nsresult DragEnter(nsIDOMDragEvent* aDragEvent);
  nsresult DragOver(nsIDOMDragEvent* aDragEvent);
  nsresult DragExit(nsIDOMDragEvent* aDragEvent);
  nsresult Drop(nsIDOMDragEvent* aDragEvent);
  nsresult DragGesture(nsIDOMDragEvent* aDragEvent);
  void CleanupDragDropCaret();
  already_AddRefed<nsIPresShell> GetPresShell();
  bool IsFileControlTextBox();
  bool ShouldHandleNativeKeyBindings(nsIDOMEvent* aKeyEvent);

protected:
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
