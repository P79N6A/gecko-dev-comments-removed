

















































#ifndef _os2framewindow_h
#define _os2framewindow_h





class os2FrameWindow
{
public:
  os2FrameWindow(nsWindow* aOwner);
  ~os2FrameWindow();

  HWND                  CreateFrameWindow(nsWindow* aParent,
                                          HWND aParentWnd,
                                          const nsIntRect& aRect,
                                          nsWindowType aWindowType,
                                          nsBorderStyle aBorderStyle);
  PRUint32              GetFCFlags(nsWindowType aWindowType,
                                   nsBorderStyle aBorderStyle);
  nsresult              Show(PRBool aState);
  void                  SetWindowListVisibility(PRBool aState);
  nsresult              GetBounds(nsIntRect& aRect);
  nsresult              Move(PRInt32 aX, PRInt32 aY);
  nsresult              Resize(PRInt32 aWidth, PRInt32 aHeight,
                               PRBool aRepaint);
  nsresult              Resize(PRInt32 aX, PRInt32 aY, PRInt32 w, PRInt32 h,
                               PRBool aRepaint);
  void                  ActivateTopLevelWidget();
  nsresult              SetSizeMode(PRInt32 aMode);
  nsresult              HideWindowChrome(PRBool aShouldHide);
  nsresult              SetTitle(const nsAString& aTitle); 
  nsresult              SetIcon(const nsAString& aIconSpec); 
  nsresult              ConstrainPosition(PRBool aAllowSlop,
                                          PRInt32* aX, PRInt32* aY);
  MRESULT               ProcessFrameMessage(ULONG msg, MPARAM mp1, MPARAM mp2);
  HWND                  GetFrameWnd()       {return mFrameWnd;}

  friend MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2);

protected:
  nsWindow *    mOwner;             
  HWND          mFrameWnd;          
  HWND          mTitleBar;          
  HWND          mSysMenu;           
  HWND          mMinMax;            
  PRUint32      mSavedStyle;        
  HPOINTER      mFrameIcon;         
  PRBool        mChromeHidden;      
  PRBool        mNeedActivation;    
  PFNWP         mPrevFrameProc;     
  nsIntRect     mFrameBounds;       
};

#endif 



