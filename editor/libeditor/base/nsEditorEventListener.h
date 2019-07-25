





































#ifndef nsEditorEventListener_h__
#define nsEditorEventListener_h__

#include "nsCOMPtr.h"

#include "nsIDOMEvent.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMFocusListener.h"

#include "nsCaret.h"

class nsEditor;
class nsIDOMDragEvent;

class nsEditorEventListener : public nsIDOMKeyListener,
                              public nsIDOMTextListener,
                              public nsIDOMCompositionListener,
                              public nsIDOMMouseListener,
                              public nsIDOMFocusListener
{
public:
  nsEditorEventListener();
  virtual ~nsEditorEventListener();

  virtual nsresult Connect(nsEditor* aEditor);

  void Disconnect();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);

  
  NS_IMETHOD HandleText(nsIDOMEvent* aTextEvent);

  
  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD HandleEndComposition(nsIDOMEvent* aCompositionEvent);

  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);

protected:
  nsresult InstallToEditor();
  void UninstallFromEditor();

  PRBool CanDrop(nsIDOMDragEvent* aEvent);
  nsresult DragEnter(nsIDOMDragEvent* aDragEvent);
  nsresult DragOver(nsIDOMDragEvent* aDragEvent);
  nsresult DragExit(nsIDOMDragEvent* aDragEvent);
  nsresult Drop(nsIDOMDragEvent* aDragEvent);
  nsresult DragGesture(nsIDOMDragEvent* aDragEvent);
  void CleanupDragDropCaret();
  already_AddRefed<nsIPresShell> GetPresShell();

protected:
  nsEditor* mEditor; 
  nsRefPtr<nsCaret> mCaret;
  PRPackedBool mCommitText;
  PRPackedBool mInTransaction;
};

#endif 
