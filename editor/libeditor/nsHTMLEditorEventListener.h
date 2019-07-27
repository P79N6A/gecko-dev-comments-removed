




#ifndef nsHTMLEditorEventListener_h__
#define nsHTMLEditorEventListener_h__

#include "nsEditorEventListener.h"
#include "nscore.h"

class nsEditor;
class nsHTMLEditor;

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
  
  virtual nsresult Connect(nsEditor* aEditor) override;
#endif

protected:
  virtual nsresult MouseDown(nsIDOMMouseEvent* aMouseEvent) override;
  virtual nsresult MouseUp(nsIDOMMouseEvent* aMouseEvent) override;
  virtual nsresult MouseClick(nsIDOMMouseEvent* aMouseEvent) override;

  inline nsHTMLEditor* GetHTMLEditor();
};

#endif 

