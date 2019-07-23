






































#ifndef EMBEDEVENTLISTENER_H
#define EMBEDEVENTLISTENER_H

#include <nsIDOMKeyListener.h>
#include <nsIDOMMouseListener.h>
#include <nsIDOMUIListener.h>

class QGeckoEmbed;

class EmbedEventListener : public nsIDOMKeyListener,
                           public nsIDOMMouseListener,
                           public nsIDOMUIListener
{
public:
    EmbedEventListener(QGeckoEmbed *q);
    ~EmbedEventListener();

    NS_DECL_ISUPPORTS

    

    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

    

    NS_IMETHOD KeyDown(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD KeyUp(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD KeyPress(nsIDOMEvent* aDOMEvent);

    

    NS_IMETHOD MouseDown(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseUp(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseClick(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseDblClick(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseOver(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseOut(nsIDOMEvent* aDOMEvent);

    

    NS_IMETHOD Activate(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD FocusIn(nsIDOMEvent* aDOMEvent);
    NS_IMETHOD FocusOut(nsIDOMEvent* aDOMEvent);

private:

    QGeckoEmbed *mOwner;
};

#endif
