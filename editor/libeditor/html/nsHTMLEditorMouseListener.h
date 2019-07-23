






































#ifndef htmlEditorMouseListener_h__
#define htmlEditorMouseListener_h__

#include "nsCOMPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMMouseListener.h"
#include "nsIEditor.h"
#include "nsIPlaintextEditor.h"
#include "nsIHTMLEditor.h"
#include "nsEditorEventListeners.h"
#include "nsHTMLEditor.h"

class nsString;

class nsHTMLEditorMouseListener : public nsTextEditorMouseListener
{
public:
  

  nsHTMLEditorMouseListener(nsHTMLEditor *aHTMLEditor);
  

  virtual ~nsHTMLEditorMouseListener();




  NS_DECL_ISUPPORTS_INHERITED


  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);


protected:

  nsHTMLEditor *mHTMLEditor; 
};



extern nsresult NS_NewHTMLEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, nsHTMLEditor *aHTMLEditor);

#endif 

