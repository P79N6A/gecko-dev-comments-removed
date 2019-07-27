




#ifndef nsFormControlFrame_h___
#define nsFormControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsIFormControlFrame.h"
#include "nsLeafFrame.h"






class nsFormControlFrame : public nsLeafFrame,
                           public nsIFormControlFrame
{
public:
  




  explicit nsFormControlFrame(nsStyleContext*);

  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsLeafFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  



  virtual nsresult HandleEvent(nsPresContext* aPresContext, 
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;

  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode)
    const override;

  



  virtual void Reflow(nsPresContext*      aCX,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&      aStatus) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  

  virtual void SetFocus(bool aOn = true, bool aRepaint = false) override;

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) override;

  
  static nsresult RegUnRegAccessKey(nsIFrame * aFrame, bool aDoReg);

  



  static nsRect GetUsableScreenRect(nsPresContext* aPresContext);

protected:

  virtual ~nsFormControlFrame();

  virtual nscoord GetIntrinsicISize() override;
  virtual nscoord GetIntrinsicBSize() override;






   





  void GetCurrentCheckState(bool* aState);
};

#endif

