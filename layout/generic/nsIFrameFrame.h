









































#ifndef nsIFrameFrame_h___
#define nsIFrameFrame_h___

class nsIDocShell;

#define NS_IFRAMEFRAME_IID \
{ 0xda876f25, 0x1cff, 0x4f0a, { \
  0xbf, 0x7e, 0x83, 0xd7, 0x4f, 0xc5, 0x2a, 0x3b } }

class nsIFrameFrame : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMEFRAME_IID)

  NS_IMETHOD GetDocShell(nsIDocShell **aDocShell) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameFrame, NS_IFRAMEFRAME_IID)

#endif
