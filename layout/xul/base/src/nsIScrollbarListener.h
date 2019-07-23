





































#ifndef nsIScrollbarListener_h___
#define nsIScrollbarListener_h___


#define NS_ISCROLLBARLISTENER_IID \
{ 0xa0adbd81, 0x2911, 0x11d3, { 0x97, 0xfa, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

static NS_DEFINE_IID(kIScrollbarListenerIID,     NS_ISCROLLBARLISTENER_IID);

class nsPresContext;

class nsIScrollbarListener : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLBARLISTENER_IID)
  
  NS_IMETHOD PositionChanged(nsPresContext* aPresContext, PRInt32 aOldIndex, PRInt32& aNewIndex) = 0;

  NS_IMETHOD PagedUpDown() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollbarListener, NS_ISCROLLBARLISTENER_IID)

#endif

