




#ifndef nsIWidget_h__
#define nsIWidget_h__

#include "nsISupports.h"
#include "nsColor.h"
#include "nsRect.h"
#include "nsStringGlue.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWidgetInitData.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsXULAppAPI.h"
#include "mozilla/EventForwards.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "Units.h"


class   nsFontMetrics;
class   nsRenderingContext;
class   nsDeviceContext;
struct  nsFont;
class   nsIRollupListener;
class   imgIContainer;
class   nsIContent;
class   ViewWrapper;
class   nsIWidgetListener;
class   nsIntRegion;

namespace mozilla {
namespace dom {
class TabChild;
}
namespace layers {
class Composer2D;
class CompositorChild;
class LayerManager;
class LayerManagerComposite;
class PLayerTransactionChild;
}
namespace gfx {
class DrawTarget;
}
}











typedef nsEventStatus (* EVENT_CALLBACK)(mozilla::WidgetGUIEvent* aEvent);




typedef void* nsNativeWidget;





#define NS_NATIVE_WINDOW      0
#define NS_NATIVE_GRAPHIC     1
#define NS_NATIVE_TMP_WINDOW  2
#define NS_NATIVE_WIDGET      3
#define NS_NATIVE_DISPLAY     4
#define NS_NATIVE_REGION      5
#define NS_NATIVE_OFFSETX     6
#define NS_NATIVE_OFFSETY     7
#define NS_NATIVE_PLUGIN_PORT 8
#define NS_NATIVE_SCREEN      9

#define NS_NATIVE_SHELLWIDGET 10


#define NS_NATIVE_SHAREABLE_WINDOW 11
#ifdef XP_MACOSX
#define NS_NATIVE_PLUGIN_PORT_QD    100
#define NS_NATIVE_PLUGIN_PORT_CG    101
#endif
#ifdef XP_WIN
#define NS_NATIVE_TSF_THREAD_MGR       100
#define NS_NATIVE_TSF_CATEGORY_MGR     101
#define NS_NATIVE_TSF_DISPLAY_ATTR_MGR 102
#define NS_NATIVE_ICOREWINDOW          103 // winrt specific
#endif

#define NS_IWIDGET_IID \
{ 0x5b27abd6, 0x9e53, 0x4a0a, \
  { 0x86, 0xf, 0x77, 0x5c, 0xc5, 0x69, 0x35, 0xf } };






#define NS_STYLE_WINDOW_SHADOW_NONE             0
#define NS_STYLE_WINDOW_SHADOW_DEFAULT          1
#define NS_STYLE_WINDOW_SHADOW_MENU             2
#define NS_STYLE_WINDOW_SHADOW_TOOLTIP          3
#define NS_STYLE_WINDOW_SHADOW_SHEET            4





enum nsTransparencyMode {
  eTransparencyOpaque = 0,  
  eTransparencyTransparent, 
  eTransparencyGlass,       
  eTransparencyBorderlessGlass 
};





enum nsCursor {   
                eCursor_standard, 
                  
                eCursor_wait, 
                  
                eCursor_select, 
                  
                eCursor_hyperlink, 
                  
                eCursor_n_resize,
                eCursor_s_resize,
                eCursor_w_resize,
                eCursor_e_resize,
                  
                eCursor_nw_resize,
                eCursor_se_resize,
                eCursor_ne_resize,
                eCursor_sw_resize,
                eCursor_crosshair,
                eCursor_move,
                eCursor_help,
                eCursor_copy, 
                eCursor_alias,
                eCursor_context_menu,
                eCursor_cell,
                eCursor_grab,
                eCursor_grabbing,
                eCursor_spinning,
                eCursor_zoom_in,
                eCursor_zoom_out,
                eCursor_not_allowed,
                eCursor_col_resize,
                eCursor_row_resize,
                eCursor_no_drop,
                eCursor_vertical_text,
                eCursor_all_scroll,
                eCursor_nesw_resize,
                eCursor_nwse_resize,
                eCursor_ns_resize,
                eCursor_ew_resize,
                eCursor_none,
                
                eCursorCount
                }; 

enum nsTopLevelWidgetZPlacement { 
  eZPlacementBottom = 0,  
  eZPlacementBelow,       
  eZPlacementTop          
};




#define NS_WIDGET_SLEEP_OBSERVER_TOPIC "sleep_notification"




#define NS_WIDGET_WAKE_OBSERVER_TOPIC "wake_notification"






#define NS_WIDGET_SUSPEND_PROCESS_OBSERVER_TOPIC "suspend_process_notification"





#define NS_WIDGET_RESUME_PROCESS_OBSERVER_TOPIC "resume_process_notification"















struct nsIMEUpdatePreference {

  typedef uint8_t Notifications;

  enum MOZ_ENUM_TYPE(Notifications)
  {
    NOTIFY_NOTHING                       = 0,
    NOTIFY_SELECTION_CHANGE              = 1 << 0,
    NOTIFY_TEXT_CHANGE                   = 1 << 1,
    NOTIFY_POSITION_CHANGE               = 1 << 2,
    
    NOTIFY_CHANGES_CAUSED_BY_COMPOSITION = 1 << 6,
    
    
    NOTIFY_DURING_DEACTIVE               = 1 << 7,
    
    
    
    
    DEFAULT_CONDITIONS_OF_NOTIFYING_CHANGES =
      NOTIFY_CHANGES_CAUSED_BY_COMPOSITION
  };

  nsIMEUpdatePreference()
    : mWantUpdates(DEFAULT_CONDITIONS_OF_NOTIFYING_CHANGES)
  {
  }

  explicit nsIMEUpdatePreference(Notifications aWantUpdates)
    : mWantUpdates(aWantUpdates | DEFAULT_CONDITIONS_OF_NOTIFYING_CHANGES)
  {
  }

  void DontNotifyChangesCausedByComposition()
  {
    mWantUpdates &= ~DEFAULT_CONDITIONS_OF_NOTIFYING_CHANGES;
  }

  bool WantSelectionChange() const
  {
    return !!(mWantUpdates & NOTIFY_SELECTION_CHANGE);
  }

  bool WantTextChange() const
  {
    return !!(mWantUpdates & NOTIFY_TEXT_CHANGE);
  }

  bool WantPositionChanged() const
  {
    return !!(mWantUpdates & NOTIFY_POSITION_CHANGE);
  }

  bool WantChanges() const
  {
    return WantSelectionChange() || WantTextChange();
  }

  bool WantChangesCausedByComposition() const
  {
    return WantChanges() &&
             !!(mWantUpdates & NOTIFY_CHANGES_CAUSED_BY_COMPOSITION);
  }

  bool WantDuringDeactive() const
  {
    return !!(mWantUpdates & NOTIFY_DURING_DEACTIVE);
  }

  Notifications mWantUpdates;
};







namespace mozilla {
namespace widget {

struct IMEState {
  








  enum Enabled {
    



    DISABLED,
    


    ENABLED,
    





    PASSWORD,
    





    PLUGIN
  };
  Enabled mEnabled;

  




  enum Open {
    



    OPEN_STATE_NOT_SUPPORTED,
    



    DONT_CHANGE_OPEN_STATE = OPEN_STATE_NOT_SUPPORTED,
    





    OPEN,
    





    CLOSED
  };
  Open mOpen;

  IMEState() : mEnabled(ENABLED), mOpen(DONT_CHANGE_OPEN_STATE) { }

  explicit IMEState(Enabled aEnabled, Open aOpen = DONT_CHANGE_OPEN_STATE) :
    mEnabled(aEnabled), mOpen(aOpen)
  {
  }
};

struct InputContext {
  InputContext() : mNativeIMEContext(nullptr) {}

  bool IsPasswordEditor() const
  {
    return mHTMLInputType.LowerCaseEqualsLiteral("password");
  }

  IMEState mIMEState;

  
  nsString mHTMLInputType;

  
  nsString mHTMLInputInputmode;

  
  nsString mActionHint;

  


  void* mNativeIMEContext;
};

struct InputContextAction {
  



  enum Cause {
    
    
    CAUSE_UNKNOWN,
    
    
    CAUSE_UNKNOWN_CHROME,
    
    CAUSE_KEY,
    
    CAUSE_MOUSE
  };
  Cause mCause;

  


  enum FocusChange {
    FOCUS_NOT_CHANGED,
    
    GOT_FOCUS,
    
    LOST_FOCUS,
    
    
    MENU_GOT_PSEUDO_FOCUS,
    
    
    MENU_LOST_PSEUDO_FOCUS
  };
  FocusChange mFocusChange;

  bool ContentGotFocusByTrustedCause() const {
    return (mFocusChange == GOT_FOCUS &&
            mCause != CAUSE_UNKNOWN);
  }

  bool UserMightRequestOpenVKB() const {
    return (mFocusChange == FOCUS_NOT_CHANGED &&
            mCause == CAUSE_MOUSE);
  }

  InputContextAction() :
    mCause(CAUSE_UNKNOWN), mFocusChange(FOCUS_NOT_CHANGED)
  {
  }

  explicit InputContextAction(Cause aCause,
                              FocusChange aFocusChange = FOCUS_NOT_CHANGED) :
    mCause(aCause), mFocusChange(aFocusChange)
  {
  }
};





struct SizeConstraints {
  SizeConstraints()
    : mMaxSize(NS_MAXSIZE, NS_MAXSIZE)
  {
  }

  SizeConstraints(nsIntSize aMinSize,
                  nsIntSize aMaxSize)
  : mMinSize(aMinSize),
    mMaxSize(aMaxSize)
  {
  }

  nsIntSize mMinSize;
  nsIntSize mMaxSize;
};




enum IMEMessage MOZ_ENUM_TYPE(int8_t)
{
  
  
  NOTIFY_IME_OF_CURSOR_POS_CHANGED,
  
  NOTIFY_IME_OF_FOCUS,
  
  NOTIFY_IME_OF_BLUR,
  
  NOTIFY_IME_OF_SELECTION_CHANGE,
  
  NOTIFY_IME_OF_TEXT_CHANGE,
  
  NOTIFY_IME_OF_COMPOSITION_UPDATE,
  
  NOTIFY_IME_OF_POSITION_CHANGE,
  
  
  REQUEST_TO_COMMIT_COMPOSITION,
  
  
  REQUEST_TO_CANCEL_COMPOSITION
};

struct IMENotification
{
  MOZ_IMPLICIT IMENotification(IMEMessage aMessage)
    : mMessage(aMessage)
  {
    switch (aMessage) {
      case NOTIFY_IME_OF_SELECTION_CHANGE:
        mSelectionChangeData.mCausedByComposition = false;
        break;
      case NOTIFY_IME_OF_TEXT_CHANGE:
        mTextChangeData.mStartOffset = 0;
        mTextChangeData.mOldEndOffset = 0;
        mTextChangeData.mNewEndOffset = 0;
        mTextChangeData.mCausedByComposition = false;
        break;
      default:
        break;
    }
  }

  IMEMessage mMessage;

  union
  {
    
    struct
    {
      bool mCausedByComposition;
    } mSelectionChangeData;

    
    struct
    {
      uint32_t mStartOffset;
      uint32_t mOldEndOffset;
      uint32_t mNewEndOffset;

      bool mCausedByComposition;

      uint32_t OldLength() const { return mOldEndOffset - mStartOffset; }
      uint32_t NewLength() const { return mNewEndOffset - mStartOffset; }
      int32_t AdditionalLength() const
      {
        return static_cast<int32_t>(mNewEndOffset - mOldEndOffset);
      }
      bool IsInInt32Range() const
      {
        return mStartOffset <= INT32_MAX &&
               mOldEndOffset <= INT32_MAX &&
               mNewEndOffset <= INT32_MAX;
      }
    } mTextChangeData;
  };

  bool IsCausedByComposition() const
  {
    switch (mMessage) {
      case NOTIFY_IME_OF_SELECTION_CHANGE:
        return mSelectionChangeData.mCausedByComposition;
      case NOTIFY_IME_OF_TEXT_CHANGE:
        return mTextChangeData.mCausedByComposition;
      default:
        return false;
    }
  }

private:
  IMENotification();
};

} 
} 





class nsIWidget : public nsISupports {
  protected:
    typedef mozilla::dom::TabChild TabChild;

  public:
    typedef mozilla::layers::Composer2D Composer2D;
    typedef mozilla::layers::CompositorChild CompositorChild;
    typedef mozilla::layers::LayerManager LayerManager;
    typedef mozilla::layers::LayerManagerComposite LayerManagerComposite;
    typedef mozilla::layers::LayersBackend LayersBackend;
    typedef mozilla::layers::PLayerTransactionChild PLayerTransactionChild;
    typedef mozilla::widget::IMEMessage IMEMessage;
    typedef mozilla::widget::IMENotification IMENotification;
    typedef mozilla::widget::IMEState IMEState;
    typedef mozilla::widget::InputContext InputContext;
    typedef mozilla::widget::InputContextAction InputContextAction;
    typedef mozilla::widget::SizeConstraints SizeConstraints;

    
    struct ThemeGeometry {
      
      uint8_t mWidgetType;
      
      nsIntRect mRect;

      ThemeGeometry(uint8_t aWidgetType, const nsIntRect& aRect)
       : mWidgetType(aWidgetType)
       , mRect(aRect)
      { }
    };

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWIDGET_IID)

    nsIWidget()
      : mLastChild(nullptr)
      , mPrevSibling(nullptr)
      , mOnDestroyCalled(false)
      , mWindowType(eWindowType_child)
      , mZIndex(0)

    {
      ClearNativeTouchSequence();
    }

        
    





























    NS_IMETHOD Create(nsIWidget        *aParent,
                      nsNativeWidget   aNativeParent,
                      const nsIntRect  &aRect,
                      nsDeviceContext *aContext,
                      nsWidgetInitData *aInitData = nullptr) = 0;

    















    virtual already_AddRefed<nsIWidget>
    CreateChild(const nsIntRect  &aRect,
                nsDeviceContext  *aContext,
                nsWidgetInitData *aInitData = nullptr,
                bool             aForceUseIWidgetParent = false) = 0;

    














    NS_IMETHOD AttachViewToTopLevel(bool aUseAttachedEvents,
                                    nsDeviceContext *aContext) = 0;

    



    virtual void SetAttachedWidgetListener(nsIWidgetListener* aListener) = 0;
    virtual nsIWidgetListener* GetAttachedWidgetListener() = 0;

    



    
    virtual nsIWidgetListener* GetWidgetListener() = 0;
    virtual void SetWidgetListener(nsIWidgetListener* alistener) = 0;
    

    




    NS_IMETHOD Destroy(void) = 0;

    



    bool Destroyed() const { return mOnDestroyCalled; }


    






    NS_IMETHOD SetParent(nsIWidget* aNewParent) = 0;

    NS_IMETHOD RegisterTouchWindow() = 0;
    NS_IMETHOD UnregisterTouchWindow() = 0;

    






    virtual nsIWidget* GetParent(void) = 0;

    




    virtual nsIWidget* GetTopLevelWidget() = 0;

    







    virtual nsIWidget* GetSheetWindowParent(void) = 0;

    



    virtual float GetDPI() = 0;

    






    mozilla::CSSToLayoutDeviceScale GetDefaultScale();

    










    static double DefaultScaleOverride();

    



    nsIWidget* GetFirstChild() const {
        return mFirstChild;
    }
    
    



    nsIWidget* GetLastChild() const {
        return mLastChild;
    }

    


    nsIWidget* GetNextSibling() const {
        return mNextSibling;
    }
    
    


    void SetNextSibling(nsIWidget* aSibling) {
        mNextSibling = aSibling;
    }
    
    


    nsIWidget* GetPrevSibling() const {
        return mPrevSibling;
    }

    


    void SetPrevSibling(nsIWidget* aSibling) {
        mPrevSibling = aSibling;
    }

    





    NS_IMETHOD Show(bool aState) = 0;

    



    NS_IMETHOD SetModal(bool aModal) = 0;

    





    virtual uint32_t GetMaxTouchPoints() const = 0;

    



    virtual bool IsVisible() const = 0;

    














    NS_IMETHOD ConstrainPosition(bool aAllowSlop,
                                 int32_t *aX,
                                 int32_t *aY) = 0;

    


















    









    NS_IMETHOD Move(double aX, double aY) = 0;

    












    NS_IMETHOD MoveClient(double aX, double aY) = 0;

    








    NS_IMETHOD Resize(double aWidth,
                      double aHeight,
                      bool   aRepaint) = 0;

    










    NS_IMETHOD Resize(double aX,
                      double aY,
                      double aWidth,
                      double aHeight,
                      bool   aRepaint) = 0;

    







    NS_IMETHOD ResizeClient(double aWidth,
                            double aHeight,
                            bool   aRepaint) = 0;

    
















    NS_IMETHOD ResizeClient(double aX,
                            double aY,
                            double aWidth,
                            double aHeight,
                            bool   aRepaint) = 0;

    


    virtual void SetZIndex(int32_t aZIndex) = 0;

    


    int32_t GetZIndex()
    {
      return mZIndex;
    }

    










    NS_IMETHOD PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                           nsIWidget *aWidget, bool aActivate) = 0;

    



    NS_IMETHOD SetSizeMode(int32_t aMode) = 0;

    



    virtual int32_t SizeMode() = 0;

    





    NS_IMETHOD Enable(bool aState) = 0;

    


    virtual bool IsEnabled() const = 0;

    








    NS_IMETHOD SetFocus(bool aRaise = false) = 0;

    







    NS_IMETHOD GetBounds(nsIntRect &aRect) = 0;

    






    NS_IMETHOD GetScreenBounds(nsIntRect &aRect) = 0;

    











    NS_IMETHOD GetRestoredBounds(nsIntRect &aRect) = 0;

    








    NS_IMETHOD GetClientBounds(nsIntRect &aRect) = 0;

    



    NS_IMETHOD GetNonClientMargins(nsIntMargin &margins) = 0;

    











    NS_IMETHOD SetNonClientMargins(nsIntMargin &margins) = 0;

    





    virtual nsIntPoint GetClientOffset() = 0;

    






    virtual void SetBackgroundColor(const nscolor &aColor) { }

    





    virtual nsCursor GetCursor(void) = 0;

    





    NS_IMETHOD SetCursor(nsCursor aCursor) = 0;

    




    virtual void ClearCachedCursor() = 0;

    








    NS_IMETHOD SetCursor(imgIContainer* aCursor,
                         uint32_t aHotspotX, uint32_t aHotspotY) = 0;

    


    nsWindowType WindowType() { return mWindowType; }

    
















    virtual void SetTransparencyMode(nsTransparencyMode aMode) = 0;

    



    virtual nsTransparencyMode GetTransparencyMode() = 0;

    



    struct Configuration {
        nsIWidget* mChild;
        nsIntRect mBounds;
        nsTArray<nsIntRect> mClipRegion;
    };

    















    virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations) = 0;

    




    virtual void GetWindowClipRegion(nsTArray<nsIntRect>* aRects) = 0;

    




    NS_IMETHOD SetWindowShadowStyle(int32_t aStyle) = 0;

    





    virtual void SetShowsToolbarButton(bool aShow) = 0;

    





    virtual void SetShowsFullScreenButton(bool aShow) = 0;

    enum WindowAnimationType {
      eGenericWindowAnimation,
      eDocumentWindowAnimation
    };

    






    virtual void SetWindowAnimationType(WindowAnimationType aType) = 0;

    




    virtual void SetDrawsTitle(bool aDrawTitle) {}

    



    NS_IMETHOD HideWindowChrome(bool aShouldHide) = 0;

    



    NS_IMETHOD MakeFullScreen(bool aFullScreen) = 0;

    



    NS_IMETHOD Invalidate(const nsIntRect & aRect) = 0;

    enum LayerManagerPersistence
    {
      LAYER_MANAGER_CURRENT = 0,
      LAYER_MANAGER_PERSISTENT
    };

    






    inline LayerManager* GetLayerManager(bool* aAllowRetaining = nullptr)
    {
        return GetLayerManager(nullptr, mozilla::layers::LayersBackend::LAYERS_NONE,
                               LAYER_MANAGER_CURRENT, aAllowRetaining);
    }

    inline LayerManager* GetLayerManager(LayerManagerPersistence aPersistence,
                                         bool* aAllowRetaining = nullptr)
    {
        return GetLayerManager(nullptr, mozilla::layers::LayersBackend::LAYERS_NONE,
                               aPersistence, aAllowRetaining);
    }

    




    virtual LayerManager* GetLayerManager(PLayerTransactionChild* aShadowManager,
                                          LayersBackend aBackendHint,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr) = 0;

    





    virtual void PrepareWindowEffects() = 0;

    





    virtual void CleanupWindowEffects() = 0;

    






    virtual bool PreRender(LayerManagerComposite* aManager) = 0;

    






    virtual void PostRender(LayerManagerComposite* aManager) = 0;

    




    virtual void DrawWindowUnderlay(LayerManagerComposite* aManager, nsIntRect aRect) = 0;

    




    virtual void DrawWindowOverlay(LayerManagerComposite* aManager, nsIntRect aRect) = 0;

    





    virtual mozilla::TemporaryRef<mozilla::gfx::DrawTarget> StartRemoteDrawing() = 0;

    






    virtual void EndRemoteDrawing() = 0;

    





    virtual void CleanupRemoteDrawing() = 0;

    











    virtual void UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) = 0;

    




    virtual void UpdateOpaqueRegion(const nsIntRegion &aOpaqueRegion) {}

    



    
    virtual void AddChild(nsIWidget* aChild) = 0;
    virtual void RemoveChild(nsIWidget* aChild) = 0;
    virtual void* GetNativeData(uint32_t aDataType) = 0;
    virtual void FreeNativeData(void * data, uint32_t aDataType) = 0;

    
    virtual nsDeviceContext* GetDeviceContext() = 0;

    

    






    NS_IMETHOD SetTitle(const nsAString& aTitle) = 0;

    








    NS_IMETHOD SetIcon(const nsAString& anIconSpec) = 0;

    





    virtual nsIntPoint WidgetToScreenOffset() = 0;

    




    virtual nsIntSize ClientToWindowSize(const nsIntSize& aClientSize) = 0;

    



    NS_IMETHOD DispatchEvent(mozilla::WidgetGUIEvent* event,
                             nsEventStatus & aStatus) = 0;

    



    NS_IMETHOD EnableDragDrop(bool aEnable) = 0;
   
    




    NS_IMETHOD CaptureMouse(bool aCapture) = 0;

    


    NS_IMETHOD SetWindowClass(const nsAString& xulWinType) = 0;

    






    NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener, bool aDoCapture) = 0;

    










    NS_IMETHOD GetAttention(int32_t aCycleCount) = 0;

    



    virtual bool HasPendingInputEvent() = 0;

    















    NS_IMETHOD SetWindowTitlebarColor(nscolor aColor, bool aActive) = 0;

    










    virtual void SetDrawsInTitlebar(bool aState) = 0;

    








    virtual bool ShowsResizeIndicator(nsIntRect* aResizerRect) = 0;

    


    virtual nsIContent* GetLastRollup() = 0;

    


    NS_IMETHOD BeginResizeDrag(mozilla::WidgetGUIEvent* aEvent,
                               int32_t aHorizontal,
                               int32_t aVertical) = 0;

    


    NS_IMETHOD BeginMoveDrag(mozilla::WidgetMouseEvent* aEvent) = 0;

    enum Modifiers {
        CAPS_LOCK = 0x01, 
        NUM_LOCK = 0x02, 
        SHIFT_L = 0x0100,
        SHIFT_R = 0x0200,
        CTRL_L = 0x0400,
        CTRL_R = 0x0800,
        ALT_L = 0x1000, 
        ALT_R = 0x2000,
        COMMAND_L = 0x4000,
        COMMAND_R = 0x8000,
        HELP = 0x10000,
        FUNCTION = 0x100000,
        NUMERIC_KEY_PAD = 0x01000000 
    };
    






















    virtual nsresult SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                              int32_t aNativeKeyCode,
                                              uint32_t aModifierFlags,
                                              const nsAString& aCharacters,
                                              const nsAString& aUnmodifiedCharacters) = 0;

    













    virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                uint32_t aNativeMessage,
                                                uint32_t aModifierFlags) = 0;

    



    virtual nsresult SynthesizeNativeMouseMove(nsIntPoint aPoint) = 0;

    




















    virtual nsresult SynthesizeNativeMouseScrollEvent(nsIntPoint aPoint,
                                                      uint32_t aNativeMessage,
                                                      double aDeltaX,
                                                      double aDeltaY,
                                                      double aDeltaZ,
                                                      uint32_t aModifierFlags,
                                                      uint32_t aAdditionalFlags) = 0;

    



    enum TouchPointerState {
      
      TOUCH_HOVER    = 0x01,
      
      TOUCH_CONTACT  = 0x02,
      
      TOUCH_REMOVE   = 0x04,
      
      
      
      
      TOUCH_CANCEL   = 0x08
    };

    












    virtual nsresult SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                                TouchPointerState aPointerState,
                                                nsIntPoint aPointerScreenPoint,
                                                double aPointerPressure,
                                                uint32_t aPointerOrientation) = 0;

    




    virtual nsresult ClearNativeTouchSequence();

    





    nsresult SynthesizeNativeTouchTap(nsIntPoint aPointerScreenPoint,
                                      bool aLongTap);

private:
  class LongTapInfo
  {
  public:
    LongTapInfo(int32_t aPointerId, nsIntPoint& aPoint,
                mozilla::TimeDuration aDuration) :
      mPointerId(aPointerId),
      mPosition(aPoint),
      mDuration(aDuration),
      mStamp(mozilla::TimeStamp::Now())
    {
    }

    int32_t mPointerId;
    nsIntPoint mPosition;
    mozilla::TimeDuration mDuration;
    mozilla::TimeStamp mStamp;
  };

  static void OnLongTapTimerCallback(nsITimer* aTimer, void* aClosure);

  nsAutoPtr<LongTapInfo> mLongTapTouchPoint;
  nsCOMPtr<nsITimer> mLongTapTimer;
  static int32_t sPointerIdCounter;

public:
    











    virtual nsresult ActivateNativeMenuItemAt(const nsAString& indexString) = 0;

    















    virtual nsresult ForceUpdateNativeMenuAt(const nsAString& indexString) = 0;

    


    NS_IMETHOD NotifyIME(const IMENotification& aIMENotification) = 0;

    


    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction) = 0;

    


    NS_IMETHOD_(InputContext) GetInputContext() = 0;

    





    NS_IMETHOD AttachNativeKeyEvent(mozilla::WidgetKeyboardEvent& aEvent) = 0;

    


    typedef void (*DoCommandCallback)(mozilla::Command, void*);
    enum NativeKeyBindingsType
    {
      NativeKeyBindingsForSingleLineEditor,
      NativeKeyBindingsForMultiLineEditor,
      NativeKeyBindingsForRichTextEditor
    };
    NS_IMETHOD_(bool) ExecuteNativeKeyBinding(
                        NativeKeyBindingsType aType,
                        const mozilla::WidgetKeyboardEvent& aEvent,
                        DoCommandCallback aCallback,
                        void* aCallbackData) = 0;

    


    NS_IMETHOD SetLayersAcceleration(bool aEnabled) = 0;

    








    NS_IMETHOD GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState) = 0;

    


    virtual nsIMEUpdatePreference GetIMEUpdatePreference() = 0;

    


 
    NS_IMETHOD OnDefaultButtonLoaded(const nsIntRect &aButtonRect) = 0;

    




















    NS_IMETHOD OverrideSystemMouseScrollSpeed(double aOriginalDeltaX,
                                              double aOriginalDeltaY,
                                              double& aOverriddenDeltaX,
                                              double& aOverriddenDeltaY) = 0;

    





    static bool
    UsePuppetWidgets()
    {
      return XRE_GetProcessType() == GeckoProcessType_Content;
    }

    









    static already_AddRefed<nsIWidget>
    CreatePuppetWidget(TabChild* aTabChild);

    




    NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent) = 0;

    



    virtual uint32_t GetGLFrameBufferFormat() { return 0;  }

    


    virtual bool HasGLContext() { return false; }

    




    virtual bool WidgetPaintsBackground() { return false; }

    virtual bool NeedsPaint() {
       if (!IsVisible()) {
           return false;
       }
       nsIntRect bounds;
       nsresult rv = GetBounds(bounds);
       NS_ENSURE_SUCCESS(rv, false);
       return !bounds.IsEmpty();
    }

    











    virtual nsIntRect GetNaturalBounds() {
        nsIntRect bounds;
        GetBounds(bounds);
        return bounds;
    }

    










    virtual void SetSizeConstraints(const SizeConstraints& aConstraints) = 0;

    




    virtual const SizeConstraints& GetSizeConstraints() const = 0;

    



    virtual TabChild* GetOwningTabChild() { return nullptr; }

    



    virtual CompositorChild* GetRemoteRenderer()
    { return nullptr; }

    






    virtual Composer2D* GetComposer2D()
    { return nullptr; }

    




    virtual int32_t RoundsWidgetCoordinatesTo() { return 1; }

protected:
    



    virtual double GetDefaultScaleInternal() { return 1.0; }

    
    
    
    
    
    
    nsCOMPtr<nsIWidget> mFirstChild;
    nsIWidget* mLastChild;
    nsCOMPtr<nsIWidget> mNextSibling;
    nsIWidget* mPrevSibling;
    
    bool mOnDestroyCalled;
    nsWindowType mWindowType;
    int32_t mZIndex;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWidget, NS_IWIDGET_IID)

#endif 
