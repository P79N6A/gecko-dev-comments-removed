




































#ifndef nsIPercentHeightObserver_h___
#define nsIPercentHeightObserver_h___

struct nsHTMLReflowState;
class  nsPresContext;


#define NS_IPERCENTHEIGHTOBSERVER_IID \
 { 0x9cdc174b, 0x4f39, 0x41ad, {0xbc, 0x16, 0x5a, 0xc5, 0xa8, 0x64, 0x14, 0xa1}}






class nsIPercentHeightObserver : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPERCENTHEIGHTOBSERVER_IID)

  
  virtual void NotifyPercentHeight(const nsHTMLReflowState& aReflowState) = 0;

  
  virtual PRBool NeedsToObserve(const nsHTMLReflowState& aReflowState) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPercentHeightObserver,
                              NS_IPERCENTHEIGHTOBSERVER_IID)

#endif 
