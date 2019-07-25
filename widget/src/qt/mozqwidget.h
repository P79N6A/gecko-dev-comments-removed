#ifndef MOZQWIDGET_H
#define MOZQWIDGET_H

#include <QtGui/QApplication>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsWidget>

#include "nsIWidget.h"
#include "prenv.h"

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

    


    void showVKB();
    void hideVKB();
    bool isVKBOpen();

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

    virtual void wheelEvent(QGraphicsSceneWheelEvent* aEvent);
    virtual void paint(QPainter* aPainter, const QStyleOptionGraphicsItem* aOption, QWidget* aWidget = 0);
    virtual void resizeEvent(QGraphicsSceneResizeEvent* aEvent);
    virtual void closeEvent(QCloseEvent* aEvent);
    virtual void hideEvent(QHideEvent* aEvent);
    virtual void showEvent(QShowEvent* aEvent);
    virtual bool event(QEvent* aEvent);

    bool SetCursor(const QPixmap& aPixmap, int, int);

private:
    nsWindow *mReceiver;
};

class MozQGraphicsViewEvents
{
public:

    MozQGraphicsViewEvents(QGraphicsView* aView, MozQWidget* aTopLevel)
     : mTopLevelWidget(aTopLevel)
     , mView(aView)
    { }

    void handleEvent(QEvent* aEvent)
    {
        if (!aEvent)
            return;
        if (aEvent->type() == QEvent::WindowActivate) {
            if (mTopLevelWidget)
                mTopLevelWidget->activate();
        }

        if (aEvent->type() == QEvent::WindowDeactivate) {
            if (mTopLevelWidget)
                mTopLevelWidget->deactivate();
        }
    }

    void handleResizeEvent(QResizeEvent* aEvent)
    {
        if (!aEvent)
            return;
        if (mTopLevelWidget) {
            
            mTopLevelWidget->setGeometry(0.0, 0.0,
                static_cast<qreal>(aEvent->size().width()),
                static_cast<qreal>(aEvent->size().height()));
            
            
            if (mView)
                mView->setSceneRect(mView->viewport()->rect());
        }
    }

    bool handleCloseEvent(QCloseEvent* aEvent)
    {
        if (!aEvent)
            return false;
        if (mTopLevelWidget) {
            
            
            QApplication::postEvent(mTopLevelWidget, new QCloseEvent(*aEvent));
            aEvent->ignore();
            return true;
        }

        return false;
    }

private:
    MozQWidget* mTopLevelWidget;
    QGraphicsView* mView;
};






class MozQGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MozQGraphicsView(MozQWidget* aTopLevel, QWidget * aParent = nsnull)
     : QGraphicsView (new QGraphicsScene(), aParent)
     , mEventHandler(this, aTopLevel)
     , mTopLevelWidget(aTopLevel)
    {
        scene()->addItem(aTopLevel);
    }

protected:

    virtual bool event(QEvent* aEvent)
    {
        mEventHandler.handleEvent(aEvent);
        return QGraphicsView::event(aEvent);
    }

    virtual void resizeEvent(QResizeEvent* aEvent)
    {
        mEventHandler.handleResizeEvent(aEvent);
        QGraphicsView::resizeEvent(aEvent);
    }

    virtual void closeEvent (QCloseEvent* aEvent)
    {
        if (!mEventHandler.handleCloseEvent(aEvent))
            QGraphicsView::closeEvent(aEvent);
    }

private:
    MozQGraphicsViewEvents mEventHandler;
    MozQWidget* mTopLevelWidget;
};

#endif
