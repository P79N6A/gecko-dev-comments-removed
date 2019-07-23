





































#ifndef __CWindowCreator__
#define __CWindowCreator__

#include "nsIWindowCreator2.h"
#include "nsIWebBrowserChrome.h"





class CWindowCreator : public nsIWindowCreator2
{
  public:
                        CWindowCreator();
    virtual             ~CWindowCreator();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWINDOWCREATOR
    NS_DECL_NSIWINDOWCREATOR2

    
    static nsresult     Initialize();
        
    static LWindow*     CreateWindowInternal(PRUint32 inChromeFlags,
                                             PRBool enablePrinting,
                                             PRInt32 width, PRInt32 height); 
};

#endif 
