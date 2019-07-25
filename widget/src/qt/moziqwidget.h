




































#ifndef MOZIQWIDGET_H
#define MOZIQWIDGET_H

#include <QtGui/QApplication>
#include <QtGui/QGraphicsWidget>
#include <QtGui/QGraphicsView>

#include "nsIWidget.h"

#ifdef MOZ_ENABLE_MEEGOTOUCH
#include <MSceneWindow>
#include <MInputMethodState>
#include <QtGui/QGraphicsSceneResizeEvent>
#include <QTimer>
#endif

class nsWindow;

class IMozQWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    


    virtual void setModal(bool) {}
    virtual bool SetCursor(nsCursor aCursor) { return false; }
    virtual void dropReceiver() { };
    virtual nsWindow* getReceiver() { return NULL; };

    virtual void activate() {}
    virtual void deactivate() {}

    


    virtual bool isVKBOpen() { return false; }

    virtual void NotifyVKB(const QRect& rect) {}
    virtual void SwitchToForeground() {}
    virtual void SwitchToBackground() {}
};

class MozQGraphicsViewEvents
{
public:

    MozQGraphicsViewEvents(QGraphicsView* aView)
     : mView(aView)
    { }

    void handleEvent(QEvent* aEvent, IMozQWidget* aTopLevel)
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

    void handleResizeEvent(QResizeEvent* aEvent, IMozQWidget* aTopLevel)
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

    bool handleCloseEvent(QCloseEvent* aEvent, IMozQWidget* aTopLevel)
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
    MozQGraphicsView(QWidget * aParent = nsnull)
     : QGraphicsView (new QGraphicsScene(), aParent)
     , mEventHandler(this)
     , mTopLevelWidget(NULL)
    {
        setMouseTracking(true);
        setFrameShape(QFrame::NoFrame);
    }

    void SetTopLevel(IMozQWidget* aTopLevel, QWidget* aParent)
    {
        scene()->addItem(aTopLevel);
        mTopLevelWidget = aTopLevel;
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
    IMozQWidget* mTopLevelWidget;
};

#ifdef MOZ_ENABLE_MEEGOTOUCH
class MozMSceneWindow : public MSceneWindow
{
    Q_OBJECT
public:
    MozMSceneWindow(IMozQWidget* aTopLevel)
     : MSceneWindow(aTopLevel->parentItem())
     , mTopLevelWidget(aTopLevel)
    {
        MInputMethodState* inputMethodState = MInputMethodState::instance();
        if (inputMethodState) {
            connect(inputMethodState, SIGNAL(inputMethodAreaChanged(const QRect&)),
                    this, SLOT(VisibleScreenAreaChanged(const QRect&)));
        }
    }

    void SetTopLevel(IMozQWidget* aTopLevel)
    {
        mTopLevelWidget = aTopLevel;
        mTopLevelWidget->setParentItem(this);
        mTopLevelWidget->installEventFilter(this);
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
            mTopLevelWidget->NotifyVKB(rect);
        }
    }

private:
    IMozQWidget* mTopLevelWidget;
    QSizeF mCurrentSize;
};






class MozMGraphicsView : public MWindow
{
    Q_OBJECT
public:
    MozMGraphicsView(QWidget* aParent = nsnull)
     : MWindow(aParent)
     , mEventHandler(this)
     , mTopLevelWidget(NULL)
     , mSceneWin(NULL)
    {
        QObject::connect(this, SIGNAL(switcherEntered()), this, SLOT(onSwitcherEntered()));
        QObject::connect(this, SIGNAL(switcherExited()), this, SLOT(onSwitcherExited()));
        setFrameShape(QFrame::NoFrame);
    }

    void SetTopLevel(IMozQWidget* aTopLevel, QWidget* aParent)
    {
        if (!mSceneWin) {
            mSceneWin = new MozMSceneWindow(aTopLevel);
            mSceneWin->appear(this);
        }
        mSceneWin->SetTopLevel(aTopLevel);
        mTopLevelWidget = aTopLevel;
    }

public Q_SLOTS:
    void onSwitcherEntered() {
        if (mTopLevelWidget) {
            mTopLevelWidget->SwitchToBackground();
        }
    }
    void onSwitcherExited() {
        if (mTopLevelWidget) {
            mTopLevelWidget->SwitchToForeground();
        }
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
    IMozQWidget* mTopLevelWidget;
    MozMSceneWindow* mSceneWin;
};

#endif 
#endif
