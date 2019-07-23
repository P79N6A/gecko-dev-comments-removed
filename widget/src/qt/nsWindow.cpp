






































#include "nsWindow.h"

#include "mozqwidget.h"

#include "gfxPlatform.h"
#include "gfxXlibSurface.h"

#include <qwidget.h>
#include <qlayout.h>
#include <QX11Info>

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
    Qt::WFlags flags = Qt::Widget;
    Qt::WA_StaticContents;
#ifdef DEBUG_WIDGETS
    qDebug("NEW WIDGET\n\tparent is %p (%s)", (void*)parent,
           parent ? qPrintable(parent->objectName()) : "null");
#endif
    
    switch (mWindowType) {
    case eWindowType_dialog:
    case eWindowType_popup:
    case eWindowType_toplevel:
    case eWindowType_invisible: {
        if (mWindowType == eWindowType_dialog) {
            flags |= Qt::Dialog;
            mContainer = new MozQWidget(this, parent, "topLevelDialog", flags);
            qDebug("\t\t#### dialog (%p)", (void*)mContainer);
            
        }
        else if (mWindowType == eWindowType_popup) {
            flags |= Qt::Popup;
            mContainer = new MozQWidget(this, parent, "topLevelPopup", flags);
            qDebug("\t\t#### popup (%p)", (void*)mContainer);
            mContainer->setFocusPolicy(Qt::WheelFocus);
        }
        else { 
            flags |= Qt::Window;
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

    mWidget->setAttribute(Qt::WA_StaticContents);
    mWidget->setAttribute(Qt::WA_OpaquePaintEvent); 

    
    

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

gfxASurface*
nsWindow::GetThebesSurface()
{
    
    
    
    
    mThebesSurface = nsnull;

    if (!mThebesSurface) {
        PRInt32 x_offset = 0, y_offset = 0;
        int width = mWidget->width(), height = mWidget->height();

        
        width = PR_MIN(32767, height);
        height = PR_MIN(32767, width);

        if (!gfxPlatform::UseGlitz()) {
            qDebug("QT_WIDGET NOT SURE: Func:%s::%d, [%ix%i]\n", __PRETTY_FUNCTION__, __LINE__, width, height);
            mThebesSurface = new gfxXlibSurface
            (mWidget->x11Info().display(),
            (Drawable)mWidget->handle(),
             static_cast<Visual*>(mWidget->x11Info().visual()),
             gfxIntSize(width, height));
            
            
            
            
             mWidget->setAttribute(Qt::WA_PaintOnScreen);

            
            
            if (mThebesSurface && mThebesSurface->CairoStatus() != 0)
                mThebesSurface = nsnull;
        } else {
#ifdef MOZ_ENABLE_GLITZ
            glitz_surface_t *gsurf;
            glitz_drawable_t *gdraw;

            glitz_drawable_format_t *gdformat = glitz_glx_find_window_format (GDK_DISPLAY(),
                                                mWidget->x11Info().appScreen(),
                                                0, NULL, 0);
            if (!gdformat)
                NS_ERROR("Failed to find glitz drawable format");

            Display* dpy = mWidget->x11Info().display(),;
            Window wnd = (Window)mWidget->x11Info().appRootWindow();

            gdraw =
                glitz_glx_create_drawable_for_window (dpy,
                                                      DefaultScreen(dpy),
                                                      gdformat,
                                                      wnd,
                                                      width,
                                                      height);
            glitz_format_t *gformat =
                glitz_find_standard_format (gdraw, GLITZ_STANDARD_RGB24);
            gsurf =
                glitz_surface_create (gdraw,
                                      gformat,
                                      width,
                                      height,
                                      0,
                                      NULL);
            glitz_surface_attach (gsurf, gdraw, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);


            
            mThebesSurface = new gfxGlitzSurface (gdraw, gsurf, PR_TRUE);
#endif
        }

        if (mThebesSurface) {
            mThebesSurface->SetDeviceOffset(gfxPoint(-x_offset, -y_offset));
        }
    }

    return mThebesSurface;
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



