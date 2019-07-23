









































#ifndef nsIFrameFrame_h___
#define nsIFrameFrame_h___

class nsIDocShell;

class nsIFrameFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIFrameFrame)

  NS_IMETHOD GetDocShell(nsIDocShell **aDocShell) = 0;

  




  NS_IMETHOD BeginSwapDocShells(nsIFrame* aOther) = 0;
  virtual void EndSwapDocShells(nsIFrame* aOther) = 0;
};

#endif
