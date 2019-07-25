







































#ifndef MOZSWIPEGESTURE_H
#define MOZSWIPEGESTURE_H

#include <QGestureRecognizer>
#include <QGesture>
#include <QTouchEvent>

class MozSwipeGestureRecognizer: public QGestureRecognizer
{
public:
    virtual QGesture* create(QObject* aTarget);
    virtual QGestureRecognizer::Result recognize(QGesture* aState,
                                                 QObject* aWatched,
                                                 QEvent* aEvent);
    virtual void reset(QGesture* aState);
};

class MozSwipeGesture: public QGesture
{
public:
    int Direction();

private:
    enum SwipeState {
        NOT_STARTED = 0,
        STARTED,
        CANCELLED,
        TRIGGERED
    };

    MozSwipeGesture();

    bool Update(const QTouchEvent::TouchPoint& aFirstPoint,
                const QTouchEvent::TouchPoint& aSecondPoint);

    int mHorizontalDirection;
    int mVerticalDirection;
    SwipeState mSwipeState;
    friend class MozSwipeGestureRecognizer;
};

#endif
