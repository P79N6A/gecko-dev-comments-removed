




































#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QInputContext>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <QtGui/QGraphicsSceneDragDropEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsSceneHoverEvent>
#include <QtGui/QGraphicsSceneWheelEvent>

#include <QtCore/QEvent>
#include <QtCore/QVariant>
#include <QtCore/QTimer>

#include "mozqwidget.h"
#include "nsWindow.h"






static bool gKeyboardOpen = false;






static bool gFailedOpenKeyboard = false;
 







static bool gPendingVKBOpen = false;




static QString gLastPreeditString;

MozQWidget::MozQWidget(nsWindow* aReceiver, QGraphicsItem* aParent)
    : QGraphicsWidget(aParent),
      mReceiver(aReceiver)
{
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
     setFlag(QGraphicsItem::ItemAcceptsInputMethod);
     setAcceptTouchEvents(true);
#endif
}

MozQWidget::~MozQWidget()
{
    if (mReceiver)
        mReceiver->QWidgetDestroyed();
}

void MozQWidget::paint(QPainter* aPainter, const QStyleOptionGraphicsItem* aOption, QWidget* aWidget )
{
    mReceiver->DoPaint(aPainter, aOption, aWidget);
}

void MozQWidget::activate()
{
    
    hideVKB();
    mReceiver->DispatchActivateEventOnTopLevelWindow();
}

void MozQWidget::deactivate()
{
    
    hideVKB();
    mReceiver->DispatchDeactivateEventOnTopLevelWindow();
}

void MozQWidget::resizeEvent(QGraphicsSceneResizeEvent* aEvent)
{
    mReceiver->OnResizeEvent(aEvent);
}

void MozQWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent* aEvent)
{
    mReceiver->contextMenuEvent(aEvent);
}

void MozQWidget::dragEnterEvent(QGraphicsSceneDragDropEvent* aEvent)
{
    mReceiver->OnDragEnter(aEvent);
}

void MozQWidget::dragLeaveEvent(QGraphicsSceneDragDropEvent* aEvent)
{
    mReceiver->OnDragLeaveEvent(aEvent);
}

void MozQWidget::dragMoveEvent(QGraphicsSceneDragDropEvent* aEvent)
{
    mReceiver->OnDragMotionEvent(aEvent);
}

void MozQWidget::dropEvent(QGraphicsSceneDragDropEvent* aEvent)
{
    mReceiver->OnDragDropEvent(aEvent);
}

void MozQWidget::focusInEvent(QFocusEvent* aEvent)
{
    mReceiver->OnFocusInEvent(aEvent);

    
    
    
    if (gFailedOpenKeyboard)
        requestVKB(0);
}

void MozQWidget::focusOutEvent(QFocusEvent* aEvent)
{
    mReceiver->OnFocusOutEvent(aEvent);
    
    if (aEvent->reason() == Qt::OtherFocusReason && gKeyboardOpen) {
        hideVKB();
    }
}

void MozQWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* aEvent)
{
    mReceiver->OnEnterNotifyEvent(aEvent);
}

void MozQWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* aEvent)
{
    mReceiver->OnLeaveNotifyEvent(aEvent);
}

void MozQWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* aEvent)
{
    mReceiver->OnMoveEvent(aEvent);
}

void MozQWidget::keyPressEvent(QKeyEvent* aEvent)
{
#if (MOZ_PLATFORM_MAEMO == 6)
    if (!gKeyboardOpen ||
       
       aEvent->key() == Qt::Key_Space ||
       aEvent->key() == Qt::Key_Return ||
       aEvent->key() == Qt::Key_Backspace) {
        mReceiver->OnKeyPressEvent(aEvent);
    }
#elif (MOZ_PLATFORM_MAEMO == 5)
    
    
    
#else
    mReceiver->OnKeyPressEvent(aEvent);
#endif
}

void MozQWidget::keyReleaseEvent(QKeyEvent* aEvent)
{
#if (MOZ_PLATFORM_MAEMO == 6)
    if (!gKeyboardOpen ||
       
       aEvent->key() == Qt::Key_Space ||
       aEvent->key() == Qt::Key_Return ||
       aEvent->key() == Qt::Key_Backspace) {
        mReceiver->OnKeyReleaseEvent(aEvent);
    }
    return;
#elif (MOZ_PLATFORM_MAEMO == 5)
    
    mReceiver->OnKeyPressEvent(aEvent);
#endif
    mReceiver->OnKeyReleaseEvent(aEvent);
}

void MozQWidget::inputMethodEvent(QInputMethodEvent* aEvent)
{
    QString currentPreeditString = aEvent->preeditString();
    QString currentCommitString = aEvent->commitString();

    
    if (currentCommitString == " ") {
        sendPressReleaseKeyEvent(Qt::Key_Space, currentCommitString.unicode());
    } else if (currentCommitString == "\n") {
        sendPressReleaseKeyEvent(Qt::Key_Return, currentCommitString.unicode());
    } else if (currentCommitString.isEmpty()) {
        
        
        if (currentPreeditString.length() == 1 && gLastPreeditString.isEmpty()) {
            
            
            
            sendPressReleaseKeyEvent(0, currentPreeditString.unicode());
        } else if (currentPreeditString.startsWith(gLastPreeditString)) {
            
            
            
            const QChar * text = currentPreeditString.unicode();
            for (int i = gLastPreeditString.length(); i < currentPreeditString.length(); i++) {
                sendPressReleaseKeyEvent(0, &text[i]);
            }
        } else {
            
            
            
            QString tempLastPre = gLastPreeditString;
            tempLastPre.truncate(gLastPreeditString.length()-1);
            if (currentPreeditString == tempLastPre) {
                sendPressReleaseKeyEvent(Qt::Key_Backspace);
            } else if (currentPreeditString != tempLastPre) {
                
                
                for (int i = 0; i < gLastPreeditString.length(); i++) {
                    sendPressReleaseKeyEvent(Qt::Key_Backspace);
                }
                
                const QChar * text = currentPreeditString.unicode();
                for (int i = 0; i < currentPreeditString.length(); i++) {
                    sendPressReleaseKeyEvent(0, &text[i]);
                }
            }
        }
    } else if (gLastPreeditString != currentCommitString) {
        
        if (currentCommitString.length() == 1 && gLastPreeditString.isEmpty()) {
            
            
            sendPressReleaseKeyEvent(0, currentCommitString.unicode());
        } else {
            
            for (int i = 0; i < gLastPreeditString.length(); i++) {
                sendPressReleaseKeyEvent(Qt::Key_Backspace);
            }
            
            const QChar * text = currentCommitString.unicode();
            for (int i = 0; i < currentCommitString.length(); i++) {
                sendPressReleaseKeyEvent(0, &text[i]);
            }
        }
    }

    
    gLastPreeditString = currentPreeditString;

    
    
    
    QGraphicsWidget::inputMethodEvent(aEvent);
}

void MozQWidget::sendPressReleaseKeyEvent(int key,
                                          const QChar* letter,
                                          bool autorep,
                                          ushort count)
{
     Qt::KeyboardModifiers modifiers  = Qt::NoModifier;
     if (letter && letter->isUpper()) {
         modifiers = Qt::ShiftModifier;
     }

     QString text = letter ? QString(*letter) : QString();

     QKeyEvent press(QEvent::KeyPress, key, modifiers, text, autorep, count);
     mReceiver->OnKeyPressEvent(&press);
     QKeyEvent release(QEvent::KeyRelease, key, modifiers, text, autorep, count);
     mReceiver->OnKeyReleaseEvent(&release);
}

void MozQWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* aEvent)
{
    
    mReceiver->OnButtonPressEvent(aEvent);
    mReceiver->OnMouseDoubleClickEvent(aEvent);
}

void MozQWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* aEvent)
{
    mReceiver->OnMotionNotifyEvent(aEvent);
}

void MozQWidget::mousePressEvent(QGraphicsSceneMouseEvent* aEvent)
{
    mReceiver->OnButtonPressEvent(aEvent);
}

void MozQWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* aEvent)
{
    mReceiver->OnButtonReleaseEvent(aEvent);
}

bool MozQWidget::event ( QEvent * event )
{
    
    
    if (!mReceiver)
        return QGraphicsWidget::event(event);

#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    switch (event->type())
    {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    {
        
        
        PRBool handled = PR_FALSE;
        mReceiver->OnTouchEvent(static_cast<QTouchEvent *>(event),handled);
        return handled;
    }
    case (QEvent::Gesture):
    {
        PRBool handled = PR_FALSE;
        mReceiver->OnGestureEvent(static_cast<QGestureEvent*>(event),handled);
        return handled;
    }
#if (MOZ_PLATFORM_MAEMO != 6)
    
    case QEvent::InputMethod:
    {
        PRBool handled = PR_FALSE;
        mReceiver->imComposeEvent(static_cast<QInputMethodEvent*>(event),handled);
        return handled;
    }
#endif

    default:
        break;
    }
#endif
    return QGraphicsWidget::event(event);
}

void MozQWidget::wheelEvent(QGraphicsSceneWheelEvent* aEvent)
{
    mReceiver->OnScrollEvent(aEvent);
}

void MozQWidget::closeEvent(QCloseEvent* aEvent)
{
    mReceiver->OnCloseEvent(aEvent);
}

void MozQWidget::hideEvent(QHideEvent* aEvent)
{
    mReceiver->hideEvent(aEvent);
    QGraphicsWidget::hideEvent(aEvent);
}

void MozQWidget::showEvent(QShowEvent* aEvent)
{
    mReceiver->showEvent(aEvent);
    QGraphicsWidget::showEvent(aEvent);
}

bool MozQWidget::SetCursor(nsCursor aCursor)
{
    Qt::CursorShape cursor = Qt::ArrowCursor;
    switch(aCursor) {
    case eCursor_standard:
        cursor = Qt::ArrowCursor;
        break;
    case eCursor_wait:
        cursor = Qt::WaitCursor;
        break;
    case eCursor_select:
        cursor = Qt::IBeamCursor;
        break;
    case eCursor_hyperlink:
        cursor = Qt::PointingHandCursor;
        break;
    case eCursor_ew_resize:
        cursor = Qt::SplitHCursor;
        break;
    case eCursor_ns_resize:
        cursor = Qt::SplitVCursor;
        break;
    case eCursor_nw_resize:
    case eCursor_se_resize:
        cursor = Qt::SizeBDiagCursor;
        break;
    case eCursor_ne_resize:
    case eCursor_sw_resize:
        cursor = Qt::SizeFDiagCursor;
        break;
    case eCursor_crosshair:
    case eCursor_move:
        cursor = Qt::SizeAllCursor;
        break;
    case eCursor_help:
        cursor = Qt::WhatsThisCursor;
        break;
    case eCursor_copy:
    case eCursor_alias:
        break;
    case eCursor_context_menu:
    case eCursor_cell:
    case eCursor_grab:
    case eCursor_grabbing:
    case eCursor_spinning:
    case eCursor_zoom_in:
    case eCursor_zoom_out:

    default:
        break;
    }

    setCursor(cursor);

    return NS_OK;
}

bool MozQWidget::SetCursor(const QPixmap& aCursor, int aHotX, int aHotY)
{
    QCursor bitmapCursor(aCursor, aHotX, aHotY);
    setCursor(bitmapCursor);

    return NS_OK;
}

void MozQWidget::setModal(bool modal)
{
#if QT_VERSION >= 0x040600
    setPanelModality(modal ? QGraphicsItem::SceneModal : QGraphicsItem::NonModal);
#else
    LOG(("Modal QGraphicsWidgets not supported in Qt < 4.6\n"));
#endif
}

QVariant MozQWidget::inputMethodQuery(Qt::InputMethodQuery aQuery) const
{
    
    
    
    if (static_cast<Qt::InputMethodQuery>( 10004 ) == aQuery)
    {
        return QVariant( 0 );
    }

    return QGraphicsWidget::inputMethodQuery(aQuery);
}






void MozQWidget::requestVKB(int aTimeout)
{
    if (!gPendingVKBOpen) {
        gPendingVKBOpen = true;

        if (aTimeout == 0)
            showVKB();
        else
            QTimer::singleShot(aTimeout, this, SLOT(showVKB()));
    }
}



void MozQWidget::showVKB()
{
    
    if (!gPendingVKBOpen)
        return;

    gPendingVKBOpen = false;

#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    QWidget* focusWidget = qApp->focusWidget();

    if (focusWidget) {
        QInputContext *inputContext = qApp->inputContext();
        if (!inputContext) {
            NS_WARNING("Requesting SIP: but no input context");
            return;
        }

        QEvent request(QEvent::RequestSoftwareInputPanel);
        inputContext->filterEvent(&request);
        focusWidget->setAttribute(Qt::WA_InputMethodEnabled, true);
        inputContext->setFocusWidget(focusWidget);
        gKeyboardOpen = true;
        gFailedOpenKeyboard = false;
    }
    else
    {
        
        gFailedOpenKeyboard = true;
    }
#else
    LOG(("VKB not supported in Qt < 4.6\n"));
#endif
}

void MozQWidget::hideVKB()
{
    if (gPendingVKBOpen) {
        
        gPendingVKBOpen = false;
    }

    if (!gKeyboardOpen) {
        return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    QInputContext *inputContext = qApp->inputContext();
    if (!inputContext) {
        NS_WARNING("Closing SIP: but no input context");
        return;
    }

    QEvent request(QEvent::CloseSoftwareInputPanel);
    inputContext->filterEvent(&request);
    inputContext->reset();
    gKeyboardOpen = false;
#else
    LOG(("VKB not supported in Qt < 4.6\n"));
#endif
}

bool MozQWidget::isVKBOpen()
{
    return gKeyboardOpen;
}
