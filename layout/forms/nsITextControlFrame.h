




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

  NS_IMETHOD    GetTextLength(int32_t* aTextLength) = 0;
  
  NS_IMETHOD    SetSelectionStart(int32_t aSelectionStart) = 0;
  NS_IMETHOD    SetSelectionEnd(int32_t aSelectionEnd) = 0;
  
  NS_IMETHOD    SetSelectionRange(int32_t aSelectionStart,
                                  int32_t aSelectionEnd,
                                  SelectionDirection aDirection = eNone) = 0;
  NS_IMETHOD    GetSelectionRange(int32_t* aSelectionStart,
                                  int32_t* aSelectionEnd,
                                  SelectionDirection* aDirection = nullptr) = 0;

  NS_IMETHOD    GetOwnedSelectionController(nsISelectionController** aSelCon) = 0;
  virtual nsFrameSelection* GetOwnedFrameSelection() = 0;

  virtual nsresult GetPhonetic(nsAString& aPhonetic) = 0;

  




  virtual nsresult EnsureEditorInitialized() = 0;

  virtual nsresult ScrollSelectionIntoView() = 0;
};

#endif
