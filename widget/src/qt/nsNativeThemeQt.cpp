







































#include "nsIFrame.h"

#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QComboBox>
#include <QRect>
#include <QPainter>
#include <QStyleOption>
#include <QStyleOptionFrameV2>
#include <QStyleOptionButton>
#include <QFlags>

#include "nsCoord.h"
#include "nsNativeThemeQt.h"
#include "nsIDeviceContext.h"


#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsILookAndFeel.h"
#include "nsIServiceManager.h"
#include "nsIEventStateManager.h"
#include "nsIDOMHTMLInputElement.h"
#include <malloc.h>


#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxQPainterSurface.h"
#include "nsIRenderingContext.h"

nsNativeThemeQt::nsNativeThemeQt()
{
    combo = new QComboBox((QWidget *)0);
    combo->resize(0, 0);
    mNoBackgroundPalette.setColor(QPalette::Window, Qt::transparent);
    mSolidPalette.setColor(QPalette::Window, Qt::white);
    ThemeChanged();
}

nsNativeThemeQt::~nsNativeThemeQt()
{
    delete combo;
}

NS_IMPL_ISUPPORTS1(nsNativeThemeQt, nsITheme)

static inline QRect qRectInPixels(const nsRect &aRect,
                                  const PRInt32 p2a)
{
    return QRect(NSAppUnitsToIntPixels(aRect.x, p2a),
                 NSAppUnitsToIntPixels(aRect.y, p2a),
                 NSAppUnitsToIntPixels(aRect.width, p2a),
                 NSAppUnitsToIntPixels(aRect.height, p2a));
}

NS_IMETHODIMP
nsNativeThemeQt::DrawWidgetBackground(nsIRenderingContext* aContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType,
                                      const nsRect& aRect,
                                      const nsRect& aClipRect)
{
    gfxContext* context = aContext->ThebesContext();
    nsRefPtr<gfxASurface> surface = context->CurrentSurface();

    context->UpdateSurfaceClip();

    if (surface->GetType() != gfxASurface::SurfaceTypeQPainter)
        return NS_ERROR_NOT_IMPLEMENTED;

    gfxQPainterSurface* qSurface = (gfxQPainterSurface*) (surface.get());
    QPainter* qPainter = qSurface->GetQPainter();

    NS_ASSERTION(qPainter, "Where'd my QPainter go?");

    if (qPainter == nsnull)
        return NS_OK;

    QStyle* style = qApp->style();

    qPainter->save();

    gfxPoint offs = surface->GetDeviceOffset();
    qPainter->translate(offs.x, offs.y);

    gfxMatrix ctm = context->CurrentMatrix();
    if (!ctm.HasNonTranslation()) {
        ctm.x0 = NSToCoordRound(ctm.x0);
        ctm.y0 = NSToCoordRound(ctm.y0);
    }

    QMatrix qctm(ctm.xx, ctm.xy, ctm.yx, ctm.yy, ctm.x0, ctm.y0);
    qPainter->setWorldMatrix(qctm, true);

    PRInt32 p2a = GetAppUnitsPerDevPixel(aContext);

    QRect r = qRectInPixels(aRect, p2a);
    QRect cr = qRectInPixels(aClipRect, p2a);

    QStyle::State extraFlags = QStyle::State_None;

    switch (aWidgetType) {
    case NS_THEME_RADIO:
    case NS_THEME_RADIO_SMALL: 
    case NS_THEME_CHECKBOX:
    case NS_THEME_CHECKBOX_SMALL: {
        QStyleOptionButton opt;
        InitButtonStyle (aWidgetType, aFrame, r, opt);

        if (aWidgetType == NS_THEME_CHECKBOX ||
            aWidgetType == NS_THEME_CHECKBOX_SMALL)
        {
            style->drawPrimitive (QStyle::PE_IndicatorCheckBox, &opt, qPainter);
        } else {
            style->drawPrimitive (QStyle::PE_IndicatorRadioButton, &opt, qPainter);
        }
        break;
    }
    case NS_THEME_BUTTON:
    case NS_THEME_BUTTON_BEVEL: {
        QStyleOptionButton opt;
        InitButtonStyle (aWidgetType, aFrame, r, opt);

        if (aWidgetType == NS_THEME_BUTTON) {
            style->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, qPainter);
            if (IsDefaultButton(aFrame))
                style->drawPrimitive(QStyle::PE_FrameDefaultButton, &opt, qPainter);
        } else {
            style->drawPrimitive(QStyle::PE_PanelButtonBevel, &opt, qPainter);
            style->drawPrimitive(QStyle::PE_FrameButtonBevel, &opt, qPainter);
        }
        break;
    }
    case NS_THEME_SCROLLBAR: {
        qPainter->fillRect(r, qApp->palette().brush(QPalette::Active, QPalette::Background));
        break;
    }
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL: {
        qPainter->fillRect(r, qApp->palette().brush(QPalette::Active, QPalette::Background));
        break;
    }
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL: {
        qPainter->fillRect(r, qApp->palette().brush(QPalette::Active, QPalette::Background));
        break;
    }
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
        extraFlags |= QStyle::State_Horizontal;
        
    case NS_THEME_SCROLLBAR_BUTTON_UP: {
        QStyleOption option;
        InitPlainStyle(aWidgetType, aFrame, r, option, extraFlags);

        style->drawControl(QStyle::CE_ScrollBarSubLine, &option, qPainter, NULL);
        break;
    }
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
        extraFlags |= QStyle::State_Horizontal;
        
    case NS_THEME_SCROLLBAR_BUTTON_DOWN: {
        QStyleOption option;
        InitPlainStyle(aWidgetType, aFrame, r, option, extraFlags);

        style->drawControl(QStyle::CE_ScrollBarAddLine, &option, qPainter, NULL);

        break;
    }
    
    
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
        extraFlags |= QStyle::State_Horizontal;
        
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL: {
        QStyleOption option;
        InitPlainStyle(aWidgetType, aFrame, r, option, extraFlags);

        style->drawControl(QStyle::CE_ScrollBarSlider, &option, qPainter, NULL);
    }
    case NS_THEME_DROPDOWN:

        break;
    case NS_THEME_DROPDOWN_BUTTON:



        break;
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
        break;
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_LISTBOX: {
        QStyleOptionFrameV2 frameOpt;
        
        if (!IsDisabled(aFrame))
            frameOpt.state |= QStyle::State_Enabled;

        frameOpt.rect = r;
        frameOpt.features = QStyleOptionFrameV2::Flat;

        if (aWidgetType == NS_THEME_TEXTFIELD || aWidgetType == NS_THEME_TEXTFIELD_MULTILINE) {
            QRect contentRect = style->subElementRect(QStyle::SE_LineEditContents, &frameOpt);

            PRInt32 frameWidth = style->pixelMetric(QStyle::PM_DefaultFrameWidth);

            contentRect.adjust(frameWidth, frameWidth, -frameWidth, -frameWidth);
            qPainter->fillRect(contentRect, QBrush(Qt::white));
        }
        
        frameOpt.palette = mNoBackgroundPalette;
        style->drawPrimitive(QStyle::PE_FrameLineEdit, &frameOpt, qPainter, NULL);
        break;
    }
    default:
        break;
    }

    qPainter->restore();
    return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeQt::GetWidgetBorder(nsIDeviceContext* aContext,
                                 nsIFrame* aFrame,
                                 PRUint8 aWidgetType,
                                 nsMargin* aResult)
{


    (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;








    return NS_OK;
}

PRBool
nsNativeThemeQt::GetWidgetPadding(nsIDeviceContext* ,
                                  nsIFrame*, PRUint8 aWidgetType,
                                  nsMargin* aResult)
{
    




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

    PRInt32 p2a = GetAppUnitsPerDevPixel(aContext);

    switch (aWidgetType) {
    case NS_THEME_RADIO_SMALL:
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX_SMALL:
    case NS_THEME_CHECKBOX: {
        nsRect frameRect = aFrame->GetRect();

        QRect qRect = qRectInPixels(frameRect, p2a);

        QStyleOptionButton option;

        InitButtonStyle(aWidgetType, aFrame, qRect, option);

        QRect rect = s->subElementRect(
            (aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_CHECKBOX_SMALL ) ?
                QStyle::SE_CheckBoxIndicator :
                QStyle::SE_RadioButtonIndicator,
            &option,
            NULL);

        (*aResult).width = rect.width();
        (*aResult).height = rect.height();
        break;
    }
    case NS_THEME_BUTTON: {
        nsRect frameRect = aFrame->GetRect();

        QRect qRect = qRectInPixels(frameRect, p2a);

        QStyleOptionButton option;

        InitButtonStyle(aWidgetType, aFrame, qRect, option);

        QRect rect = s->subElementRect(
            QStyle::SE_PushButtonFocusRect,
            &option,
            NULL);

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
        
        break;
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarSliderMin);
        
        break;
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarSliderMin);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        
        break;
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        (*aResult).height = s->pixelMetric(QStyle::PM_SliderLength);
        break;
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_SliderLength);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        break;
    case NS_THEME_DROPDOWN_BUTTON: {



        break;
    }
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
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
    
    
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_RADIO:
    case NS_THEME_RADIO_SMALL:
    case NS_THEME_CHECKBOX:
    case NS_THEME_CHECKBOX_SMALL:
    case NS_THEME_BUTTON_BEVEL:
    case NS_THEME_BUTTON:
    
    
    
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    

        return PR_TRUE;
    default:

        break;
    }

    return PR_FALSE;
}

PRBool
nsNativeThemeQt::WidgetIsContainer(PRUint8 aWidgetType)
{

    









    return PR_TRUE;
}

PRBool
nsNativeThemeQt::ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType)
{










    return PR_FALSE;
}

PRBool
nsNativeThemeQt::ThemeNeedsComboboxDropmarker()
{

    return PR_FALSE;
}

void
nsNativeThemeQt::InitButtonStyle(PRUint8 aWidgetType,
                                 nsIFrame* aFrame,
                                 QRect rect,
                                 QStyleOptionButton &opt)
{
    PRInt32 eventState = GetContentState(aFrame, aWidgetType);

    opt.rect = rect;
    opt.palette = mNoBackgroundPalette;

    PRBool disabled = IsDisabled(aFrame);

    if (!disabled)
        opt.state |= QStyle::State_Enabled;
    if (eventState & NS_EVENT_STATE_HOVER)
        opt.state |= QStyle::State_MouseOver;
    if (eventState & NS_EVENT_STATE_FOCUS)
        opt.state |= QStyle::State_HasFocus;
    if (!disabled && eventState & NS_EVENT_STATE_ACTIVE)
        
        opt.state |= QStyle::State_Sunken;

    switch (aWidgetType) {
    case NS_THEME_RADIO:
    case NS_THEME_RADIO_SMALL:
    case NS_THEME_CHECKBOX:
    case NS_THEME_CHECKBOX_SMALL:
        if (IsChecked(aFrame))
            opt.state |= QStyle::State_On;
        else
            opt.state |= QStyle::State_Off;

        break;
    default:
        if (!(eventState & NS_EVENT_STATE_ACTIVE))
            opt.state |= QStyle::State_Raised;
        break;
    }
}

void
nsNativeThemeQt::InitPlainStyle(PRUint8 aWidgetType,
                                nsIFrame* aFrame,
                                QRect rect,
                                QStyleOption &opt,
                                QStyle::State extraFlags)
{
    PRInt32 eventState = GetContentState(aFrame, aWidgetType);

    opt.rect = rect;

    if (!IsDisabled(aFrame))
        opt.state |= QStyle::State_Enabled;
    if (eventState & NS_EVENT_STATE_HOVER)
        opt.state |= QStyle::State_MouseOver;
    if (eventState & NS_EVENT_STATE_FOCUS)
        opt.state |= QStyle::State_HasFocus;

    opt.state |= extraFlags;
}
