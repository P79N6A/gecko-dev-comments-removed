



































#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "nsRect.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsIToolkit.h"
#include "nsIAppShell.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "BasicLayers.h"

class nsIContent;
class nsAutoRollup;
class gfxContext;










class nsBaseWidget : public nsIWidget
{
  friend class nsAutoRollup;

protected:
  typedef mozilla::layers::BasicLayerManager BasicLayerManager;

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
  virtual float           GetDPI();
  virtual double          GetDefaultScale();
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
  virtual nsDeviceContext* GetDeviceContext();
  virtual nsIToolkit*     GetToolkit();
  virtual LayerManager*   GetLayerManager(LayerManagerPersistence aPersistence,
                                          bool* aAllowRetaining = nsnull);
  using nsIWidget::GetLayerManager;

  virtual void            DrawOver(LayerManager* aManager, nsIntRect aRect) {}
  virtual void            UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) {}
  virtual gfxASurface*    GetThebesSurface();
  NS_IMETHOD              SetModal(PRBool aModal); 
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType);
  NS_IMETHOD              SetBounds(const nsIntRect &aRect);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  virtual nsIntPoint      GetClientOffset();
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
  NS_IMETHOD              BeginMoveDrag(nsMouseEvent* aEvent);
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              ResetInputState() { return NS_OK; }
  NS_IMETHOD              SetIMEOpenState(PRBool aState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              GetIMEOpenState(PRBool* aState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              SetInputMode(const IMEContext& aContext) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              GetInputMode(IMEContext& aContext) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              SetIMEEnabled(PRUint32 aState);
  NS_IMETHOD              GetIMEEnabled(PRUint32* aState);
  NS_IMETHOD              CancelIMEComposition() { return NS_OK; }
  NS_IMETHOD              SetAcceleratedRendering(PRBool aEnabled);
  virtual PRBool          GetAcceleratedRendering();
  virtual PRBool          GetShouldAccelerate();
  NS_IMETHOD              GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMEFocusChange(PRBool aFocus) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMESelectionChange(void) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() { return nsIMEUpdatePreference(PR_FALSE, PR_FALSE); }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(PRInt32 aOriginalDelta, PRBool aIsHorizontal, PRInt32 &aOverriddenDelta);
  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              EVENT_CALLBACK   aHandleEventFunction,
              nsDeviceContext *aContext,
              nsIAppShell      *aAppShell = nsnull,
              nsIToolkit       *aToolkit = nsnull,
              nsWidgetInitData *aInitData = nsnull,
              PRBool           aForceUseIWidgetParent = PR_FALSE);
  NS_IMETHOD              AttachViewToTopLevel(EVENT_CALLBACK aViewEventFunction, nsDeviceContext *aContext);
  virtual ViewWrapper*    GetAttachedViewPtr();
  NS_IMETHOD              SetAttachedViewPtr(ViewWrapper* aViewWrapper);
  NS_IMETHOD              ResizeClient(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              RegisterTouchWindow();
  NS_IMETHOD              UnregisterTouchWindow();

  nsPopupLevel PopupLevel() { return mPopupLevel; }

  virtual nsIntSize       ClientToWindowSize(const nsIntSize& aClientSize)
  {
    return aClientSize;
  }

  
  PRBool IsPopupWithTitleBar() const
  {
    return (mWindowType == eWindowType_popup && 
            mBorderStyle != eBorderStyle_default &&
            mBorderStyle & eBorderStyle_title);
  }

  NS_IMETHOD              ReparentNativeWidget(nsIWidget* aNewParent) = 0;
  




  class AutoLayerManagerSetup {
  public:
    AutoLayerManagerSetup(nsBaseWidget* aWidget, gfxContext* aTarget,
                          BasicLayerManager::BufferMode aDoubleBuffering);
    ~AutoLayerManagerSetup();
  private:
    nsBaseWidget* mWidget;
  };
  friend class AutoLayerManagerSetup;

  class AutoUseBasicLayerManager {
  public:
    AutoUseBasicLayerManager(nsBaseWidget* aWidget);
    ~AutoUseBasicLayerManager();
  private:
    nsBaseWidget* mWidget;
  };
  friend class AutoUseBasicLayerManager;

protected:

  virtual void            ResolveIconName(const nsAString &aIconName,
                                          const nsAString &aIconSuffix,
                                          nsILocalFile **aResult);
  virtual void            OnDestroy();
  virtual void            BaseCreate(nsIWidget *aParent,
                                     const nsIntRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsDeviceContext *aContext,
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

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_CHILD_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  BasicLayerManager* CreateBasicLayerManager();

protected: 
  void*             mClientData;
  ViewWrapper*      mViewWrapperPtr;
  EVENT_CALLBACK    mEventCallback;
  EVENT_CALLBACK    mViewCallback;
  nsDeviceContext* mContext;
  nsIToolkit*       mToolkit;
  nsRefPtr<LayerManager> mLayerManager;
  nsRefPtr<LayerManager> mBasicLayerManager;
  nscolor           mBackground;
  nscolor           mForeground;
  nsCursor          mCursor;
  nsWindowType      mWindowType;
  nsBorderStyle     mBorderStyle;
  PRPackedBool      mOnDestroyCalled;
  PRPackedBool      mUseAcceleratedRendering;
  PRPackedBool      mTemporarilyUseBasicLayerManager;
  nsIntRect         mBounds;
  nsIntRect*        mOriginalBounds;
  
  nsAutoArrayPtr<nsIntRect> mClipRects;
  PRUint32          mClipRectCount;
  PRInt32           mZIndex;
  nsSizeMode        mSizeMode;
  nsPopupLevel      mPopupLevel;

  
  
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

#endif 
