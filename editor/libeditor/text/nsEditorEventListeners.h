




































#ifndef editorInterfaces_h__
#define editorInterfaces_h__

#include "nsCOMPtr.h"

#include "nsIDOMEvent.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMFocusListener.h"

#include "nsIEditor.h"
#include "nsIPlaintextEditor.h"
#include "nsCaret.h"
#include "nsIPresShell.h"
#include "nsWeakPtr.h"
#include "nsIWeakReferenceUtils.h"

class nsIDOMDragEvent;





class nsTextEditorKeyListener : public nsIDOMKeyListener {
public:
  

  nsTextEditorKeyListener();
  

  virtual ~nsTextEditorKeyListener();

  


  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}


  NS_DECL_ISUPPORTS




  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);


protected:
  nsIEditor*     mEditor;		
};




class nsTextEditorTextListener : public nsIDOMTextListener
{
public:
  

  nsTextEditorTextListener();
  

  virtual ~nsTextEditorTextListener();

  


  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}


  NS_DECL_ISUPPORTS


    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
    NS_IMETHOD HandleText(nsIDOMEvent* aTextEvent);


protected:
  nsIEditor*      mEditor;		
	PRBool					mCommitText;
	PRBool					mInTransaction;
};


class nsIEditorIMESupport;

class nsTextEditorCompositionListener : public nsIDOMCompositionListener
{
public:
  

  nsTextEditorCompositionListener();
  

  virtual ~nsTextEditorCompositionListener();

  


  void SetEditor(nsIEditor *aEditor);


  NS_DECL_ISUPPORTS


  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD HandleEndComposition(nsIDOMEvent* aCompositionEvent);


protected:
  nsIEditorIMESupport*     mEditor;		
};




class nsTextEditorMouseListener : public nsIDOMMouseListener 
{
public:
  

  nsTextEditorMouseListener();
  

  virtual ~nsTextEditorMouseListener();

  


  void SetEditor(nsIEditor *aEditor){mEditor = aEditor;}


  NS_DECL_ISUPPORTS


  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);


protected:
  nsIEditor*     mEditor;		

};




class nsTextEditorDragListener : public nsIDOMEventListener 
{
public:
  

  nsTextEditorDragListener();
  

  virtual ~nsTextEditorDragListener();

  


  void SetEditor(nsIEditor *aEditor)          { mEditor = aEditor; }
  void SetPresShell(nsIPresShell *aPresShell) {
    mPresShell = do_GetWeakReference(aPresShell);
  }


  NS_DECL_ISUPPORTS

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

protected:

  PRBool   CanDrop(nsIDOMDragEvent* aEvent);
  nsresult DragEnter(nsIDOMDragEvent* aDragEvent);
  nsresult DragOver(nsIDOMDragEvent* aDragEvent);
  nsresult DragLeave(nsIDOMDragEvent* aDragEvent);
  nsresult Drop(nsIDOMDragEvent* aDragEvent);
  nsresult DragGesture(nsIDOMDragEvent* aDragEvent);

protected:
  nsIEditor* mEditor;
  nsWeakPtr  mPresShell;
  
  nsRefPtr<nsCaret> mCaret;
  PRBool            mCaretDrawn;
};



class nsTextEditorFocusListener : public nsIDOMFocusListener 
{
public:
  

  nsTextEditorFocusListener(nsIEditor *aEditor, nsIPresShell *aPresShell);
  

  virtual ~nsTextEditorFocusListener();


  NS_DECL_ISUPPORTS


  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);


protected:
  nsIEditor*     mEditor;		
  nsWeakPtr mPresShell;
};





extern nsresult NS_NewEditorKeyListener(nsIDOMEventListener ** aInstancePtrResult, nsIEditor *aEditor);



extern nsresult NS_NewEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, nsIEditor *aEditor);



extern nsresult NS_NewEditorTextListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor *aEditor);



extern nsresult NS_NewEditorDragListener(nsIDOMEventListener ** aInstancePtrResult, nsIPresShell* aPresShell,
                                            nsIEditor *aEditor);



extern nsresult NS_NewEditorCompositionListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor *aEditor);



extern nsresult
NS_NewEditorFocusListener(nsIDOMEventListener** aInstancePtrResult,
                          nsIEditor *aEditor, nsIPresShell *aPresShell);

#endif 

