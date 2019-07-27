




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

protected:
  virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent) MOZ_OVERRIDE;
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent) MOZ_OVERRIDE;
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent) MOZ_OVERRIDE;

  inline nsHTMLEditor* GetHTMLEditor();
};

#endif 

