





#ifndef nsITextControlElement_h___
#define nsITextControlElement_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
class nsIContent;
class nsAString;
class nsIEditor;
class nsISelectionController;
class nsFrameSelection;
class nsTextControlFrame;


#define NS_ITEXTCONTROLELEMENT_IID    \
{ 0x3dd53b59, 0x9d8f, 0x40a3, \
  { 0x81, 0xd7, 0xb3, 0x43, 0xa0, 0x51, 0xfc, 0xb5 } }





class nsITextControlElement : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEXTCONTROLELEMENT_IID)

  


  NS_IMETHOD SetValueChanged(bool changed) = 0;

  



  NS_IMETHOD_(bool) IsSingleLineTextControl() const = 0;

  



  NS_IMETHOD_(bool) IsTextArea() const = 0;

  



  NS_IMETHOD_(bool) IsPlainTextControl() const = 0;

  



  NS_IMETHOD_(bool) IsPasswordTextControl() const = 0;

  



  NS_IMETHOD_(int32_t) GetCols() = 0;

  


  NS_IMETHOD_(int32_t) GetWrapCols() = 0;

  



  NS_IMETHOD_(int32_t) GetRows() = 0;

  


  NS_IMETHOD_(void) GetDefaultValueFromContent(nsAString& aValue) = 0;

  


  NS_IMETHOD_(bool) ValueChanged() const = 0;

  






  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, bool aIgnoreWrap) const = 0;

  




  NS_IMETHOD_(nsIEditor*) GetTextEditor() = 0;

  




  NS_IMETHOD_(nsISelectionController*) GetSelectionController() = 0;

  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection() = 0;

  



  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame) = 0;

  



  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame) = 0;

  




  NS_IMETHOD CreateEditor() = 0;

  


  NS_IMETHOD_(nsIContent*) GetRootEditorNode() = 0;

  


  NS_IMETHOD_(nsIContent*) CreatePlaceholderNode() = 0;

  


  NS_IMETHOD_(nsIContent*) GetPlaceholderNode() = 0;

  


  NS_IMETHOD_(void) InitializeKeyboardEventListeners() = 0;

  


  NS_IMETHOD_(void) UpdatePlaceholderVisibility(bool aNotify) = 0;

  


  NS_IMETHOD_(bool) GetPlaceholderVisibility() = 0;

  


  NS_IMETHOD_(void) OnValueChanged(bool aNotify) = 0;

  static const int32_t DEFAULT_COLS = 20;
  static const int32_t DEFAULT_ROWS = 1;
  static const int32_t DEFAULT_ROWS_TEXTAREA = 2;
  static const int32_t DEFAULT_UNDO_CAP = 1000;

  
  typedef enum {
    eHTMLTextWrap_Off     = 1,    
    eHTMLTextWrap_Hard    = 2,    
    eHTMLTextWrap_Soft    = 3     
  } nsHTMLTextWrap;

  static bool
  GetWrapPropertyEnum(nsIContent* aContent, nsHTMLTextWrap& aWrapProp);

  





  NS_IMETHOD_(bool) HasCachedSelection() = 0;

  static already_AddRefed<nsITextControlElement>
  GetTextControlElementFromEditingHost(nsIContent* aHost);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextControlElement,
                              NS_ITEXTCONTROLELEMENT_IID)

#endif 

