






































#ifndef nsScrollbar_h__
#define nsScrollbar_h__

#include "nsCommonWidget.h"
#include "nsINativeScrollbar.h"
#include <qscrollbar.h>





class nsQScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    nsQScrollBar(int aMinValue, int aMaxValue, int aLineStep,
                 int aPageStep, int aValue, Orientation aOrientation,
                 QWidget *aParent, const char *aName = 0);

    void ScrollBarMoved(int aMessage,int aValue = -1);

public slots:
    void slotValueChanged(int aValue);
private:
};





class nsNativeScrollbar : public nsCommonWidget,
                          public nsINativeScrollbar
{
public:
    nsNativeScrollbar();
    virtual ~nsNativeScrollbar();

    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSINATIVESCROLLBAR

    

    virtual PRBool OnScroll(nsScrollbarEvent &aEvent,PRUint32 cPos);

    virtual PRBool IsScrollBar() const { return PR_TRUE; }
protected:
    QWidget *createQWidget(QWidget *parent, nsWidgetInitData *aInitData);


private:
    QScrollBar::Orientation mOrientation;
    int mMaxValue;
    int mLineStep;
    int mPageStep;
    int mValue;
    PRUint32 mNsSBID;
};

#endif
