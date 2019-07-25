




































package org.mozilla.fennec.ui;

import org.mozilla.fennec.gfx.IntPoint;
import org.mozilla.fennec.gfx.IntRect;
import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.LayerController;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import java.util.Timer;
import java.util.TimerTask;







public class PanZoomController {
    private LayerController mController;

    private static final float FRICTION = 0.97f;
    
    private static final int STOPPED_THRESHOLD = 4;
    
    private static final float SNAP_LIMIT = 0.75f;
    
    private static final float OVERSCROLL_DECEL_RATE = 0.04f;
    
    private static final int SNAP_TIME = 150;
    
    
    private static final int SUBDIVISION_COUNT = 1000;

    
    private static final int FLING_STATE_SCROLLING = 0;
    
    private static final int FLING_STATE_SNAPPING = 1;

    private boolean mTouchMoved, mStopped;
    private long mLastTimestamp;
    private Timer mFlingTimer;
    private Axis mX, mY;
    private float mInitialZoomSpan;
    
    private IntPoint mInitialZoomFocus;
    
    private boolean mTracking, mZooming;

    public PanZoomController(LayerController controller) {
        mController = controller;
        mX = new Axis(); mY = new Axis();
        mStopped = true;

        populatePositionAndLength();
    }

    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:   return onTouchStart(event);
        case MotionEvent.ACTION_MOVE:   return onTouchMove(event);
        case MotionEvent.ACTION_UP:     return onTouchEnd(event);
        default:                        return false;
        }
    }

    public void geometryChanged() {
        populatePositionAndLength();
    }

    





    public boolean getRedrawHint() {
        return mStopped && !mTracking && !mZooming;
    }

    



    private boolean onTouchStart(MotionEvent event) {
        

        if (event.getPointerCount() > 1 || mZooming) {
            mZooming = true;
            mTouchMoved = false;
            mStopped = true;
            return false;
        }

        mX.touchPos = event.getX(0); mY.touchPos = event.getY(0);
        mTouchMoved = mStopped = false;
        mTracking = true;
        
        return true;
    }

    private boolean onTouchMove(MotionEvent event) {
        if (event.getPointerCount() > 1 || mZooming) {
            mZooming = true;
            mTouchMoved = false;
            mStopped = true;
            return false;
        }

        if (!mTouchMoved)
            mLastTimestamp = System.currentTimeMillis();
        mTouchMoved = true;

        
        track(event, System.currentTimeMillis());

        if (mFlingTimer != null) {
            mFlingTimer.cancel();
            mFlingTimer = null;
        }

        return true;
    }

    private boolean onTouchEnd(MotionEvent event) {
        if (mZooming)
            mZooming = false;

        mTracking = false;
        fling(System.currentTimeMillis());
        return true;
    }

    private void track(MotionEvent event, long timestamp) {
        long timeStep = timestamp - mLastTimestamp;
        mLastTimestamp = timestamp;

        mX.velocity = mX.touchPos - event.getX(0); mY.velocity = mY.touchPos - event.getY(0);
        mX.touchPos = event.getX(0); mY.touchPos = event.getY(0);

        float absVelocity = (float)Math.sqrt(mX.velocity * mX.velocity +
                                             mY.velocity * mY.velocity);
        mStopped = absVelocity < STOPPED_THRESHOLD;

        mX.applyEdgeResistance(); mX.displace();
        mY.applyEdgeResistance(); mY.displace();
        updatePosition();
    }

    private void fling(long timestamp) {
        long timeStep = timestamp - mLastTimestamp;
        mLastTimestamp = timestamp;

        if (mStopped)
            mX.velocity = mY.velocity = 0.0f;

        mX.displace(); mY.displace();

        if (mFlingTimer != null)
            mFlingTimer.cancel();

        mX.startFling(); mY.startFling();

        mFlingTimer = new Timer();
        mFlingTimer.scheduleAtFixedRate(new TimerTask() {
            public void run() { mController.post(new FlingRunnable()); }
        }, 0, 1000L/60L);
    }

    private void updatePosition() {
        Log.e("Fennec", "moving to " + mX.viewportPos + ", " + mY.viewportPos);

        mController.scrollTo(mX.viewportPos, mY.viewportPos);
        mController.notifyLayerClientOfGeometryChange();
    }

    
    private void populatePositionAndLength() {
        IntSize pageSize = mController.getPageSize();
        IntRect visibleRect = mController.getVisibleRect();
        IntSize screenSize = mController.getScreenSize();

        Log.e("Fennec", "page size: " + pageSize + " visible rect: " + visibleRect +
              "screen size: " + screenSize);

        mX.setPageLength(pageSize.width);
        mX.viewportPos = visibleRect.x;
        mX.setViewportLength(visibleRect.width);

        mY.setPageLength(pageSize.height);
        mY.viewportPos = visibleRect.y;
        mY.setViewportLength(visibleRect.height);
    }

    
    private class FlingRunnable implements Runnable {
        public void run() {
            populatePositionAndLength();
            mX.advanceFling(); mY.advanceFling();

            
            
            boolean waitingToSnapX = mX.getFlingState() == Axis.FlingStates.WAITING_TO_SNAP;
            boolean waitingToSnapY = mY.getFlingState() == Axis.FlingStates.WAITING_TO_SNAP;
            if (mX.getOverscroll() != Axis.Overscroll.NONE &&
                    mY.getOverscroll() != Axis.Overscroll.NONE) {
                if (waitingToSnapX && waitingToSnapY) {
                    mX.startSnap(); mY.startSnap();
                }
            } else {
                if (waitingToSnapX)
                    mX.startSnap();
                if (waitingToSnapY)
                    mY.startSnap();
            }

            mX.displace(); mY.displace();
            updatePosition();

            if (mX.getFlingState() == Axis.FlingStates.STOPPED &&
                    mY.getFlingState() == Axis.FlingStates.STOPPED) {
                stop();
            }
        }

        private void stop() {
            mStopped = true;
            if (mFlingTimer != null) {
                mFlingTimer.cancel();
                mFlingTimer = null;
            }
        }
    }

    private float computeElasticity(float excess, float viewportLength) {
        return 1.0f - excess / (viewportLength * SNAP_LIMIT);
    }

    
    private static class Axis {
        public enum FlingStates {
            STOPPED,
            SCROLLING,
            WAITING_TO_SNAP,
            SNAPPING,
        }

        public enum Overscroll {
            NONE,
            MINUS,      
            PLUS,       
        }

        public float touchPos;                  
        public float velocity;                  

        private FlingStates mFlingState;        
        private EaseOutAnimation mSnapAnim;     

        
        public int viewportPos;
        private int mViewportLength;
        private int mScreenLength;
        private int mPageLength;

        public FlingStates getFlingState() { return mFlingState; }

        public void setViewportLength(int viewportLength) { mViewportLength = viewportLength; }
        public void setScreenLength(int screenLength) { mScreenLength = screenLength; }
        public void setPageLength(int pageLength) { mPageLength = pageLength; }

        private int getViewportEnd() { return viewportPos + mViewportLength; }

        public Overscroll getOverscroll() {
            if (viewportPos < 0)
                return Overscroll.MINUS;
            if (viewportPos > mPageLength - mViewportLength)
                return Overscroll.PLUS;
            return Overscroll.NONE;
        }

        
        
        private int getExcess() {
            switch (getOverscroll()) {
            case MINUS:     return -viewportPos;
            case PLUS:      return getViewportEnd() - mPageLength;
            default:        return 0;
            }
        }

        
        public void applyEdgeResistance() {
            int excess = getExcess();
            if (excess > 0)
                velocity *= SNAP_LIMIT - (float)excess / mViewportLength;
        }

        public void startFling() { mFlingState = FlingStates.SCROLLING; }

        
        public void advanceFling() {
            switch (mFlingState) {
            case SCROLLING:
                scroll();
                return;
            case WAITING_TO_SNAP:
                
                return;
            case SNAPPING:
                snap();
                return;
            }
        }

        
        private void scroll() {
            
            Overscroll overscroll = getOverscroll();
            if (overscroll == Overscroll.NONE) {
                velocity *= FRICTION;
                if (Math.abs(velocity) < 0.1f) {
                    velocity = 0.0f;
                    mFlingState = FlingStates.STOPPED;
                }
                return;
            }

            
            float elasticity = 1.0f - getExcess() / (mViewportLength * SNAP_LIMIT);
            if (overscroll == Overscroll.MINUS)
                velocity = Math.min((velocity + OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);
            else
                velocity = Math.max((velocity - OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);

            if (velocity == 0.0f)
                mFlingState = FlingStates.WAITING_TO_SNAP;
        }

        
        public void startSnap() {
            switch (getOverscroll()) {
            case MINUS:
                mSnapAnim = new EaseOutAnimation(viewportPos, 0.0f);
                break;
            case PLUS:
                mSnapAnim = new EaseOutAnimation(viewportPos, mPageLength - mViewportLength);
                break;
            default:
                throw new RuntimeException("Not overscrolled at startSnap()");
            }

            mFlingState = FlingStates.SNAPPING;
        }

        
        private void snap() {
            mSnapAnim.advance();
            viewportPos = (int)Math.round(mSnapAnim.getPosition());

            if (mSnapAnim.getFinished()) {
                mSnapAnim = null;
                mFlingState = FlingStates.STOPPED;
            }
        }

        
        public void displace() { viewportPos += velocity; }
    }

    private static class EaseOutAnimation {
        private float[] mFrames;
        private float mPosition;
        private float mOrigin;
        private float mDest;
        private long mTimestamp;
        private boolean mFinished;

        public EaseOutAnimation(float position, float dest) {
            mPosition = mOrigin = position;
            mDest = dest;
            mFrames = new float[SNAP_TIME];
            mTimestamp = System.currentTimeMillis();
            mFinished = false;
            plot(position, dest, mFrames);
        }

        public float getPosition() { return mPosition; }
        public boolean getFinished() { return mFinished; }

        private void advance() {
            int frame = (int)(System.currentTimeMillis() - mTimestamp);
            if (frame >= SNAP_TIME) {
                mPosition = mDest;
                mFinished = true;
                return;
            }

            mPosition = mFrames[frame];
        }

        private static void plot(float from, float to, float[] frames) {
            int nextX = 0;
            for (int i = 0; i < SUBDIVISION_COUNT; i++) {
                float t = (float)i / (float)SUBDIVISION_COUNT;
                float xPos = (3.0f*t*t - 2.0f*t*t*t) * (float)frames.length;
                if ((int)xPos < nextX)
                    continue;

                int oldX = nextX;
                nextX = (int)xPos;

                float yPos = 1.74f*t*t - 0.74f*t*t*t;
                float framePos = from + (to - from) * yPos;

                while (oldX < nextX)
                    frames[oldX++] = framePos;

                if (nextX >= frames.length)
                    break;
            }

            
            while (nextX < frames.length) {
                frames[nextX] = frames[nextX - 1];
                nextX++;
            }
        }
    }

    


    public boolean onScale(ScaleGestureDetector detector) {
        float newZoom = detector.getCurrentSpan() / mInitialZoomSpan;

        IntSize screenSize = mController.getScreenSize();
        float x = mInitialZoomFocus.x - (detector.getFocusX() / newZoom);
        float y = mInitialZoomFocus.y - (detector.getFocusY() / newZoom);
        float width = screenSize.width / newZoom;
        float height = screenSize.height / newZoom;
        mController.setVisibleRect((int)Math.round(x), (int)Math.round(y),
                                   (int)Math.round(width), (int)Math.round(height));
        mController.notifyLayerClientOfGeometryChange();
        return true;
    }

    public boolean onScaleBegin(ScaleGestureDetector detector) {
        IntRect initialZoomRect = (IntRect)mController.getVisibleRect().clone();
        float initialZoom = mController.getZoomFactor();

        mInitialZoomFocus = new IntPoint((int)Math.round(initialZoomRect.x + (detector.getFocusX() / initialZoom)),
                                         (int)Math.round(initialZoomRect.y + (detector.getFocusY() / initialZoom)));
        mInitialZoomSpan = detector.getCurrentSpan() / initialZoom;
        return true;
    }

    public void onScaleEnd(ScaleGestureDetector detector) {
        
    }
}

