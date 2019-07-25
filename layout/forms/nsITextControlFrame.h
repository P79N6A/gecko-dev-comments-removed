




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
  NS_DECL_QUERYFRAME_TARGET(nsITextControlFrame)

  enum SelectionDirection {
    eNone,
    eForward,
    eBackward
  };

  NS_IMETHOD    GetEditor(nsIEditor **aEditor) = 0;

  NS_IMETHOD    GetTextLength(PRInt32* aTextLength) = 0;
  
  NS_IMETHOD    SetSelectionStart(PRInt32 aSelectionStart) = 0;
  NS_IMETHOD    SetSelectionEnd(PRInt32 aSelectionEnd) = 0;
  
  NS_IMETHOD    SetSelectionRange(PRInt32 aSelectionStart,
                                  PRInt32 aSelectionEnd,
                                  SelectionDirection aDirection = eNone) = 0;
  NS_IMETHOD    GetSelectionRange(PRInt32* aSelectionStart,
                                  PRInt32* aSelectionEnd,
                                  SelectionDirection* aDirection = nsnull) = 0;

  NS_IMETHOD    GetOwnedSelectionController(nsISelectionController** aSelCon) = 0;
  virtual nsFrameSelection* GetOwnedFrameSelection() = 0;

  virtual nsresult GetPhonetic(nsAString& aPhonetic) = 0;

  




  virtual nsresult EnsureEditorInitialized() = 0;

  virtual nsresult ScrollSelectionIntoView() = 0;
};

#endif
