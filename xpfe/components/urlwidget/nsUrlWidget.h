




































#ifndef nsurlwidget_h___
#define nsurlwidget_h___

#include "nsIUrlWidget.h"


#define NS_IURLWIDGET_CID { 0x1802EE82, 0x34A1, 0x11d4, { 0x82, 0xEE, 0x00, 0x50, 0xDA, 0x2D, 0xA7, 0x71 } }


class nsUrlWidget : public nsIUrlWidget {
public:
    nsUrlWidget();
    virtual ~nsUrlWidget();
    nsresult Init();
	
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURLWIDGET
};
#endif
