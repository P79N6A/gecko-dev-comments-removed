




































#ifndef nsIWidget_h__
#define nsIWidget_h__

#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"

#include "prthread.h"
#include "nsEvent.h"
#include "nsCOMPtr.h"
#include "nsITheme.h"
#include "nsNativeWidget.h"
#include "nsWidgetInitData.h"
#include "nsTArray.h"


class   nsIAppShell;
class   nsIToolkit;
class   nsIFontMetrics;
class   nsIRenderingContext;
class   nsIDeviceContext;
struct  nsFont;
class   nsIEventListener;
class   nsIRollupListener;
class   nsGUIEvent;
class   imgIContainer;
class   gfxASurface;
class   nsIContent;











typedef nsEventStatus (* EVENT_CALLBACK)(nsGUIEvent *event);





#define NS_NATIVE_WINDOW      0
#define NS_NATIVE_GRAPHIC     1
#define NS_NATIVE_WIDGET      3
#define NS_NATIVE_DISPLAY     4
#define NS_NATIVE_REGION      5
#define NS_NATIVE_OFFSETX     6
#define NS_NATIVE_OFFSETY     7
#define NS_NATIVE_PLUGIN_PORT 8
#define NS_NATIVE_SCREEN      9
#define NS_NATIVE_SHELLWIDGET 10      // Get the shell GtkWidget
#ifdef XP_MACOSX
#define NS_NATIVE_PLUGIN_PORT_QD    100
#define NS_NATIVE_PLUGIN_PORT_CG    101
#endif
#ifdef XP_WIN
#define NS_NATIVE_TSF_THREAD_MGR       100
#define NS_NATIVE_TSF_CATEGORY_MGR     101
#define NS_NATIVE_TSF_DISPLAY_ATTR_MGR 102
#endif

#define NS_IWIDGET_IID \
  { 0xb681539f, 0x5dac, 0x45af, \
    { 0x8a, 0x25, 0xdf, 0xd7, 0x14, 0xe0, 0x9f, 0x43 } }






#define NS_STYLE_WINDOW_SHADOW_NONE             0
#define NS_STYLE_WINDOW_SHADOW_DEFAULT          1





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






class nsIWidget : public nsISupports {

  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWIDGET_IID)

    nsIWidget()
      : mLastChild(nsnull)
      , mPrevSibling(nsnull)
    {}

        
    






























    NS_IMETHOD Create(nsIWidget        *aParent,
                        const nsIntRect  &aRect,
                        EVENT_CALLBACK   aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell      *aAppShell = nsnull,
                        nsIToolkit       *aToolkit = nsnull,
                        nsWidgetInitData *aInitData = nsnull) = 0;

    


















    NS_IMETHOD Create(nsNativeWidget aParent,
                        const nsIntRect  &aRect,
                        EVENT_CALLBACK   aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell      *aAppShell = nsnull,
                        nsIToolkit       *aToolkit = nsnull,
                        nsWidgetInitData *aInitData = nsnull) = 0;


    



    
    NS_IMETHOD  GetClientData(void*& aClientData) = 0;
    NS_IMETHOD  SetClientData(void* aClientData) = 0;
    

    




    NS_IMETHOD Destroy(void) = 0;


    






    NS_IMETHOD SetParent(nsIWidget* aNewParent) = 0;


    






    virtual nsIWidget* GetParent(void) = 0;

    




    virtual nsIWidget* GetTopLevelWidget() = 0;

    







    virtual nsIWidget* GetSheetWindowParent(void) = 0;

    



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

    





    NS_IMETHOD Show(PRBool aState) = 0;

    



    NS_IMETHOD SetModal(PRBool aModal) = 0;

    



    NS_IMETHOD IsVisible(PRBool & aState) = 0;

    














    NS_IMETHOD ConstrainPosition(PRBool aAllowSlop,
                                 PRInt32 *aX,
                                 PRInt32 *aY) = 0;

    






    NS_IMETHOD Move(PRInt32 aX, PRInt32 aY) = 0;

    







    NS_IMETHOD Resize(PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint) = 0;

    









    NS_IMETHOD Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint) = 0;

    


    NS_IMETHOD SetZIndex(PRInt32 aZIndex) = 0;

    


    NS_IMETHOD GetZIndex(PRInt32* aZIndex) = 0;

    










    NS_IMETHOD PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                           nsIWidget *aWidget, PRBool aActivate) = 0;

    



    NS_IMETHOD SetSizeMode(PRInt32 aMode) = 0;

    



    NS_IMETHOD GetSizeMode(PRInt32* aMode) = 0;

    





    NS_IMETHOD Enable(PRBool aState) = 0;

    



    NS_IMETHOD IsEnabled(PRBool *aState) = 0;

    


    NS_IMETHOD SetFocus(PRBool aRaise = PR_FALSE) = 0;

    





    NS_IMETHOD GetBounds(nsIntRect &aRect) = 0;


    









    NS_IMETHOD GetScreenBounds(nsIntRect &aRect) = 0;


    






    NS_IMETHOD GetClientBounds(nsIntRect &aRect) = 0;

    





    virtual nscolor GetForegroundColor(void) = 0;

    






    NS_IMETHOD SetForegroundColor(const nscolor &aColor) = 0;

    






    virtual nscolor GetBackgroundColor(void) = 0;

    






    NS_IMETHOD SetBackgroundColor(const nscolor &aColor) = 0;

    





    virtual nsCursor GetCursor(void) = 0;

    





    NS_IMETHOD SetCursor(nsCursor aCursor) = 0;

    








    NS_IMETHOD SetCursor(imgIContainer* aCursor,
                         PRUint32 aHotspotX, PRUint32 aHotspotY) = 0;

    




    NS_IMETHOD GetWindowType(nsWindowType& aWindowType) = 0;

    
















    virtual void SetTransparencyMode(nsTransparencyMode aMode) = 0;

    



    virtual nsTransparencyMode GetTransparencyMode() = 0;

    



    struct Configuration {
        nsIWidget* mChild;
        nsIntRect mBounds;
        nsTArray<nsIntRect> mClipRegion;
    };

    











    virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations) = 0;

    




    virtual void GetWindowClipRegion(nsTArray<nsIntRect>* aRects) = 0;

    




    NS_IMETHOD SetWindowShadowStyle(PRInt32 aStyle) = 0;

    





    virtual void SetShowsToolbarButton(PRBool aShow) = 0;

    



    NS_IMETHOD HideWindowChrome(PRBool aShouldHide) = 0;

    



    NS_IMETHOD MakeFullScreen(PRBool aFullScreen) = 0;

    



    NS_IMETHOD Validate() = 0;

    






    NS_IMETHOD Invalidate(PRBool aIsSynchronous) = 0;

    






    NS_IMETHOD Invalidate(const nsIntRect & aRect, PRBool aIsSynchronous) = 0;

    





     NS_IMETHOD Update() = 0;

    






    NS_IMETHOD AddEventListener(nsIEventListener * aListener) = 0;

    







    virtual nsIToolkit* GetToolkit() = 0;    

    























    virtual void Scroll(const nsIntPoint& aDelta,
                        const nsTArray<nsIntRect>& aDestRects,
                        const nsTArray<Configuration>& aReconfigureChildren) = 0;

    



    
    virtual void AddChild(nsIWidget* aChild) = 0;
    virtual void RemoveChild(nsIWidget* aChild) = 0;
    virtual void* GetNativeData(PRUint32 aDataType) = 0;
    virtual void FreeNativeData(void * data, PRUint32 aDataType) = 0;
    virtual nsIRenderingContext* GetRenderingContext() = 0;

    
    virtual nsIDeviceContext* GetDeviceContext() = 0;

    

    





    NS_IMETHOD SetBorderStyle(nsBorderStyle aBorderStyle) = 0;

    






    NS_IMETHOD SetTitle(const nsAString& aTitle) = 0;

    








    NS_IMETHOD SetIcon(const nsAString& anIconSpec) = 0;

    





    virtual nsIntPoint WidgetToScreenOffset() = 0;

    



    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus) = 0;

    



    NS_IMETHOD EnableDragDrop(PRBool aEnable) = 0;
   
    




    NS_IMETHOD CaptureMouse(PRBool aCapture) = 0;

    


    NS_IMETHOD SetWindowClass(const nsAString& xulWinType) = 0;

    







    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent) = 0;

    










    NS_IMETHOD GetAttention(PRInt32 aCycleCount) = 0;

    



    virtual PRBool HasPendingInputEvent() = 0;

    






    NS_IMETHOD BeginSecureKeyboardInput() = 0;

    






    NS_IMETHOD EndSecureKeyboardInput() = 0;

    















    NS_IMETHOD SetWindowTitlebarColor(nscolor aColor, PRBool aActive) = 0;
    
    








    virtual PRBool ShowsResizeIndicator(nsIntRect* aResizerRect) = 0;

    


    virtual gfxASurface *GetThebesSurface() = 0;

    


    virtual nsIContent* GetLastRollup() = 0;

    


    NS_IMETHOD BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical) = 0;

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
    






















    virtual nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                              PRInt32 aNativeKeyCode,
                                              PRUint32 aModifierFlags,
                                              const nsAString& aCharacters,
                                              const nsAString& aUnmodifiedCharacters) = 0;

    











    virtual nsresult ActivateNativeMenuItemAt(const nsAString& indexString) = 0;

    















    virtual nsresult ForceUpdateNativeMenuAt(const nsAString& indexString) = 0;

    


    NS_IMETHOD ResetInputState()=0;

    









    




    NS_IMETHOD SetIMEOpenState(PRBool aState) = 0;

    




    NS_IMETHOD GetIMEOpenState(PRBool* aState) = 0;

    








    enum IMEStatus {
      



      IME_STATUS_DISABLED = 0,
      


      IME_STATUS_ENABLED = 1,
      





      IME_STATUS_PASSWORD = 2,
      





      IME_STATUS_PLUGIN = 3
    };

    


    NS_IMETHOD SetIMEEnabled(PRUint32 aState) = 0;

    


    NS_IMETHOD GetIMEEnabled(PRUint32* aState) = 0;

    


    NS_IMETHOD CancelIMEComposition() = 0;

    








    NS_IMETHOD GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState) = 0;

    








    NS_IMETHOD OnIMEFocusChange(PRBool aFocus) = 0;

    





    NS_IMETHOD OnIMETextChange(PRUint32 aStart,
                               PRUint32 aOldEnd,
                               PRUint32 aNewEnd) = 0;

    


    NS_IMETHOD OnIMESelectionChange(void) = 0;

    


 
    NS_IMETHOD OnDefaultButtonLoaded(const nsIntRect &aButtonRect) = 0;

protected:
    
    
    
    
    
    
    nsCOMPtr<nsIWidget> mFirstChild;
    nsIWidget* mLastChild;
    nsCOMPtr<nsIWidget> mNextSibling;
    nsIWidget* mPrevSibling;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWidget, NS_IWIDGET_IID)

#endif 
