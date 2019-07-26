





#ifndef nsDSURIContentListener_h__
#define nsDSURIContentListener_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIURIContentListener.h"
#include "nsWeakReference.h"

class nsDocShell;
class nsIWebNavigationInfo;
class nsIHttpChannel;

class nsDSURIContentListener :
    public nsIURIContentListener,
    public nsSupportsWeakReference

{
friend class nsDocShell;
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIURICONTENTLISTENER

    nsresult Init();

protected:
    nsDSURIContentListener(nsDocShell* aDocShell);
    virtual ~nsDSURIContentListener();

    void DropDocShellreference() {
        mDocShell = nullptr;
    }

    
    
    bool CheckFrameOptions(nsIRequest* request);
    bool CheckOneFrameOptionsPolicy(nsIHttpChannel* httpChannel,
                                    const nsAString& policy);

    enum XFOHeader {
      eDENY,
      eSAMEORIGIN,
      eALLOWFROM
    };

    void ReportXFOViolation(nsIDocShellTreeItem* aTopDocShellItem,
                            nsIURI* aThisURI,
                            XFOHeader aHeader);
protected:
    nsDocShell*                      mDocShell;

    
    
    
    nsWeakPtr                        mWeakParentContentListener;
    nsIURIContentListener*           mParentContentListener;

    nsCOMPtr<nsIWebNavigationInfo>   mNavInfo;
};

#endif 
