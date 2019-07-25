




































#ifndef MOZQWIDGET_H
#define MOZQWIDGET_H

#include <QtGui/QApplication>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsWidget>

#include "nsIWidget.h"
#include "prenv.h"

#include "nsIObserverService.h"
#include "mozilla/Services.h"

#ifdef MOZ_ENABLE_QTMOBILITY
#include "mozqorientationsensorfilter.h"
#endif

#ifdef MOZ_ENABLE_MEEGOTOUCH
#include <QtGui/QGraphicsSceneResizeEvent>
#include <MSceneWindow>
#include <MScene>
#include <QTimer>
#include <MInputMethodState>
#include <MApplication>
#endif

class QEvent;
class QPixmap;
class QWidget;

class nsWindow;

class MozQWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    MozQWidget(nsWindow* aReceiver, QGraphicsItem *aParent);

    ~MozQWidget();

    


    void setModal(bool);
    bool SetCursor(nsCursor aCursor);
    void dropReceiver() { mReceiver = 0x0; };
    nsWindow* getReceiver() { return mReceiver; };

    void activate();
    void deactivate();

    QVariant inputMethodQuery(Qt::InputMethodQuery aQuery) const;

    


    void requestVKB(int aTimeout);
    void hideVKB();
    bool isVKBOpen();

public slots:
    void showVKB();

#ifdef MOZ_ENABLE_QTMOBILITY
    void orientationChanged();
#endif

protected:
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* aEvent);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent* aEvent);
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent* aEvent);
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent* aEvent);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* aEvent);
    virtual void focusInEvent(QFocusEvent* aEvent);
    virtual void focusOutEvent(QFocusEvent* aEvent);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* aEvent);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* aEvent);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* aEvent);
    virtual void keyPressEvent(QKeyEvent* aEvent);
    virtual void keyReleaseEvent(QKeyEvent* aEvent);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* aEvent);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* aEvent);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* aEvent);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* aEvent);
    virtual void inputMethodEvent(QInputMethodEvent* aEvent);

    virtual void wheelEvent(QGraphicsSceneWheelEvent* aEvent);
    virtual void paint(QPainter* aPainter, const QStyleOptionGraphicsItem* aOption, QWidget* aWidget = 0);
    virtual void resizeEvent(QGraphicsSceneResizeEvent* aEvent);
    virtual void closeEvent(QCloseEvent* aEvent);
    virtual void hideEvent(QHideEvent* aEvent);
    virtual void showEvent(QShowEvent* aEvent);
    virtual bool event(QEvent* aEvent);

    bool SetCursor(const QPixmap& aPixmap, int, int);

private:
    void sendPressReleaseKeyEvent(int key, const QChar* letter = 0, bool autorep = false, ushort count = 1);
    nsWindow *mReceiver;
};

class MozQGraphicsViewEvents
{
public:

    MozQGraphicsViewEvents(QGraphicsView* aView)
     : mView(aView)
    { }

    void handleEvent(QEvent* aEvent, MozQWidget* aTopLevel)
    {
        if (!aEvent)
            return;
        if (aEvent->type() == QEvent::WindowActivate) {
            if (aTopLevel)
                aTopLevel->activate();
        }

        if (aEvent->type() == QEvent::WindowDeactivate) {
            if (aTopLevel)
                aTopLevel->deactivate();
        }
    }

    void handleResizeEvent(QResizeEvent* aEvent, MozQWidget* aTopLevel)
    {
        if (!aEvent)
            return;
        if (aTopLevel) {
            
            aTopLevel->setGeometry(0.0, 0.0,
                static_cast<qreal>(aEvent->size().width()),
                static_cast<qreal>(aEvent->size().height()));
            
            
            if (mView)
                mView->setSceneRect(mView->viewport()->rect());
        }
    }

    bool handleCloseEvent(QCloseEvent* aEvent, MozQWidget* aTopLevel)
    {
        if (!aEvent)
            return false;
        if (aTopLevel) {
            
            
            QApplication::postEvent(aTopLevel, new QCloseEvent(*aEvent));
            aEvent->ignore();
            return true;
        }

        return false;
    }

private:
    QGraphicsView* mView;
};






class MozQGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MozQGraphicsView(MozQWidget* aTopLevel, QWidget * aParent = nsnull)
     : QGraphicsView (new QGraphicsScene(), aParent)
     , mEventHandler(this)
     , mTopLevelWidget(aTopLevel)
    {
        scene()->addItem(aTopLevel);
        setMouseTracking(true);
        setFrameShape(QFrame::NoFrame);
    }

protected:

    virtual bool event(QEvent* aEvent)
    {
        mEventHandler.handleEvent(aEvent, mTopLevelWidget);
        return QGraphicsView::event(aEvent);
    }

    virtual void resizeEvent(QResizeEvent* aEvent)
    {
        mEventHandler.handleResizeEvent(aEvent, mTopLevelWidget);
        QGraphicsView::resizeEvent(aEvent);
    }

    virtual void closeEvent (QCloseEvent* aEvent)
    {
        if (!mEventHandler.handleCloseEvent(aEvent, mTopLevelWidget))
            QGraphicsView::closeEvent(aEvent);
    }

private:
    MozQGraphicsViewEvents mEventHandler;
    MozQWidget* mTopLevelWidget;
};

#ifdef MOZ_ENABLE_MEEGOTOUCH
class MozMSceneWindow : public MSceneWindow
{
    Q_OBJECT
public:
    MozMSceneWindow(MozQWidget* aTopLevel)
     : MSceneWindow(aTopLevel->parentItem())
     , mTopLevelWidget(aTopLevel)
    {
        mTopLevelWidget->setParentItem(this);
        mTopLevelWidget->installEventFilter(this);
        MInputMethodState* inputMethodState = MInputMethodState::instance();
        if (inputMethodState) {
            connect(inputMethodState, SIGNAL(inputMethodAreaChanged(const QRect&)),
                    this, SLOT(VisibleScreenAreaChanged(const QRect&)));
        }
    }

protected:
    virtual void resizeEvent(QGraphicsSceneResizeEvent* aEvent)
    {
        mCurrentSize = aEvent->newSize();
        MSceneWindow::resizeEvent(aEvent);
        CheckTopLevelSize();
    }

    virtual bool eventFilter(QObject* watched, QEvent* e)
    {
        if (e->type() == QEvent::GraphicsSceneResize ||
            e->type() == QEvent::GraphicsSceneMove) {

            
            QTimer::singleShot(0, this, SLOT(CheckTopLevelSize()));
        }

        return false;
    }

private slots:
    void CheckTopLevelSize()
    {
        if (mTopLevelWidget) {
            qreal xpos = 0;
            qreal ypos = 0;
            qreal width = mCurrentSize.width();
            qreal height = mCurrentSize.height();

            
            QRectF r = mTopLevelWidget->geometry();
            if (r != QRectF(xpos, ypos, width, height)) {
                mTopLevelWidget->setGeometry(xpos, ypos, width, height);
            }
        }
    }

    void VisibleScreenAreaChanged(const QRect& rect) {
        if (mTopLevelWidget) {
            QRect r = mTopLevelWidget->geometry().toRect();
            if (rect.height()) {
                r.setHeight(rect.height());
            }

            nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
            if (observerService) {
                QString rect = QString("{\"left\": %1, \"top\": %2, \"right\": %3, \"bottom\": %4}")
                                       .arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
                observerService->NotifyObservers(nsnull, "softkb-change", rect.utf16());
            }
        }
    }

private:
    MozQWidget* mTopLevelWidget;
    QSizeF mCurrentSize;
};






class MozMGraphicsView : public MWindow
{

public:
    MozMGraphicsView(MozQWidget* aTopLevel, QWidget* aParent = nsnull)
     : MWindow(aParent)
     , mEventHandler(this)
     , mTopLevelWidget(aTopLevel)
    {
        MozMSceneWindow* page = new MozMSceneWindow(aTopLevel);
        page->appear(this);
    }

protected:
    virtual bool event(QEvent* aEvent) {
        mEventHandler.handleEvent(aEvent, mTopLevelWidget);
        return MWindow::event(aEvent);
    }

    virtual void resizeEvent(QResizeEvent* aEvent)
    {
        setSceneRect(viewport()->rect());
        MWindow::resizeEvent(aEvent);
    }

    virtual void closeEvent (QCloseEvent* aEvent)
    {
        if (!mEventHandler.handleCloseEvent(aEvent, mTopLevelWidget)) {
            MWindow::closeEvent(aEvent);
        }
    }

private:
    MozQGraphicsViewEvents mEventHandler;
    MozQWidget* mTopLevelWidget;
};

#endif 
#endif
