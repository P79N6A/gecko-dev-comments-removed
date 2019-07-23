









































#ifndef nsIFrameFrame_h___
#define nsIFrameFrame_h___

class nsIDocShell;

class nsIFrameFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIFrameFrame)

  NS_IMETHOD GetDocShell(nsIDocShell **aDocShell) = 0;

  




  NS_IMETHOD BeginSwapDocShells(nsIFrame* aOther) = 0;
  virtual void EndSwapDocShells(nsIFrame* aOther) = 0;
};

#endif
