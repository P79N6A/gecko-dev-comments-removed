



































#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "nsRect.h"
#include "nsIWidget.h"
#include "nsIToolkit.h"
#include "nsIAppShell.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"

class nsIContent;
class nsAutoRollup;
class gfxContext;










class nsBaseWidget : public nsIWidget
{
  friend class nsAutoRollup;

public:
  nsBaseWidget();
  virtual ~nsBaseWidget();
  
  NS_DECL_ISUPPORTS
  
  
  NS_IMETHOD              CaptureMouse(PRBool aCapture);
  NS_IMETHOD              GetClientData(void*& aClientData);
  NS_IMETHOD              SetClientData(void* aClientData);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual nsIWidget*      GetTopLevelWidget();
  virtual nsIWidget*      GetSheetWindowParent(void);
  virtual void            AddChild(nsIWidget* aChild);
  virtual void            RemoveChild(nsIWidget* aChild);

  NS_IMETHOD              SetZIndex(PRInt32 aZIndex);
  NS_IMETHOD              GetZIndex(PRInt32* aZIndex);
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                      nsIWidget *aWidget, PRBool aActivate);

  NS_IMETHOD              SetSizeMode(PRInt32 aMode);
  NS_IMETHOD              GetSizeMode(PRInt32* aMode);

  virtual nscolor         GetForegroundColor(void);
  NS_IMETHOD              SetForegroundColor(const nscolor &aColor);
  virtual nscolor         GetBackgroundColor(void);
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  virtual nsCursor        GetCursor();
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    PRUint32 aHotspotX, PRUint32 aHotspotY);
  NS_IMETHOD              GetWindowType(nsWindowType& aWindowType);
  virtual void            SetTransparencyMode(nsTransparencyMode aMode);
  virtual nsTransparencyMode GetTransparencyMode();
  virtual void            GetWindowClipRegion(nsTArray<nsIntRect>* aRects);
  NS_IMETHOD              SetWindowShadowStyle(PRInt32 aStyle);
  virtual void            SetShowsToolbarButton(PRBool aShow) {}
  NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);
  NS_IMETHOD              MakeFullScreen(PRBool aFullScreen);
  virtual nsIDeviceContext* GetDeviceContext();
  virtual nsIToolkit*     GetToolkit();
  virtual LayerManager*   GetLayerManager();
  virtual gfxASurface*    GetThebesSurface();
  NS_IMETHOD              SetModal(PRBool aModal); 
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType);
  NS_IMETHOD              SetBounds(const nsIntRect &aRect);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientOffset(nsIntPoint &aPt);
  NS_IMETHOD              EnableDragDrop(PRBool aEnable);
  NS_IMETHOD              GetAttention(PRInt32 aCycleCount);
  virtual PRBool          HasPendingInputEvent();
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec);
  NS_IMETHOD              BeginSecureKeyboardInput();
  NS_IMETHOD              EndSecureKeyboardInput();
  NS_IMETHOD              SetWindowTitlebarColor(nscolor aColor, PRBool aActive);
  virtual void            SetDrawsInTitlebar(PRBool aState) {}
  virtual PRBool          ShowsResizeIndicator(nsIntRect* aResizerRect);
  virtual void            FreeNativeData(void * data, PRUint32 aDataType) {}
  NS_IMETHOD              BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              ResetInputState() { return NS_OK; }
  NS_IMETHOD              SetIMEOpenState(PRBool aState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              GetIMEOpenState(PRBool* aState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              SetIMEEnabled(PRUint32 aState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              GetIMEEnabled(PRUint32* aState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              CancelIMEComposition() { return NS_OK; }
  NS_IMETHOD              SetAcceleratedRendering(PRBool aEnabled);
  virtual PRBool          GetAcceleratedRendering();
  NS_IMETHOD              GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMEFocusChange(PRBool aFocus) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMESelectionChange(void) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(PRInt32 aOriginalDelta, PRBool aIsHorizontal, PRInt32 &aOverriddenDelta);
  NS_IMETHOD              AttachViewToTopLevel(EVENT_CALLBACK aViewEventFunction, nsIDeviceContext *aContext);
  virtual ViewWrapper*    GetAttachedViewPtr();
  NS_IMETHOD              SetAttachedViewPtr(ViewWrapper* aViewWrapper);
  NS_IMETHOD              ResizeClient(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);

  




  class AutoLayerManagerSetup {
  public:
    AutoLayerManagerSetup(nsBaseWidget* aWidget, gfxContext* aTarget);
    ~AutoLayerManagerSetup();
  private:
    nsBaseWidget* mWidget;
  };
  friend class AutoLayerManagerSetup;

protected:

  virtual void            ResolveIconName(const nsAString &aIconName,
                                          const nsAString &aIconSuffix,
                                          nsILocalFile **aResult);
  virtual void            OnDestroy();
  virtual void            BaseCreate(nsIWidget *aParent,
                                     const nsIntRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell,
                                     nsIToolkit *aToolkit,
                                     nsWidgetInitData *aInitData);

  virtual nsIContent* GetLastRollup()
  {
    return mLastRollup;
  }

  virtual nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                            PRInt32 aNativeKeyCode,
                                            PRUint32 aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters)
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                              PRUint32 aNativeMessage,
                                              PRUint32 aModifierFlags)
  { return NS_ERROR_UNEXPECTED; }

  
  
  PRBool StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects);

protected: 
  void*             mClientData;
  ViewWrapper*      mViewWrapperPtr;
  EVENT_CALLBACK    mEventCallback;
  EVENT_CALLBACK    mViewCallback;
  nsIDeviceContext* mContext;
  nsIToolkit*       mToolkit;
  nsRefPtr<LayerManager> mLayerManager;
  nscolor           mBackground;
  nscolor           mForeground;
  nsCursor          mCursor;
  nsWindowType      mWindowType;
  nsBorderStyle     mBorderStyle;
  PRPackedBool      mOnDestroyCalled;
  PRPackedBool      mUseAcceleratedRendering;
  nsIntRect         mBounds;
  nsIntRect*        mOriginalBounds;
  
  nsAutoArrayPtr<nsIntRect> mClipRects;
  PRUint32          mClipRectCount;
  PRInt32           mZIndex;
  nsSizeMode        mSizeMode;

  
  
  static nsIContent* mLastRollup;
    
#ifdef DEBUG
protected:
  static nsAutoString debug_GuiEventToString(nsGUIEvent * aGuiEvent);
  static PRBool debug_WantPaintFlashing();

  static void debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRect *     aRect,
                                   PRBool                aIsSynchronous,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID);

  static void debug_DumpEvent(FILE *                aFileOut,
                              nsIWidget *           aWidget,
                              nsGUIEvent *          aGuiEvent,
                              const nsCAutoString & aWidgetName,
                              PRInt32               aWindowID);
  
  static void debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   nsPaintEvent *        aPaintEvent,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID);

  static PRBool debug_GetCachedBoolPref(const char* aPrefName);
#endif
};













class nsAutoRollup
{
  PRBool wasClear;

  public:

  nsAutoRollup();
  ~nsAutoRollup();
};















class ScrollRectIterBase {
public:
  PRBool IsDone() { return mHead == nsnull; }
  void operator++() { mHead = mHead->mNext; }
  const nsIntRect& Rect() const { return *mHead; }

protected:
  ScrollRectIterBase() {}

  struct ScrollRect : public nsIntRect {
    ScrollRect(const nsIntRect& aIntRect) : nsIntRect(aIntRect) {}

    
    
    
    void Flip(const nsIntPoint& aDelta)
    {
      if (aDelta.x > 0) x = -XMost();
      if (aDelta.y > 0) y = -YMost();
    }

    ScrollRect* mNext;
  };

  void BaseInit(const nsIntPoint& aDelta, ScrollRect* aHead);

private:
  void Flip(const nsIntPoint& aDelta)
  {
    for (ScrollRect* r = mHead; r; r = r->mNext) {
      r->Flip(aDelta);
    }
  }

  





  class InitialSortComparator {
  public:
    PRBool Equals(const ScrollRect* a, const ScrollRect* b) const
    {
      return a->y == b->y && a->x == b->x;
    }
    PRBool LessThan(const ScrollRect* a, const ScrollRect* b) const
    {
      return a->y < b->y || (a->y == b->y && a->x > b->x);
    }
  };

  void Move(ScrollRect** aUnmovedLink);

  
  ScrollRect* mHead;
  
  ScrollRect** mTailLink;
};

class BlitRectIter : public ScrollRectIterBase {
public:
  BlitRectIter(const nsIntPoint& aDelta, const nsTArray<nsIntRect>& aRects);
private:
  
  BlitRectIter(const BlitRectIter&);
  void operator=(const BlitRectIter&);

  nsTArray<ScrollRect> mRects;
};

#endif 
