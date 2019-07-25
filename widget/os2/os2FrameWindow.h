
















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
  uint32_t              GetFCFlags(nsWindowType aWindowType,
                                   nsBorderStyle aBorderStyle);
  nsresult              Show(bool aState);
  void                  SetWindowListVisibility(bool aState);
  nsresult              GetBounds(nsIntRect& aRect);
  nsresult              Move(int32_t aX, int32_t aY);
  nsresult              Resize(int32_t aWidth, int32_t aHeight,
                               bool aRepaint);
  nsresult              Resize(int32_t aX, int32_t aY, int32_t w, int32_t h,
                               bool aRepaint);
  void                  ActivateTopLevelWidget();
  nsresult              SetSizeMode(int32_t aMode);
  nsresult              HideWindowChrome(bool aShouldHide);
  nsresult              SetTitle(const nsAString& aTitle); 
  nsresult              SetIcon(const nsAString& aIconSpec); 
  nsresult              ConstrainPosition(bool aAllowSlop,
                                          int32_t* aX, int32_t* aY);
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
  uint32_t      mSavedStyle;        
  HPOINTER      mFrameIcon;         
  bool          mChromeHidden;      
  bool          mNeedActivation;    
  PFNWP         mPrevFrameProc;     
  nsIntRect     mFrameBounds;       
};

#endif 



