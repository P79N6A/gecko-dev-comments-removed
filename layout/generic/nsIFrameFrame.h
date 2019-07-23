









































#ifndef nsIFrameFrame_h___
#define nsIFrameFrame_h___

class nsIDocShell;

#define NS_IFRAMEFRAME_IID \
{ 0x22e34108, 0xc24b, 0x40ea, { \
  0xb9, 0x79, 0x50, 0x18, 0x38, 0x8d, 0xd5, 0x88 } }

class nsIFrameFrame : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMEFRAME_IID)

  NS_IMETHOD GetDocShell(nsIDocShell **aDocShell) = 0;

  




  NS_IMETHOD BeginSwapDocShells(nsIFrame* aOther) = 0;
  virtual void EndSwapDocShells(nsIFrame* aOther) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameFrame, NS_IFRAMEFRAME_IID)

#endif
