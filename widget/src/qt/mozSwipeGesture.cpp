



































#include "mozSwipeGesture.h"
#include <QTouchEvent>
#include <prtypes.h>
#include <nsIDOMSimpleGestureEvent.h>
#include <math.h>


static const float TRIGGER_DISTANCE = 0.3;



const int MAX_FINGER_DISTANCE = 250;


const int FINGER_DISTANCE_MISTAKE = 50;

QGesture*
MozSwipeGestureRecognizer::create(QObject* target)
{
    return new MozSwipeGesture();
}

QGestureRecognizer::Result
MozSwipeGestureRecognizer::recognize(QGesture* aState,
                                     QObject* aWatched,
                                     QEvent* aEvent)
{
    const QTouchEvent* ev = static_cast<const QTouchEvent *>(aEvent);
    MozSwipeGesture* swipe = static_cast<MozSwipeGesture *>(aState);

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;

    switch (aEvent->type()) {
        case QEvent::TouchBegin:
            swipe->mSwipeState = MozSwipeGesture::NOT_STARTED;
            result = QGestureRecognizer::MayBeGesture;
            break;

        case QEvent::TouchEnd:
            if (swipe->state() != Qt::NoGesture &&
                swipe->mSwipeState == MozSwipeGesture::TRIGGERED) {
                result = QGestureRecognizer::FinishGesture;
            }
            else {
                result = QGestureRecognizer::CancelGesture;
            }
            break;

        case QEvent::TouchUpdate:
            
            if (swipe->mSwipeState > MozSwipeGesture::STARTED) {
                break;
            }

            if (ev->touchPoints().count() > 2) {
                swipe->mSwipeState = MozSwipeGesture::CANCELLED;
                result = QGestureRecognizer::CancelGesture;
                break;
            }


            if (ev->touchPoints().count() == 2) {
               swipe->mSwipeState = MozSwipeGesture::STARTED;
               QList <QTouchEvent::TouchPoint> touchPoints = ev->touchPoints();
               if (!swipe->Update(touchPoints[0], touchPoints[1])) {
                   result = QGestureRecognizer::CancelGesture;
                   swipe->mSwipeState = MozSwipeGesture::CANCELLED;
               }
               if (swipe->mSwipeState == MozSwipeGesture::TRIGGERED) {
                   result = QGestureRecognizer::TriggerGesture;
               }
               else {
                   result = QGestureRecognizer::MayBeGesture;
               }
           }
           break;

        default:
            result = QGestureRecognizer::Ignore;
    }

    return result;
}

void
MozSwipeGestureRecognizer::reset(QGesture* aState)
{
    MozSwipeGesture* swipe = static_cast<MozSwipeGesture *>(aState);
    swipe->mHorizontalDirection = 0;
    swipe->mVerticalDirection = 0;
    QGestureRecognizer::reset(aState);
}

MozSwipeGesture::MozSwipeGesture()
  : mHorizontalDirection(0)
  , mVerticalDirection(0)
  , mSwipeState(MozSwipeGesture::NOT_STARTED)
{
}

int MozSwipeGesture::Direction()
{
    return mHorizontalDirection | mVerticalDirection;
}

bool
MozSwipeGesture::Update(const QTouchEvent::TouchPoint& aFirstPoint,
                        const QTouchEvent::TouchPoint& aSecondPoint)
{
    
    QPointF distance = aFirstPoint.pos() - aSecondPoint.pos();
    if (distance.manhattanLength() > MAX_FINGER_DISTANCE) {
        return false;
    }

    QPointF startPositions[]   = {aFirstPoint.startNormalizedPos(),
                                  aSecondPoint.startNormalizedPos()
                                 };
    QPointF currentPositions[] = {aFirstPoint.normalizedPos(),
                                  aSecondPoint.normalizedPos()
                                 };

    float xDistance = fabs(currentPositions[0].x()-startPositions[0].x());
    float yDistance = fabs(currentPositions[0].y()-startPositions[0].y());

    QPointF startFingerDistance = aFirstPoint.startPos()-aSecondPoint.startPos();
    QPointF fingerDistance = aFirstPoint.pos()-aSecondPoint.pos();

    
    if ((startFingerDistance-fingerDistance).manhattanLength()
         > FINGER_DISTANCE_MISTAKE) {
        return false;
    }

    mVerticalDirection = nsIDOMSimpleGestureEvent::DIRECTION_UP;
    if (currentPositions[0].y() > startPositions[0].y()) {
        mVerticalDirection = nsIDOMSimpleGestureEvent::DIRECTION_DOWN;
    }

    mHorizontalDirection = nsIDOMSimpleGestureEvent::DIRECTION_LEFT;
    if (currentPositions[0].x() > startPositions[0].y()) {
        mHorizontalDirection = nsIDOMSimpleGestureEvent::DIRECTION_RIGHT;
    }

    if (xDistance > TRIGGER_DISTANCE) {
        if (yDistance < TRIGGER_DISTANCE/2) {
            mVerticalDirection = 0;
        }
        mSwipeState = TRIGGERED;
    }

    if (yDistance > TRIGGER_DISTANCE) {
        if (xDistance < TRIGGER_DISTANCE/2) {
            mHorizontalDirection = 0;
        }
        mSwipeState = TRIGGERED;
    }

    
    QPointF hotspot = aFirstPoint.scenePos() + aSecondPoint.scenePos();
    hotspot /= 2;
    setHotSpot(hotspot);
    return true;
}
