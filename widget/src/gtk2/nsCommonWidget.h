





































#ifndef __nsCommonWidget_h__
#define __nsCommonWidget_h__

#include "nsBaseWidget.h"
#include "nsGUIEvent.h"
#include <gdk/gdkevents.h>

#ifdef MOZ_LOGGING


#define FORCE_PR_LOG

#include "prlog.h"

extern PRLogModuleInfo *gWidgetLog;
extern PRLogModuleInfo *gWidgetFocusLog;
extern PRLogModuleInfo *gWidgetIMLog;
extern PRLogModuleInfo *gWidgetDrawLog;

#define LOG(args) PR_LOG(gWidgetLog, 4, args)
#define LOGFOCUS(args) PR_LOG(gWidgetFocusLog, 4, args)
#define LOGIM(args) PR_LOG(gWidgetIMLog, 4, args)
#define LOGDRAW(args) PR_LOG(gWidgetDrawLog, 4, args)

#else

#define LOG(args)
#define LOGFOCUS(args)
#define LOGIM(args)
#define LOGDRAW(args)

#endif 

class nsCommonWidget : public nsBaseWidget {
public:
    nsCommonWidget();
    virtual ~nsCommonWidget();

    virtual nsIWidget *GetParent(void);

    void CommonCreate(nsIWidget *aParent, PRBool aListenForResizes);

    
    void InitButtonEvent(nsMouseEvent &aEvent,
                         GdkEventButton *aGdkEvent);
    void InitMouseScrollEvent(nsMouseScrollEvent &aEvent,
                              GdkEventScroll *aGdkEvent);
    void InitKeyEvent(nsKeyEvent &aEvent, GdkEventKey *aGdkEvent);

    void DispatchGotFocusEvent(void);
    void DispatchLostFocusEvent(void);
    void DispatchActivateEvent(void);
    void DispatchDeactivateEvent(void);
    void DispatchResizeEvent(nsRect &aRect, nsEventStatus &aStatus);

    NS_IMETHOD DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus);

    
    virtual void NativeResize(PRInt32 aWidth,
                              PRInt32 aHeight,
                              PRBool  aRepaint) = 0;

    virtual void NativeResize(PRInt32 aX,
                              PRInt32 aY,
                              PRInt32 aWidth,
                              PRInt32 aHeight,
                              PRBool  aRepaint) = 0;

    virtual void NativeShow  (PRBool  aAction) = 0;

    
    NS_IMETHOD         Show             (PRBool aState);
    NS_IMETHOD         Resize           (PRInt32 aWidth,
                                         PRInt32 aHeight,
                                         PRBool  aRepaint);
    NS_IMETHOD         Resize           (PRInt32 aX,
                                         PRInt32 aY,
                                         PRInt32 aWidth,
                                         PRInt32 aHeight,
                                         PRBool   aRepaint);
    NS_IMETHOD         GetPreferredSize (PRInt32 &aWidth,
                                         PRInt32 &aHeight);
    NS_IMETHOD         SetPreferredSize (PRInt32 aWidth,
                                         PRInt32 aHeight);
    NS_IMETHOD         Enable           (PRBool  aState);
    NS_IMETHOD         IsEnabled        (PRBool *aState);

    
    void OnDestroy(void);

    
    PRBool AreBoundsSane(void);

protected:
    nsCOMPtr<nsIWidget> mParent;
    
    PRPackedBool        mIsTopLevel;
    
    PRPackedBool        mIsDestroyed;

    
    
    PRPackedBool        mNeedsResize;
    
    
    PRPackedBool        mNeedsMove;
    
    PRPackedBool        mListenForResizes;
    
    PRPackedBool        mIsShown;
    PRPackedBool        mNeedsShow;
    
    PRBool              mEnabled;
    
    PRBool              mCreated;
    
    
    PRBool              mPlaced;

    
    PRUint32            mPreferredWidth;
    PRUint32            mPreferredHeight;
};

#endif 
