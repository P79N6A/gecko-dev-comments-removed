




































#ifndef nsFormControlFrame_h___
#define nsFormControlFrame_h___

#include "nsIFormControlFrame.h"
#include "nsLeafFrame.h"






class nsFormControlFrame : public nsLeafFrame,
                           public nsIFormControlFrame
{

public:
  




  nsFormControlFrame(nsStyleContext*);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsLeafFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  NS_DECL_QUERYFRAME

  



  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  virtual nscoord GetBaseline() const;

  



  NS_IMETHOD Reflow(nsPresContext*      aCX,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  virtual void Destroy();

  

  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);

  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 
  
  
  static nsresult RegUnRegAccessKey(nsIFrame * aFrame, PRBool aDoReg);

  



  static nsRect GetUsableScreenRect(nsPresContext* aPresContext);

protected:

  virtual ~nsFormControlFrame();

  virtual nscoord GetIntrinsicWidth();
  virtual nscoord GetIntrinsicHeight();






   





  void GetCurrentCheckState(PRBool* aState);
};

#endif

