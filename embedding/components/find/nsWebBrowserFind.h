





#ifndef nsWebBrowserFindImpl_h__
#define nsWebBrowserFindImpl_h__

#include "nsIWebBrowserFind.h"

#include "nsCOMPtr.h"
#include "nsWeakReference.h"

#include "nsIFind.h"

#include "nsString.h"

#define NS_WEB_BROWSER_FIND_CONTRACTID "@mozilla.org/embedcomp/find;1"

#define NS_WEB_BROWSER_FIND_CID \
 {0x57cf9383, 0x3405, 0x11d5, {0xbe, 0x5b, 0xaa, 0x20, 0xfa, 0x2c, 0xf3, 0x7c}}

class nsISelection;
class nsIDOMWindow;

class nsIDocShell;





class nsWebBrowserFind  : public nsIWebBrowserFind,
                          public nsIWebBrowserFindInFrames
{
public:
                nsWebBrowserFind();
    
    
    NS_DECL_ISUPPORTS
    
    
    NS_DECL_NSIWEBBROWSERFIND
                
    
    NS_DECL_NSIWEBBROWSERFINDINFRAMES


protected:

    virtual     ~nsWebBrowserFind();

    bool        CanFindNext()
                { return mSearchString.Length() != 0; }

    nsresult    SearchInFrame(nsIDOMWindow* aWindow, bool aWrapping,
                              bool* didFind);

    nsresult    OnStartSearchFrame(nsIDOMWindow *aWindow);
    nsresult    OnEndSearchFrame(nsIDOMWindow *aWindow);

    void        GetFrameSelection(nsIDOMWindow* aWindow, nsISelection** aSel);
    nsresult    ClearFrameSelection(nsIDOMWindow *aWindow);
    
    nsresult    OnFind(nsIDOMWindow *aFoundWindow);
    
    nsIDocShell *GetDocShellFromWindow(nsIDOMWindow *inWindow);

    void        SetSelectionAndScroll(nsIDOMWindow* aWindow, 
                                      nsIDOMRange* aRange);

    nsresult    GetRootNode(nsIDOMDocument* aDomDoc, nsIDOMNode** aNode);
    nsresult    GetSearchLimits(nsIDOMRange* aRange,
                                nsIDOMRange* aStartPt,
                                nsIDOMRange* aEndPt,
                                nsIDOMDocument* aDoc,
                                nsISelection* aSel,
                                bool aWrap);
    nsresult    SetRangeAroundDocument(nsIDOMRange* aSearchRange,
                                       nsIDOMRange* aStartPoint,
                                       nsIDOMRange* aEndPoint,
                                       nsIDOMDocument* aDoc);
    
protected:

    nsString        mSearchString;
    
    bool            mFindBackwards;
    bool            mWrapFind;
    bool            mEntireWord;
    bool            mMatchCase;
    
    bool            mSearchSubFrames;
    bool            mSearchParentFrames;

    nsWeakPtr       mCurrentSearchFrame;    
    nsWeakPtr       mRootSearchFrame;       
    nsWeakPtr       mLastFocusedWindow;     
};

#endif
