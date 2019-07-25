

















































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
  nsresult              Show(bool aState);
  void                  SetWindowListVisibility(bool aState);
  nsresult              GetBounds(nsIntRect& aRect);
  nsresult              Move(PRInt32 aX, PRInt32 aY);
  nsresult              Resize(PRInt32 aWidth, PRInt32 aHeight,
                               bool aRepaint);
  nsresult              Resize(PRInt32 aX, PRInt32 aY, PRInt32 w, PRInt32 h,
                               bool aRepaint);
  void                  ActivateTopLevelWidget();
  nsresult              SetSizeMode(PRInt32 aMode);
  nsresult              HideWindowChrome(bool aShouldHide);
  nsresult              SetTitle(const nsAString& aTitle); 
  nsresult              SetIcon(const nsAString& aIconSpec); 
  nsresult              ConstrainPosition(bool aAllowSlop,
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
  bool          mChromeHidden;      
  bool          mNeedActivation;    
  PFNWP         mPrevFrameProc;     
  nsIntRect     mFrameBounds;       
};

#endif 



