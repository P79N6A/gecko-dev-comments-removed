



























































#ifndef _nsframewindow_h
#define _nsframewindow_h

#include "nsWindow.h"
#include "nssize.h"





class nsFrameWindow : public nsWindow
{
public:
  nsFrameWindow();
  virtual ~nsFrameWindow();

  
  virtual nsresult      CreateWindow(nsWindow* aParent,
                                     HWND aParentWnd,
                                     const nsIntRect& aRect,
                                     PRUint32 aStyle);
  NS_IMETHOD            Show(PRBool aState);
  NS_IMETHOD            GetClientBounds(nsIntRect& aRect);

protected:
  
  virtual HWND          GetMainWindow()     const {return mFrameWnd;}
  virtual void          ActivateTopLevelWidget();
  virtual PRBool        OnReposition(PSWP pSwp);
  virtual PRInt32       GetClientHeight()   {return mSizeClient.height;}

  
  PRUint32              GetFCFlags();
  void                  UpdateClientSize();
  void                  SetWindowListVisibility(PRBool aState);
  MRESULT               FrameMessage(ULONG msg, MPARAM mp1, MPARAM mp2);

  friend MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2);

  PFNWP         mPrevFrameProc;
  nsSize        mSizeClient;
  PRBool        mNeedActivation;
};

#endif 



