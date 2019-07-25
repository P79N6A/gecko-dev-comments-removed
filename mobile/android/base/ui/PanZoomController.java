




































package org.mozilla.gecko.ui;

import org.json.JSONObject;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import java.util.Timer;
import java.util.TimerTask;







public class PanZoomController
    extends GestureDetector.SimpleOnGestureListener
    implements ScaleGestureDetector.OnScaleGestureListener
{
    private static final String LOGTAG = "GeckoPanZoomController";

    private LayerController mController;

    private static final float FRICTION = 0.85f;
    
    private static final float STOPPED_THRESHOLD = 4.0f;
    
    private static final float SNAP_LIMIT = 0.75f;
    
    private static final float OVERSCROLL_DECEL_RATE = 0.04f;
    
    private static final int SNAP_TIME = 240;
    
    
    private static final int SUBDIVISION_COUNT = 1000;
    
    
    private static final float PAN_THRESHOLD = 4.0f;
    
    private static final double AXIS_LOCK_ANGLE = Math.PI / 6.0; 

    private Timer mFlingTimer;
    private Axis mX, mY;
    
    private float mInitialZoomSpan;
    
    private PointF mInitialZoomFocus;

    private enum PanZoomState {
        NOTHING,        
        FLING,          
        TOUCHING,       
        PANNING_LOCKED, 
        PANNING,        
        PANNING_HOLD,   

        PANNING_HOLD_LOCKED, 
        PINCHING,       
    }

    private PanZoomState mState;

    public PanZoomController(LayerController controller) {
        mController = controller;
        mX = new Axis(); mY = new Axis();
        mState = PanZoomState.NOTHING;

        populatePositionAndLength();
    }

    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getActionMasked()) {
        case MotionEvent.ACTION_DOWN:   return onTouchStart(event);
        case MotionEvent.ACTION_MOVE:   return onTouchMove(event);
        case MotionEvent.ACTION_UP:     return onTouchEnd(event);
        case MotionEvent.ACTION_CANCEL: return onTouchCancel(event);
        default:                        return false;
        }
    }

    public void geometryChanged() {
        populatePositionAndLength();
    }

    



    private boolean onTouchStart(MotionEvent event) {
        
        
        if (mFlingTimer != null) {
            mFlingTimer.cancel();
            mFlingTimer = null;
        }

        switch (mState) {
        case FLING:
        case NOTHING:
            mState = PanZoomState.TOUCHING;
            mX.firstTouchPos = mX.touchPos = event.getX(0);
            mY.firstTouchPos = mY.touchPos = event.getY(0);
            return false;
        case TOUCHING:
        case PANNING:
        case PANNING_LOCKED:
        case PANNING_HOLD:
        case PANNING_HOLD_LOCKED:
        case PINCHING:
            mState = PanZoomState.PINCHING;
            return false;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in onTouchStart");
        return false;
    }

    private boolean onTouchMove(MotionEvent event) {
        switch (mState) {
        case NOTHING:
        case FLING:
            
            Log.e(LOGTAG, "Received impossible touch move while in " + mState);
            return false;
        case TOUCHING:
            if (panDistance(event) < PAN_THRESHOLD)
                return false;
            
        case PANNING_HOLD_LOCKED:
            mState = PanZoomState.PANNING_LOCKED;
            
        case PANNING_LOCKED:
            track(event);
            return true;
        case PANNING_HOLD:
            mState = PanZoomState.PANNING;
            
        case PANNING:
            track(event);
            return true;
        case PINCHING:
            
            return false;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in onTouchMove");
        return false;
    }

    private boolean onTouchEnd(MotionEvent event) {
        switch (mState) {
        case NOTHING:
        case FLING:
            
            Log.e(LOGTAG, "Received impossible touch end while in " + mState);
            return false;
        case TOUCHING:
            mState = PanZoomState.NOTHING;
            
            
            
            fling();
            return false;
        case PANNING:
        case PANNING_LOCKED:
        case PANNING_HOLD:
        case PANNING_HOLD_LOCKED:
            mState = PanZoomState.FLING;
            fling();
            return true;
        case PINCHING:
            int points = event.getPointerCount();
            if (points == 1) {
                
                mState = PanZoomState.NOTHING;
            } else if (points == 2) {
                int pointRemovedIndex = event.getActionIndex();
                int pointRemainingIndex = 1 - pointRemovedIndex; 
                mState = PanZoomState.TOUCHING;
                mX.firstTouchPos = mX.touchPos = event.getX(pointRemainingIndex);
                mX.firstTouchPos = mY.touchPos = event.getY(pointRemainingIndex);
            } else {
                
            }
            return true;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in onTouchEnd");
        return false;
    }

    private boolean onTouchCancel(MotionEvent event) {
        mState = PanZoomState.NOTHING;
        
        fling();
        return false;
    }

    private float panDistance(MotionEvent move) {
        float dx = mX.firstTouchPos - move.getX(0);
        float dy = mY.firstTouchPos - move.getY(0);
        return (float)Math.sqrt(dx * dx + dy * dy);
    }

    private void track(MotionEvent event) {
        float x = event.getX(0);
        float y = event.getY(0);

        if (mState == PanZoomState.PANNING_LOCKED) {
            
            double angle = Math.atan2(y - mY.firstTouchPos, x - mX.firstTouchPos); 
            angle = Math.abs(angle); 
            if (angle < AXIS_LOCK_ANGLE || angle > (Math.PI - AXIS_LOCK_ANGLE)) {
                
                y = mY.firstTouchPos;
            } else if (Math.abs(angle - (Math.PI / 2)) < AXIS_LOCK_ANGLE) {
                
                x = mX.firstTouchPos;
            } else {
                
                mState = PanZoomState.PANNING;
                angle = Math.abs(angle - (Math.PI / 2));  
                Log.i(LOGTAG, "Breaking axis lock at " + (angle * 180.0 / Math.PI) + " degrees");
            }
        }

        float zoomFactor = mController.getZoomFactor();
        mX.velocity = (mX.touchPos - x) / zoomFactor;
        mY.velocity = (mY.touchPos - y) / zoomFactor;
        mX.touchPos = x;
        mY.touchPos = y;

        if (stopped()) {
            if (mState == PanZoomState.PANNING) {
                mState = PanZoomState.PANNING_HOLD;
            } else if (mState == PanZoomState.PANNING_LOCKED) {
                mState = PanZoomState.PANNING_HOLD_LOCKED;
            } else {
                
                Log.e(LOGTAG, "Impossible case " + mState + " when stopped in track");
                mState = PanZoomState.PANNING_HOLD_LOCKED;
            }
        }

        mX.applyEdgeResistance(); mX.displace();
        mY.applyEdgeResistance(); mY.displace();
        updatePosition();
    }

    private void fling() {
        if (mState != PanZoomState.FLING)
            mX.velocity = mY.velocity = 0.0f;

        mX.displace(); mY.displace();
        updatePosition();

        if (mFlingTimer != null)
            mFlingTimer.cancel();

        boolean stopped = stopped();
        mX.startFling(stopped); mY.startFling(stopped);

        mFlingTimer = new Timer();
        mFlingTimer.scheduleAtFixedRate(new TimerTask() {
            public void run() { mController.post(new FlingRunnable()); }
        }, 0, 1000L/60L);
    }

    private boolean stopped() {
        float absVelocity = (float)Math.sqrt(mX.velocity * mX.velocity +
                                             mY.velocity * mY.velocity);
        return absVelocity < STOPPED_THRESHOLD;
    }

    private void updatePosition() {
        mController.scrollTo(mX.viewportPos, mY.viewportPos);
        mController.notifyLayerClientOfGeometryChange();
    }

    
    private void populatePositionAndLength() {
        IntSize pageSize = mController.getPageSize();
        RectF visibleRect = mController.getVisibleRect();
        IntSize screenSize = mController.getScreenSize();

        mX.setPageLength(pageSize.width);
        mX.viewportPos = visibleRect.left;
        mX.setViewportLength(visibleRect.width());

        mY.setPageLength(pageSize.height);
        mY.viewportPos = visibleRect.top;
        mY.setViewportLength(visibleRect.height());
    }

    
    private class FlingRunnable implements Runnable {
        public void run() {
            populatePositionAndLength();
            mX.advanceFling(); mY.advanceFling();

            
            
            boolean waitingToSnapX = mX.getFlingState() == Axis.FlingStates.WAITING_TO_SNAP;
            boolean waitingToSnapY = mY.getFlingState() == Axis.FlingStates.WAITING_TO_SNAP;
            if ((mX.getOverscroll() == Axis.Overscroll.PLUS || mX.getOverscroll() == Axis.Overscroll.MINUS) &&
                (mY.getOverscroll() == Axis.Overscroll.PLUS || mY.getOverscroll() == Axis.Overscroll.MINUS))
            {
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
            mState = PanZoomState.NOTHING;
            if (mFlingTimer != null) {
                mFlingTimer.cancel();
                mFlingTimer = null;
            }
        }
    }

    private float computeElasticity(float excess, float viewportLength) {
        return 1.0f - excess / (viewportLength * SNAP_LIMIT);
    }

    private static boolean floatsApproxEqual(float a, float b) {
        
        return Math.abs(a - b) < 1e-6;
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
            BOTH,       
        }

        public float firstTouchPos;             
        public float touchPos;                  
        public float velocity;                  

        private FlingStates mFlingState;        
        private EaseOutAnimation mSnapAnim;     

        
        public float viewportPos;
        private float mViewportLength;
        private int mScreenLength;
        private int mPageLength;

        public FlingStates getFlingState() { return mFlingState; }

        public void setViewportLength(float viewportLength) { mViewportLength = viewportLength; }
        public void setScreenLength(int screenLength) { mScreenLength = screenLength; }
        public void setPageLength(int pageLength) { mPageLength = pageLength; }

        private float getViewportEnd() { return viewportPos + mViewportLength; }

        public Overscroll getOverscroll() {
            boolean minus = (viewportPos < 0.0f);
            boolean plus = (getViewportEnd() > mPageLength);
            if (minus && plus)
                return Overscroll.BOTH;
            else if (minus)
                return Overscroll.MINUS;
            else if (plus)
                return Overscroll.PLUS;
            else
                return Overscroll.NONE;
        }

        
        
        private float getExcess() {
            switch (getOverscroll()) {
            case MINUS:     return Math.min(-viewportPos, mPageLength - getViewportEnd());
            case PLUS:      return Math.min(viewportPos, getViewportEnd() - mPageLength);
            default:        return 0.0f;
            }
        }

        
        public void applyEdgeResistance() {
            float excess = getExcess();
            if (excess > 0.0f)
                velocity *= SNAP_LIMIT - excess / mViewportLength;
        }

        public void startFling(boolean stopped) {
            if (!stopped) {
                mFlingState = FlingStates.SCROLLING;
                return;
            }

            float excess = getExcess();
            if (floatsApproxEqual(excess, 0.0f))
                mFlingState = FlingStates.STOPPED;
            else
                mFlingState = FlingStates.WAITING_TO_SNAP;
        }

        
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
            
            float excess = getExcess();
            if (floatsApproxEqual(excess, 0.0f)) {
                velocity *= FRICTION;
                if (Math.abs(velocity) < 0.1f) {
                    velocity = 0.0f;
                    mFlingState = FlingStates.STOPPED;
                }
                return;
            }

            
            float elasticity = 1.0f - excess / (mViewportLength * SNAP_LIMIT);
            if (getOverscroll() == Overscroll.MINUS)
                velocity = Math.min((velocity + OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);
            else 
                velocity = Math.max((velocity - OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);

            if (Math.abs(velocity) < 0.3f) {
                velocity = 0.0f;
                mFlingState = FlingStates.WAITING_TO_SNAP;
            }
        }

        
        public void startSnap() {
            switch (getOverscroll()) {
            case MINUS:
                mSnapAnim = new EaseOutAnimation(viewportPos, viewportPos + getExcess());
                break;
            case PLUS:
                mSnapAnim = new EaseOutAnimation(viewportPos, viewportPos - getExcess());
                break;
            default:
                
                mFlingState = FlingStates.STOPPED;
                return;
            }

            mFlingState = FlingStates.SNAPPING;
        }

        
        private void snap() {
            mSnapAnim.advance();
            viewportPos = mSnapAnim.getPosition();

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

    


    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        mState = PanZoomState.PINCHING;
        float newZoom = detector.getCurrentSpan() / mInitialZoomSpan;

        IntSize screenSize = mController.getScreenSize();
        float x = mInitialZoomFocus.x - (detector.getFocusX() / newZoom);
        float y = mInitialZoomFocus.y - (detector.getFocusY() / newZoom);
        float width = screenSize.width / newZoom;
        float height = screenSize.height / newZoom;
        mController.setVisibleRect(x, y, width, height);
        mController.notifyLayerClientOfGeometryChange();
        populatePositionAndLength();
        return true;
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        mState = PanZoomState.PINCHING;
        RectF initialZoomRect = mController.getVisibleRect();
        float initialZoom = mController.getZoomFactor();

        mInitialZoomFocus = new PointF(initialZoomRect.left + (detector.getFocusX() / initialZoom),
                                       initialZoomRect.top + (detector.getFocusY() / initialZoom));
        mInitialZoomSpan = detector.getCurrentSpan() / initialZoom;

        GeckoApp.mAppContext.hidePluginViews();
        return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        mState = PanZoomState.PANNING_HOLD_LOCKED;
        mX.firstTouchPos = mX.touchPos = detector.getFocusX();
        mY.firstTouchPos = mY.touchPos = detector.getFocusY();

        GeckoApp.mAppContext.showPluginViews();
    }

    @Override
    public void onLongPress(MotionEvent motionEvent) {
        JSONObject ret = new JSONObject();
        try {
            PointF point = new PointF(motionEvent.getX(), motionEvent.getY());
            point = mController.convertViewPointToLayerPoint(point);
            if (point == null) {
                return;
            }
            ret.put("x", (int)Math.round(point.x));
            ret.put("y", (int)Math.round(point.y));
        } catch(Exception ex) {
            Log.w(LOGTAG, "Error building return: " + ex);
        }

        GeckoEvent e = new GeckoEvent("Gesture:LongPress", ret.toString());
        GeckoAppShell.sendEventToGecko(e);
    }
}
