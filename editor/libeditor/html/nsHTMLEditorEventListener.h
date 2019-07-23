






































#ifndef nsHTMLEditorEventListener_h__
#define nsHTMLEditorEventListener_h__

#include "nsEditorEventListener.h"
#include "nsHTMLEditor.h"

class nsHTMLEditorEventListener : public nsEditorEventListener
{
public:
  nsHTMLEditorEventListener(nsHTMLEditor* aEditor) :
    nsEditorEventListener(aEditor)
  {
  }

  virtual ~nsHTMLEditorEventListener()
  {
  }

  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
};

#endif 

