







































#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"
#include "nsIDragSessionGTK.h"
#include "nsIObserver.h"
#include <gtk/gtk.h>

class nsWindow;

#ifndef HAVE_NSGOBJECTREFTRAITS
#define HAVE_NSGOBJECTREFTRAITS
template <class T>
class nsGObjectRefTraits : public nsPointerRefTraits<T> {
public:
    static void Release(T *aPtr) { g_object_unref(aPtr); }
    static void AddRef(T *aPtr) { g_object_ref(aPtr); }
};
#endif

#ifndef HAVE_NSAUTOREFTRAITS_GTKWIDGET
#define HAVE_NSAUTOREFTRAITS_GTKWIDGET
template <>
class nsAutoRefTraits<GtkWidget> : public nsGObjectRefTraits<GtkWidget> { };
#endif

#ifndef HAVE_NSAUTOREFTRAITS_GDKDRAGCONTEXT
#define HAVE_NSAUTOREFTRAITS_GDKDRAGCONTEXT
template <>
class nsAutoRefTraits<GdkDragContext> :
    public nsGObjectRefTraits<GdkDragContext> { };
#endif





class nsDragService : public nsBaseDragService,
                      public nsIDragSessionGTK,
                      public nsIObserver
{
public:
    nsDragService();
    virtual ~nsDragService();

    NS_DECL_ISUPPORTS_INHERITED

    NS_DECL_NSIOBSERVER

    
    NS_IMETHOD InvokeDragSession (nsIDOMNode *aDOMNode,
                                  nsISupportsArray * anArrayTransferables,
                                  nsIScriptableRegion * aRegion,
                                  PRUint32 aActionType);
    NS_IMETHOD StartDragSession();
    NS_IMETHOD EndDragSession(bool aDoneDrag);

    
    NS_IMETHOD SetCanDrop            (bool             aCanDrop);
    NS_IMETHOD GetCanDrop            (bool            *aCanDrop);
    NS_IMETHOD GetNumDropItems       (PRUint32 * aNumItems);
    NS_IMETHOD GetData               (nsITransferable * aTransferable,
                                      PRUint32 aItemIndex);
    NS_IMETHOD IsDataFlavorSupported (const char *aDataFlavor, bool *_retval);

    

    NS_IMETHOD TargetSetLastContext  (GtkWidget      *aWidget,
                                      GdkDragContext *aContext,
                                      guint           aTime);
    NS_IMETHOD TargetStartDragMotion (void);
    NS_IMETHOD TargetEndDragMotion   (GtkWidget      *aWidget,
                                      GdkDragContext *aContext,
                                      guint           aTime);
    NS_IMETHOD TargetDataReceived    (GtkWidget         *aWidget,
                                      GdkDragContext    *aContext,
                                      gint               aX,
                                      gint               aY,
                                      GtkSelectionData  *aSelection_data,
                                      guint              aInfo,
                                      guint32            aTime);

    NS_IMETHOD TargetSetTimeCallback (nsIDragSessionGTKTimeCB aCallback);

    static nsDragService* GetInstance();

    
    

    gboolean ScheduleMotionEvent(nsWindow *aWindow,
                                 GdkDragContext *aDragContext,
                                 nsIntPoint aWindowPoint,
                                 guint aTime);
    void ScheduleLeaveEvent();
    gboolean ScheduleDropEvent(nsWindow *aWindow,
                               GdkDragContext *aDragContext,
                               nsIntPoint aWindowPoint,
                               guint aTime);

    nsWindow* GetMostRecentDestWindow()
    {
        return mScheduledTask == eDragTaskNone ? mTargetWindow
            : mPendingWindow;
    }

    

    
    
    
    void           SourceEndDragSession(GdkDragContext *aContext,
                                        gint            aResult);
    void           SourceDataGet(GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 GtkSelectionData *selection_data,
                                 guint             info,
                                 guint32           aTime);

    
    void SetDragIcon(GdkDragContext* aContext);

private:

    
    
    
    
    enum DragTask {
        eDragTaskNone,
        eDragTaskMotion,
        eDragTaskLeave,
        eDragTaskDrop
    };
    DragTask mScheduledTask;
    
    
    guint mTaskSource;

    
    

    
    
    
    
    nsRefPtr<nsWindow> mPendingWindow;
    nsIntPoint mPendingWindowPoint;
    nsCountedRef<GdkDragContext> mPendingDragContext;
    guint mPendingTime;

    
    
    
    nsRefPtr<nsWindow> mTargetWindow;
    nsIntPoint mTargetWindowPoint;
    
    
    nsCountedRef<GtkWidget> mTargetWidget;
    nsCountedRef<GdkDragContext> mTargetDragContext;
    guint           mTargetTime;

    
    bool            mCanDrop;

    
    bool            mTargetDragDataReceived;
    
    void           *mTargetDragData;
    PRUint32        mTargetDragDataLen;
    
    bool           IsTargetContextList(void);
    
    
    void           GetTargetDragData(GdkAtom aFlavor);
    
    void           TargetResetData(void);

    

    
    GtkWidget     *mHiddenWidget;
    
    GtkWidget     *mGrabWidget;
    
    nsCOMPtr<nsISupportsArray> mSourceDataItems;

    nsCOMPtr<nsIScriptableRegion> mSourceRegion;

    
    GtkTargetList *GetSourceList(void);

    
    
    bool SetAlphaPixmap(gfxASurface     *aPixbuf,
                          GdkDragContext  *aContext,
                          PRInt32          aXOffset,
                          PRInt32          aYOffset,
                          const nsIntRect &dragRect);

    gboolean Schedule(DragTask aTask, nsWindow *aWindow,
                      GdkDragContext *aDragContext,
                      nsIntPoint aWindowPoint, guint aTime);

    
    static gboolean TaskDispatchCallback(gpointer data);
    gboolean RunScheduledTask();
};

#endif

