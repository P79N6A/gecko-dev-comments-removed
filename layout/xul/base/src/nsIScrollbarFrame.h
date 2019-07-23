





































#ifndef nsIScrollbarFrame_h___
#define nsIScrollbarFrame_h___


#define NS_ISCROLLBARFRAME_IID \
{ 0x660e5ed6, 0x1cf7, 0x47ff, { 0xb0, 0x94, 0xc5, 0x88, 0xc2, 0x19, 0x86, 0xe8 } }

static NS_DEFINE_IID(kIScrollbarFrameIID,     NS_ISCROLLBARFRAME_IID);

class nsIScrollbarMediator;

class nsIScrollbarFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLBARFRAME_IID)
  
  
  
  virtual void SetScrollbarMediatorContent(nsIContent* aMediator) = 0;

  
  virtual nsIScrollbarMediator* GetScrollbarMediator() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollbarFrame, NS_ISCROLLBARFRAME_IID)

#endif
