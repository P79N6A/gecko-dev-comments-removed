




#ifndef nsFormControlFrame_h___
#define nsFormControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsIFormControlFrame.h"
#include "nsLeafFrame.h"






class nsFormControlFrame : public nsLeafFrame,
                           public nsIFormControlFrame
{
public:
  




  nsFormControlFrame(nsStyleContext*);

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsLeafFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  



  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual nscoord GetBaseline() const MOZ_OVERRIDE;

  



  NS_IMETHOD Reflow(nsPresContext*      aCX,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  

  virtual void SetFocus(bool aOn = true, bool aRepaint = false);

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) MOZ_OVERRIDE;

  
  static nsresult RegUnRegAccessKey(nsIFrame * aFrame, bool aDoReg);

  



  static nsRect GetUsableScreenRect(nsPresContext* aPresContext);

protected:

  virtual ~nsFormControlFrame();

  virtual nscoord GetIntrinsicWidth() MOZ_OVERRIDE;
  virtual nscoord GetIntrinsicHeight() MOZ_OVERRIDE;






   





  void GetCurrentCheckState(bool* aState);
};

#endif

