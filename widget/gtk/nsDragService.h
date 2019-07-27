





#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsAutoPtr.h"
#include "nsBaseDragService.h"
#include "nsIObserver.h"
#include "nsAutoRef.h"
#include <gtk/gtk.h>

class nsWindow;

namespace mozilla {
namespace gfx {
class SourceSurface;
}
}

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





class nsDragService final : public nsBaseDragService,
                            public nsIObserver
{
public:
    nsDragService();

    NS_DECL_ISUPPORTS_INHERITED

    NS_DECL_NSIOBSERVER

    
    NS_IMETHOD InvokeDragSession (nsIDOMNode *aDOMNode,
                                  nsISupportsArray * anArrayTransferables,
                                  nsIScriptableRegion * aRegion,
                                  uint32_t aActionType) override;
    NS_IMETHOD StartDragSession() override;
    NS_IMETHOD EndDragSession(bool aDoneDrag) override;

    
    NS_IMETHOD SetCanDrop            (bool             aCanDrop) override;
    NS_IMETHOD GetCanDrop            (bool            *aCanDrop) override;
    NS_IMETHOD GetNumDropItems       (uint32_t * aNumItems) override;
    NS_IMETHOD GetData               (nsITransferable * aTransferable,
                                      uint32_t aItemIndex) override;
    NS_IMETHOD IsDataFlavorSupported (const char *aDataFlavor,
                                      bool *_retval) override;

    
    

    static nsDragService* GetInstance();

    void TargetDataReceived          (GtkWidget         *aWidget,
                                      GdkDragContext    *aContext,
                                      gint               aX,
                                      gint               aY,
                                      GtkSelectionData  *aSelection_data,
                                      guint              aInfo,
                                      guint32            aTime);

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
                                 guint32           aTime);

    
    void SetDragIcon(GdkDragContext* aContext);

protected:
    virtual ~nsDragService();

private:

    
    
    
    
    enum DragTask {
        eDragTaskNone,
        eDragTaskMotion,
        eDragTaskLeave,
        eDragTaskDrop,
        eDragTaskSourceEnd
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
    uint32_t        mTargetDragDataLen;
    
    bool           IsTargetContextList(void);
    
    
    void           GetTargetDragData(GdkAtom aFlavor);
    
    void           TargetResetData(void);

    

    
    GtkWidget     *mHiddenWidget;
    
    nsCOMPtr<nsISupportsArray> mSourceDataItems;

    nsCOMPtr<nsIScriptableRegion> mSourceRegion;

    
    GtkTargetList *GetSourceList(void);

    
    
    bool SetAlphaPixmap(SourceSurface *aPixbuf,
                        GdkDragContext  *aContext,
                        int32_t          aXOffset,
                        int32_t          aYOffset,
                        const nsIntRect &dragRect);

    gboolean Schedule(DragTask aTask, nsWindow *aWindow,
                      GdkDragContext *aDragContext,
                      nsIntPoint aWindowPoint, guint aTime);

    
    static gboolean TaskDispatchCallback(gpointer data);
    gboolean RunScheduledTask();
    void UpdateDragAction();
    void DispatchMotionEvents();
    void ReplyToDragMotion();
    gboolean DispatchDropEvent();
};

#endif

