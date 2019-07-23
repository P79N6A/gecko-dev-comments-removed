




































#ifndef nsIXULPopupListener_h__
#define nsIXULPopupListener_h__


#define NS_IXULPOPUPLISTENER_IID \
{ 0x2c453161, 0x942, 0x11d3, { 0xbf, 0x87, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

class nsIDOMElement;

typedef enum {
    eXULPopupType_popup,
    eXULPopupType_context,
    eXULPopupType_tooltip,
    eXULPopupType_blur
} XULPopupType;

class nsIXULPopupListener: public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULPOPUPLISTENER_IID)

    NS_IMETHOD Init(nsIDOMElement* anElement, const XULPopupType& aPopupType) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULPopupListener, NS_IXULPOPUPLISTENER_IID)

nsresult
NS_NewXULPopupListener(nsIXULPopupListener** result);

#endif 
