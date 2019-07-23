



































#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "nsRect.h"
#include "nsIWidget.h"
#include "nsIMouseListener.h"
#include "nsIEventListener.h"
#include "nsIMenuListener.h"
#include "nsIToolkit.h"
#include "nsIAppShell.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"










class nsBaseWidget : public nsIWidget
{

public:
  nsBaseWidget();
  virtual ~nsBaseWidget();
  
  NS_DECL_ISUPPORTS
  
  NS_IMETHOD              PreCreateWidget(nsWidgetInitData *aWidgetInitData) { return NS_OK;}
  
  
  NS_IMETHOD              CaptureMouse(PRBool aCapture);
  NS_IMETHOD              Validate();
  NS_IMETHOD              InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);
  NS_IMETHOD              GetClientData(void*& aClientData);
  NS_IMETHOD              SetClientData(void* aClientData);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
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
  NS_IMETHOD              SetWindowType(nsWindowType aWindowType);
  NS_IMETHOD              SetWindowTranslucency(PRBool aTranslucent);
  NS_IMETHOD              GetWindowTranslucency(PRBool& aTranslucent);
  NS_IMETHOD              UpdateTranslucentWindowAlpha(const nsRect& aRect, PRUint8* aAlphas);
  NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);
  NS_IMETHOD              MakeFullScreen(PRBool aFullScreen);
  nsresult                MakeFullScreenInternal(PRBool aFullScreen);
  virtual nsIRenderingContext* GetRenderingContext();
  virtual nsIDeviceContext* GetDeviceContext();
  virtual nsIToolkit*     GetToolkit();  
#ifdef MOZ_CAIRO_GFX
  virtual gfxASurface*    GetThebesSurface();
#endif
  NS_IMETHOD              SetModal(PRBool aModal); 
  NS_IMETHOD              ModalEventFilter(PRBool aRealEvent, void *aEvent,
                            PRBool *aForWindow);
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType);
  NS_IMETHOD              SetBorderStyle(nsBorderStyle aBorderStyle); 
  NS_IMETHOD              AddMouseListener(nsIMouseListener * aListener);
  NS_IMETHOD              AddEventListener(nsIEventListener * aListener);
  NS_IMETHOD              AddMenuListener(nsIMenuListener * aListener);
  NS_IMETHOD              SetBounds(const nsRect &aRect);
  NS_IMETHOD              GetBounds(nsRect &aRect);
  NS_IMETHOD              GetClientBounds(nsRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsRect &aRect);
  NS_IMETHOD              GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD              ScrollRect(nsRect &aRect, PRInt32 aDx, PRInt32 aDy);
  NS_IMETHOD              ScrollWidgets(PRInt32 aDx, PRInt32 aDy);
  NS_IMETHOD              EnableDragDrop(PRBool aEnable);
  NS_IMETHOD              GetAttention(PRInt32 aCycleCount);
  NS_IMETHOD              GetLastInputEventTime(PRUint32& aTime);
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec);
  NS_IMETHOD              SetAnimatedResize(PRUint16 aAnimation);
  NS_IMETHOD              GetAnimatedResize(PRUint16* aAnimation);
  virtual nsIWidget*      GetTopLevelWindow();
  virtual void            ConvertToDeviceCoordinates(nscoord  &aX,nscoord &aY) {}
  virtual void            FreeNativeData(void * data, PRUint32 aDataType) {}

protected:

  virtual void            ResolveIconName(const nsAString &aIconName,
                                          const nsAString &aIconSuffix,
                                          nsILocalFile **aResult);
  virtual void            OnDestroy();
  virtual void            BaseCreate(nsIWidget *aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell,
                                     nsIToolkit *aToolkit,
                                     nsWidgetInitData *aInitData);

protected: 
  void*             mClientData;
  EVENT_CALLBACK    mEventCallback;
  nsIDeviceContext  *mContext;
  nsIToolkit        *mToolkit;
  nsIMouseListener  *mMouseListener;
  nsIEventListener  *mEventListener;
  nsIMenuListener   *mMenuListener;
  nscolor           mBackground;
  nscolor           mForeground;
  nsCursor          mCursor;
  nsWindowType      mWindowType;
  nsBorderStyle     mBorderStyle;
  PRPackedBool      mIsShiftDown;
  PRPackedBool      mIsControlDown;
  PRPackedBool      mIsAltDown;
  PRPackedBool      mIsDestroying;
  PRPackedBool      mOnDestroyCalled;
  nsRect            mBounds;
  nsRect*           mOriginalBounds;
  PRInt32           mZIndex;
  nsSizeMode        mSizeMode;
    
    
    
    
  enum {
    CREATE       = 0x0101,
    CREATE_NATIVE,
    DESTROY, 
    SET_FOCUS,
    SET_CURSOR,
    CREATE_HACK
  };

#ifdef DEBUG
protected:
  static nsAutoString debug_GuiEventToString(nsGUIEvent * aGuiEvent);
  static PRBool debug_WantPaintFlashing();

  static void debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsRect *        aRect,
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

#endif 
