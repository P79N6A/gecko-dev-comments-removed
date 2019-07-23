




































#ifndef nsITextControlFrame_h___
#define nsITextControlFrame_h___
 
#include "nsIFormControlFrame.h"

class nsIEditor;
class nsIDocShell;
class nsISelectionController;
class nsFrameSelection;

#define NS_IGFXTEXTCONTROLFRAME2_IID \
{/* 0c3b64da-4431-11da-94fd-00e08161165f*/ \
0xc3b64da, 0x4431, 0x11da, \
{ 0x94, 0xfd, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f } }

class nsITextControlFrame : public nsIFormControlFrame
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGFXTEXTCONTROLFRAME2_IID)

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
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextControlFrame,
                              NS_IGFXTEXTCONTROLFRAME2_IID)

#endif
