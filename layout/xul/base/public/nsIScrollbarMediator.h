






































#ifndef nsIScrollbarMediator_h___
#define nsIScrollbarMediator_h___


#define NS_ISCROLLBARMEDIATOR_IID \
{ 0xb589027f, 0x271b, 0x4c68, { 0x91, 0xdf, 0x04, 0xf1, 0x39, 0x88, 0x5e, 0x9a } }

static NS_DEFINE_IID(kIScrollbarMediatorIID, NS_ISCROLLBARMEDIATOR_IID);

class nsIScrollbarMediator : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLBARMEDIATOR_IID)

  
  
  

  NS_IMETHOD PositionChanged(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex) = 0;
  NS_IMETHOD ScrollbarButtonPressed(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

  NS_IMETHOD VisibilityChanged(nsISupports* aScrollbar, PRBool aVisible) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollbarMediator, NS_ISCROLLBARMEDIATOR_IID)

#endif

