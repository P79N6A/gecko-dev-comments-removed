






































#include "nsWindow.h"

#include "mozqwidget.h"

#include <qvbox.h>
#include <qwidget.h>
#include <qlayout.h>

NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsCommonWidget,
                             nsISupportsWeakReference)

nsWindow::nsWindow()
{
}

nsWindow::~nsWindow()
{
}

QWidget*
nsWindow::createQWidget(QWidget *parent, nsWidgetInitData *aInitData)
{
    Qt::WFlags flags = Qt::WNoAutoErase|Qt::WStaticContents;
#ifdef DEBUG_WIDGETS
    qDebug("NEW WIDGET\n\tparent is %p (%s)", (void*)parent,
           parent ? parent->name() : "null");
#endif
    
    switch (mWindowType) {
    case eWindowType_dialog:
    case eWindowType_popup:
    case eWindowType_toplevel:
    case eWindowType_invisible: {
        if (mWindowType == eWindowType_dialog) {
            flags |= Qt::WType_Dialog;
            mContainer = new MozQWidget(this, parent, "topLevelDialog", flags);
            qDebug("\t\t#### dialog (%p)", (void*)mContainer);
            
        }
        else if (mWindowType == eWindowType_popup) {
            flags |= Qt::WType_Popup;
            mContainer = new MozQWidget(this, parent, "topLevelPopup", flags);
            qDebug("\t\t#### popup (%p)", (void*)mContainer);
            mContainer->setFocusPolicy(QWidget::WheelFocus);
        }
        else { 
            flags |= Qt::WType_TopLevel;
            mContainer = new MozQWidget(this, parent, "topLevelWindow", flags);
            qDebug("\t\t#### toplevel (%p)", (void*)mContainer);
            
        }
        mWidget = mContainer;
    }
        break;
    case eWindowType_child: {
        mWidget = new MozQWidget(this, parent, "paintArea", 0);
        qDebug("\t\t#### child (%p)", (void*)mWidget);
    }
        break;
    default:
        break;
    }

    mWidget->setBackgroundMode(Qt::NoBackground);

    return mWidget;
}

NS_IMETHODIMP
nsWindow::PreCreateWidget(nsWidgetInitData *aWidgetInitData)
{
    if (nsnull != aWidgetInitData) {
        mWindowType = aWidgetInitData->mWindowType;
        mBorderStyle = aWidgetInitData->mBorderStyle;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}


ChildWindow::ChildWindow()
{
}

ChildWindow::~ChildWindow()
{
}

PRBool
ChildWindow::IsChild() const
{
    return PR_TRUE;
}

PopupWindow::PopupWindow()
{
    qDebug("===================== popup!");
}

PopupWindow::~PopupWindow()
{
}

