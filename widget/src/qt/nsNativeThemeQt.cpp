






































#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QComboBox>
#include <QRect>
#include <QPainter>
#include <QStyleOption>
 #include <QStyleOptionFrameV2>

#include "nsNativeThemeQt.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsILookAndFeel.h"
#include "nsIServiceManager.h"
#include "nsIEventStateManager.h"
#include <malloc.h>

#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxQPainterSurface.h"
#include "nsIRenderingContext.h"

nsNativeThemeQt::nsNativeThemeQt()
{
    combo = new QComboBox((QWidget *)0);
    combo->resize(0, 0);
    ThemeChanged();
}

nsNativeThemeQt::~nsNativeThemeQt()
{
    delete combo;
}

NS_IMPL_ISUPPORTS1(nsNativeThemeQt, nsITheme)

static QRect qRect(const nsRect &aRect, const nsTransform2D *aTrans)
{
    int x = aRect.x;
    int y = aRect.y;
    int w = aRect.width;
    int h = aRect.height;
    aTrans->TransformCoord(&x,&y,&w,&h);
    return QRect(x, y, w, h);
}

NS_IMETHODIMP
nsNativeThemeQt::DrawWidgetBackground(nsIRenderingContext* aContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType,
                                      const nsRect& aRect,
                                      const nsRect& aClipRect)
{


    gfxContext* context = aContext->ThebesContext();
    nsRefPtr<gfxASurface> surface =  context->CurrentSurface();
    gfxASurface* raw = surface;
    gfxQPainterSurface* qsurface = (gfxQPainterSurface*)raw;
    QPainter* qpainter = qsurface->GetQPainter();


    if (!qpainter)
        return NS_OK;

    QStyle* style = qApp->style();
    const QPalette::ColorGroup cg = qApp->palette().currentColorGroup();

    nsTransform2D* curTrans;
    aContext->GetCurrentTransform(curTrans);

    QRect r = qRect(aRect, curTrans);
    QRect cr = qRect(aClipRect, curTrans);


    qpainter->save();
    qpainter->translate(r.x(), r.y());
    r.translate(-r.x(), -r.y());





    QStyle::PrimitiveElement pe = QStyle::PE_CustomBase;



    QStyle::State flags = IsDisabled(aFrame) ?
                            QStyle::State_None :
                            QStyle::State_Enabled;

    PRInt32 eventState = GetContentState(aFrame, aWidgetType);


    if (eventState & NS_EVENT_STATE_HOVER)
        flags |= QStyle::State_MouseOver;
    if (eventState & NS_EVENT_STATE_FOCUS)
        flags |= QStyle::State_HasFocus;
    if (eventState & NS_EVENT_STATE_ACTIVE)
        flags |= QStyle::State_DownArrow;

    switch (aWidgetType) {
    case NS_THEME_RADIO:


        break;
    case NS_THEME_CHECKBOX:


        break;
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:

        break;
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:

        
    case NS_THEME_SCROLLBAR_BUTTON_UP:

        break;
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:

        
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:

        break;
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:

        
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:

        break;
    case NS_THEME_BUTTON_BEVEL:


        break;
    case NS_THEME_BUTTON:


        break;
    case NS_THEME_DROPDOWN:

        break;
    case NS_THEME_DROPDOWN_BUTTON:



        break;
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
        break;
    case NS_THEME_TEXTFIELD:
    case NS_THEME_LISTBOX: {
        qDebug("NS_THEME_TEXTFIELD || NS_THEME_LISTBOX");
        
        pe = QStyle::PE_PanelLineEdit;

        QStyleOptionFrameV2 option;
        option.direction = QApplication::layoutDirection();
        option.rect = r;
        option.type = QStyleOption::SO_Frame;
        option.state = flags;
        option.lineWidth = 10;
        option.midLineWidth = 10;
        option.features = QStyleOptionFrameV2::Flat;
        style->drawPrimitive(pe, &option, qpainter, NULL);
        break;
    }
    default:
        break;
    }





    qpainter->restore();
    return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeQt::GetWidgetBorder(nsIDeviceContext* aContext,
                                 nsIFrame* aFrame,
                                 PRUint8 aWidgetType,
                                 nsMargin* aResult)
{


    (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;

    switch(aWidgetType) {
    case NS_THEME_TEXTFIELD:
    case NS_THEME_LISTBOX:
        (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right =
                         frameWidth;
    }

    return NS_OK;
}

PRBool
nsNativeThemeQt::GetWidgetPadding(nsIDeviceContext* ,
                                  nsIFrame*, PRUint8 aWidgetType,
                                  nsMargin* aResult)
{


    
    if (aWidgetType == NS_THEME_BUTTON_FOCUS ||
        aWidgetType == NS_THEME_TOOLBAR_BUTTON ||
        aWidgetType == NS_THEME_TOOLBAR_DUAL_BUTTON) {
        aResult->SizeTo(0, 0, 0, 0);
        return PR_TRUE;
    }

    return PR_FALSE;
}

NS_IMETHODIMP
nsNativeThemeQt::GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                      PRUint8 aWidgetType,
                                      nsSize* aResult, PRBool* aIsOverridable)
{


    (*aResult).width = (*aResult).height = 0;
    *aIsOverridable = PR_TRUE;

    QStyle *s = qApp->style();

    switch (aWidgetType) {
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX: {
        QRect rect = s->subElementRect(aWidgetType == NS_THEME_CHECKBOX
                               ? QStyle::SE_CheckBoxIndicator
                               : QStyle::SE_RadioButtonIndicator,
                               0);
        (*aResult).width = rect.width();
        (*aResult).height = rect.height();
        break;
    }
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        (*aResult).height = (*aResult).width;
        *aIsOverridable = PR_FALSE;
        break;
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarSliderMin);
        *aIsOverridable = PR_FALSE;
        break;
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarSliderMin);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        *aIsOverridable = PR_FALSE;
        break;
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
        break;
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
        break;
    case NS_THEME_DROPDOWN_BUTTON: {



        break;
    }
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TEXTFIELD:
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeQt::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType,
                                    nsIAtom* aAttribute, PRBool* aShouldRepaint)
{


    *aShouldRepaint = TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsNativeThemeQt::ThemeChanged()
{


    QStyle *s = qApp->style();
    if (s)
      frameWidth = s->pixelMetric(QStyle::PM_DefaultFrameWidth);
    return NS_OK;
}

PRBool
nsNativeThemeQt::ThemeSupportsWidget(nsPresContext* aPresContext,
                                     nsIFrame* aFrame,
                                     PRUint8 aWidgetType)
{



    switch (aWidgetType) {
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX:
    case NS_THEME_BUTTON_BEVEL:
    case NS_THEME_BUTTON:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_BUTTON:
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_LISTBOX:

        return PR_TRUE;
    default:
        break;
    }

    return PR_FALSE;
}

PRBool
nsNativeThemeQt::WidgetIsContainer(PRUint8 aWidgetType)
{
    
    if (aWidgetType == NS_THEME_DROPDOWN_BUTTON ||
        aWidgetType == NS_THEME_RADIO ||
        aWidgetType == NS_THEME_CHECKBOX) {

        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
nsNativeThemeQt::ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType)
{
    if (aWidgetType == NS_THEME_DROPDOWN ||
        aWidgetType == NS_THEME_BUTTON || 
        aWidgetType == NS_THEME_TREEVIEW_HEADER_CELL) { 

        return PR_TRUE;
    }

    return PR_FALSE;
}

PRBool
nsNativeThemeQt::ThemeNeedsComboboxDropmarker()
{

    return PR_FALSE;
}
