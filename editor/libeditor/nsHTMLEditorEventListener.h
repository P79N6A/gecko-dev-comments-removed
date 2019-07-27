




#ifndef nsHTMLEditorEventListener_h__
#define nsHTMLEditorEventListener_h__

#include "nsEditorEventListener.h"
#include "nscore.h"

class nsEditor;
class nsHTMLEditor;
class nsIDOMEvent;

class nsHTMLEditorEventListener : public nsEditorEventListener
{
public:
  nsHTMLEditorEventListener() :
    nsEditorEventListener()
  {
  }

  virtual ~nsHTMLEditorEventListener()
  {
  }

#ifdef DEBUG
  
  virtual nsresult Connect(nsEditor* aEditor);
#endif

  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);

protected:
  inline nsHTMLEditor* GetHTMLEditor();
};

#endif 

