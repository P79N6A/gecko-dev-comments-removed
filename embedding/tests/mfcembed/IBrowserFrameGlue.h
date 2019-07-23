










































#ifndef _IBROWSERFRAMEGLUE_H
#define _IBROWSERFRAMEGLUE_H

struct IBrowserFrameGlue {
    
    virtual void UpdateStatusBarText(const PRUnichar *aMessage) = 0;
    virtual void UpdateProgress(PRInt32 aCurrent, PRInt32 aMax) = 0;
    virtual void UpdateBusyState(PRBool aBusy) = 0;
    virtual void UpdateCurrentURI(nsIURI *aLocation) = 0;
    virtual void UpdateSecurityStatus(PRInt32 aState) = 0;

    
    virtual PRBool CreateNewBrowserFrame(PRUint32 chromeMask, 
                            PRInt32 x, PRInt32 y, 
                            PRInt32 cx, PRInt32 cy,
                            nsIWebBrowser ** aWebBrowser) = 0;
    virtual void DestroyBrowserFrame() = 0;
    virtual void GetBrowserFrameTitle(PRUnichar **aTitle) = 0;
    virtual void SetBrowserFrameTitle(const PRUnichar *aTitle) = 0;
    virtual void GetBrowserFramePosition(PRInt32 *aX, PRInt32 *aY) = 0;
    virtual void SetBrowserFramePosition(PRInt32 aX, PRInt32 aY) = 0;
    virtual void GetBrowserFrameSize(PRInt32 *aCX, PRInt32 *aCY) = 0;
    virtual void SetBrowserFrameSize(PRInt32 aCX, PRInt32 aCY) = 0;
    virtual void GetBrowserFramePositionAndSize(PRInt32 *aX, PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY) = 0;
    virtual void SetBrowserFramePositionAndSize(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, PRBool fRepaint) = 0;
    virtual void ShowBrowserFrame(PRBool aShow) = 0;
    virtual void SetFocus() = 0;
    virtual void FocusAvailable(PRBool *aFocusAvail) = 0;
    virtual void GetBrowserFrameVisibility(PRBool *aVisible) = 0;

    
    virtual void ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo) = 0;

    
    virtual void ShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText) = 0;
    virtual void HideTooltip() = 0;

    virtual HWND GetBrowserFrameNativeWnd() = 0;
};

#define    NS_DECL_BROWSERFRAMEGLUE    \
    public:    \
        virtual void UpdateStatusBarText(const PRUnichar *aMessage);    \
        virtual void UpdateProgress(PRInt32 aCurrent, PRInt32 aMax);    \
        virtual void UpdateBusyState(PRBool aBusy);                        \
        virtual void UpdateCurrentURI(nsIURI *aLocation);                \
        virtual void UpdateSecurityStatus(PRInt32 aState);              \
        virtual PRBool CreateNewBrowserFrame(PRUint32 chromeMask, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy, nsIWebBrowser** aWebBrowser);    \
        virtual void DestroyBrowserFrame();                            \
        virtual void GetBrowserFrameTitle(PRUnichar **aTitle);    \
        virtual void SetBrowserFrameTitle(const PRUnichar *aTitle);    \
        virtual void GetBrowserFramePosition(PRInt32 *aX, PRInt32 *aY);    \
        virtual void SetBrowserFramePosition(PRInt32 aX, PRInt32 aY);    \
        virtual void GetBrowserFrameSize(PRInt32 *aCX, PRInt32 *aCY);    \
        virtual void SetBrowserFrameSize(PRInt32 aCX, PRInt32 aCY);        \
        virtual void GetBrowserFramePositionAndSize(PRInt32 *aX, PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY);    \
        virtual void SetBrowserFramePositionAndSize(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, PRBool fRepaint);    \
        virtual void ShowBrowserFrame(PRBool aShow);                    \
        virtual void SetFocus();                                        \
        virtual void FocusAvailable(PRBool *aFocusAvail);                \
        virtual void GetBrowserFrameVisibility(PRBool *aVisible);        \
        virtual void ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo); \
        virtual void ShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText); \
        virtual void HideTooltip(); \
        virtual HWND GetBrowserFrameNativeWnd();
        
typedef IBrowserFrameGlue *PBROWSERFRAMEGLUE;

#endif 
