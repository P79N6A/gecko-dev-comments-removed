




































#ifndef nsIWidget_h__
#define nsIWidget_h__

#include "nsISupports.h"
#include "nsColor.h"
#include "nsIMouseListener.h"
#include "nsIMenuListener.h"
#include "nsCoord.h"

#include "prthread.h"
#include "nsEvent.h"
#include "nsCOMPtr.h"


class   nsIAppShell;
class   nsIToolkit;
class   nsIFontMetrics;
class   nsIRenderingContext;
class   nsIDeviceContext;
class   nsIRegion;
struct  nsRect;
struct  nsFont;
class   nsIMenuBar;
class   nsIEventListener;
class   nsIRollupListener;
class   nsGUIEvent;
struct  nsColorMap;
class   imgIContainer;

#ifdef MOZ_CAIRO_GFX
class   gfxASurface;
#endif









typedef nsEventStatus (*PR_CALLBACK EVENT_CALLBACK)(nsGUIEvent *event);





#define NS_NATIVE_WINDOW      0
#define NS_NATIVE_GRAPHIC     1
#define NS_NATIVE_COLORMAP    2
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


#define NS_IWIDGET_IID \
{ 0xB3F10C8D, 0x4C07, 0x4B1E, \
  { 0xA1, 0xCD, 0xB3, 0x86, 0x96, 0x42, 0x62, 0x05 } }





typedef void* nsNativeWidget;





enum nsWindowType {     
  
  eWindowType_toplevel,
  
  eWindowType_dialog,
  
  eWindowType_popup,
  
  eWindowType_child,
  
  eWindowType_invisible,
  
  eWindowType_plugin,
  
  eWindowType_java,
  
  eWindowType_sheet
};

enum nsBorderStyle
{
  
  eBorderStyle_none     = 0,

  
  eBorderStyle_all      = 1 << 0,

  
  eBorderStyle_border   = 1 << 1,

  
  eBorderStyle_resizeh  = 1 << 2,

  
  eBorderStyle_title    = 1 << 3,

  
  eBorderStyle_menu     = 1 << 4,

  
  
  eBorderStyle_minimize = 1 << 5,

  
  eBorderStyle_maximize = 1 << 6,

  
  eBorderStyle_close    = 1 << 7,

  
  eBorderStyle_default  = -1
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
                
                eCursorCount
                }; 

enum nsContentType {
  eContentTypeInherit = -1,
  eContentTypeUI = 0,
  eContentTypeContent = 1,
  eContentTypeContentFrame = 2
};

enum nsTopLevelWidgetZPlacement { 
  eZPlacementBottom = 0,  
  eZPlacementBelow,       
  eZPlacementTop          
};






struct nsWidgetInitData {
  nsWidgetInitData()
    : clipChildren(PR_FALSE), 
      clipSiblings(PR_FALSE), 
      mDropShadow(PR_FALSE),
      mListenForResizes(PR_FALSE),
      mWindowType(eWindowType_child),
      mBorderStyle(eBorderStyle_default),
      mContentType(eContentTypeInherit),
      mUnicode(PR_TRUE)
  {
  }

  
  PRPackedBool  clipChildren, clipSiblings, mDropShadow;
  PRPackedBool  mListenForResizes;
  nsWindowType mWindowType;
  nsBorderStyle mBorderStyle;
  nsContentType mContentType;  
  PRPackedBool mUnicode;
};





class nsIWidget : public nsISupports {

  public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWIDGET_IID)

    nsIWidget()
      : mLastChild(nsnull)
      , mPrevSibling(nsnull)
    {}

        
    






























    NS_IMETHOD Create(nsIWidget        *aParent,
                        const nsRect     &aRect,
                        EVENT_CALLBACK   aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell      *aAppShell = nsnull,
                        nsIToolkit       *aToolkit = nsnull,
                        nsWidgetInitData *aInitData = nsnull) = 0;

    


















    NS_IMETHOD Create(nsNativeWidget aParent,
                        const nsRect     &aRect,
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

    





    NS_IMETHOD GetBounds(nsRect &aRect) = 0;


    








    NS_IMETHOD GetScreenBounds(nsRect &aRect) = 0;


    






    NS_IMETHOD GetClientBounds(nsRect &aRect) = 0;

    





    NS_IMETHOD GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight) = 0;

    





    virtual nscolor GetForegroundColor(void) = 0;

    






    NS_IMETHOD SetForegroundColor(const nscolor &aColor) = 0;

    






    virtual nscolor GetBackgroundColor(void) = 0;

    






    NS_IMETHOD SetBackgroundColor(const nscolor &aColor) = 0;

    





    virtual nsIFontMetrics* GetFont(void) = 0;

    





    NS_IMETHOD SetFont(const nsFont &aFont) = 0;

    





    virtual nsCursor GetCursor(void) = 0;

    





    NS_IMETHOD SetCursor(nsCursor aCursor) = 0;

    








    NS_IMETHOD SetCursor(imgIContainer* aCursor,
                         PRUint32 aHotspotX, PRUint32 aHotspotY) = 0;

    




    NS_IMETHOD GetWindowType(nsWindowType& aWindowType) = 0;

    


















    NS_IMETHOD SetWindowTranslucency(PRBool aTranslucent) = 0;

    





    NS_IMETHOD GetWindowTranslucency(PRBool& aTranslucent) = 0;

    








    NS_IMETHOD UpdateTranslucentWindowAlpha(const nsRect& aRect, PRUint8* aAlphas) = 0;

    



    NS_IMETHOD HideWindowChrome(PRBool aShouldHide) = 0;

    



    NS_IMETHOD MakeFullScreen(PRBool aFullScreen) = 0;

    



    NS_IMETHOD Validate() = 0;

    






    NS_IMETHOD Invalidate(PRBool aIsSynchronous) = 0;

    






    NS_IMETHOD Invalidate(const nsRect & aRect, PRBool aIsSynchronous) = 0;

    






    NS_IMETHOD InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous) = 0;

    





     NS_IMETHOD Update() = 0;

    






    NS_IMETHOD AddMouseListener(nsIMouseListener * aListener) = 0;

    






    NS_IMETHOD AddEventListener(nsIEventListener * aListener) = 0;

    






    NS_IMETHOD AddMenuListener(nsIMenuListener * aListener) = 0;
    
    





    virtual nsIToolkit* GetToolkit() = 0;    

    






    NS_IMETHOD SetColorMap(nsColorMap *aColorMap) = 0;

    









    NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect) = 0;

    









    NS_IMETHOD ScrollWidgets(PRInt32 aDx, PRInt32 aDy) = 0;

    









    NS_IMETHOD ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy) = 0;

    



    
    virtual void AddChild(nsIWidget* aChild) = 0;
    virtual void RemoveChild(nsIWidget* aChild) = 0;
    virtual void* GetNativeData(PRUint32 aDataType) = 0;
    virtual void FreeNativeData(void * data, PRUint32 aDataType) = 0;
    virtual nsIRenderingContext* GetRenderingContext() = 0;

    
    virtual nsIDeviceContext* GetDeviceContext() = 0;

    

    





    NS_IMETHOD SetBorderStyle(nsBorderStyle aBorderStyle) = 0;

    






    NS_IMETHOD SetTitle(const nsAString& aTitle) = 0;

    








    NS_IMETHOD SetIcon(const nsAString& anIconSpec) = 0;

    






    NS_IMETHOD SetMenuBar(nsIMenuBar * aMenuBar) = 0;

    





    NS_IMETHOD ShowMenuBar(PRBool aShow) = 0;

    






    NS_IMETHOD WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect) = 0;

    






    NS_IMETHOD ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect) = 0;

    







    NS_IMETHOD BeginResizingChildren(void) = 0;

    





    NS_IMETHOD EndResizingChildren(void) = 0;

    



    NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight) = 0;

    



    NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight) = 0;

    



    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus) = 0;

    



    NS_IMETHOD EnableDragDrop(PRBool aEnable) = 0;
   
    virtual void  ConvertToDeviceCoordinates(nscoord &aX,nscoord &aY) = 0;

    




    NS_IMETHOD CaptureMouse(PRBool aCapture) = 0;

    


    NS_IMETHOD SetWindowClass(const nsAString& xulWinType) = 0;

    







    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent) = 0;

    










    NS_IMETHOD ModalEventFilter(PRBool aRealEvent, void *aEvent, PRBool *aForWindow) = 0;

    










    NS_IMETHOD GetAttention(PRInt32 aCycleCount) = 0;

    









    NS_IMETHOD GetLastInputEventTime(PRUint32& aTime) = 0;

#ifdef MOZ_CAIRO_GFX
    


    virtual gfxASurface *GetThebesSurface() = 0;
#endif

    






    NS_IMETHOD SetAnimatedResize(PRUint16 aAnimation) = 0;

    






    NS_IMETHOD GetAnimatedResize(PRUint16* aAnimation) = 0;

    



    virtual nsIWidget* GetTopLevelWindow(void) = 0;

protected:
    
    
    
    
    
    
    nsCOMPtr<nsIWidget> mFirstChild;
    nsIWidget* mLastChild;
    nsCOMPtr<nsIWidget> mNextSibling;
    nsIWidget* mPrevSibling;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWidget, NS_IWIDGET_IID)

#endif 
