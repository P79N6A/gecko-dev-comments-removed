




































#ifndef nsITextControlFrame_h___
#define nsITextControlFrame_h___
 
#include "nsIFormControlFrame.h"

class nsIEditor;
class nsIDocShell;
class nsISelectionController;
class nsFrameSelection;

class nsITextControlFrame : public nsIFormControlFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsITextControlFrame)

  NS_IMETHOD    GetEditor(nsIEditor **aEditor) = 0;

  





  NS_IMETHOD    OwnsValue(PRBool* aOwnsValue) = 0;

  







  NS_IMETHOD    GetValue(nsAString& aValue, PRBool aIgnoreWrap) const = 0;
  
  NS_IMETHOD    GetTextLength(PRInt32* aTextLength) = 0;
  
  



  NS_IMETHOD    CheckFireOnChange() = 0;
  NS_IMETHOD    SetSelectionStart(PRInt32 aSelectionStart) = 0;
  NS_IMETHOD    SetSelectionEnd(PRInt32 aSelectionEnd) = 0;
  
  NS_IMETHOD    SetSelectionRange(PRInt32 aSelectionStart, PRInt32 aSelectionEnd) = 0;
  NS_IMETHOD    GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd) = 0;

  virtual nsISelectionController* GetOwnedSelectionController() = 0;
  virtual nsFrameSelection* GetOwnedFrameSelection() = 0;

  virtual nsresult GetPhonetic(nsAString& aPhonetic) = 0;
};

#endif
