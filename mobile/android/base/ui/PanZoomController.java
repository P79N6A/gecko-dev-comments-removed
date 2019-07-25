




































package org.mozilla.gecko.ui;

import org.json.JSONObject;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.RectUtils;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import java.lang.Math;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;







public class PanZoomController
    extends GestureDetector.SimpleOnGestureListener
    implements ScaleGestureDetector.OnScaleGestureListener, GeckoEventListener
{
    private static final String LOGTAG = "GeckoPanZoomController";

    private LayerController mController;

    private static final float FRICTION = 0.85f;
    
    private static final float STOPPED_THRESHOLD = 4.0f;
    
    private static final float SNAP_LIMIT = 0.75f;
    
    private static final float OVERSCROLL_DECEL_RATE = 0.04f;
    
    private static final int SNAP_TIME = 240;
    
    
    private static final int SUBDIVISION_COUNT = 1000;
    
    
    private static final float PAN_THRESHOLD = 0.1f;
    
    private static final double AXIS_LOCK_ANGLE = Math.PI / 6.0; 
    
    
    private static final float MAX_EVENT_ACCELERATION = 0.012f;
    
    public static final int ZOOM_DURATION         = 200;

    private Timer mFlingTimer;
    private Axis mX, mY;
    
    private PointF mLastZoomFocus;
    
    private long mLastEventTime;

    private enum PanZoomState {
        NOTHING,        
        FLING,          
        TOUCHING,       
        PANNING_LOCKED, 
        PANNING,        
        PANNING_HOLD,   

        PANNING_HOLD_LOCKED, 
        PINCHING,       
        ANIMATED_ZOOM   
    }

    private PanZoomState mState;

    public PanZoomController(LayerController controller) {
        mController = controller;
        mX = new Axis(); mY = new Axis();
        mState = PanZoomState.NOTHING;

        populatePositionAndLength();

        GeckoAppShell.registerGeckoEventListener("Browser:ZoomToRect", this);
        GeckoAppShell.registerGeckoEventListener("Browser:ZoomToPageWidth", this);
    }

    protected void finalize() throws Throwable {
        GeckoAppShell.unregisterGeckoEventListener("Browser:ZoomToRect", this);
        GeckoAppShell.unregisterGeckoEventListener("Browser:ZoomToPageWidth", this);
        super.finalize();
    }
    
    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if (event.equals("Browser:ZoomToRect")) {
                if (mController != null) {
                    float scale = mController.getZoomFactor();
                    float x = (float)message.getDouble("x");
                    float y = (float)message.getDouble("y");
                    final RectF zoomRect = new RectF(x, y,
                                         x + (float)message.getDouble("w"),
                                         y + (float)message.getDouble("h"));
                    mController.post(new Runnable() {
                        public void run() {
                            animatedZoomTo(zoomRect);
                        }
                    });
                }
            } else if (event.equals("Browser:ZoomToPageWidth")) {
                if (mController != null) {
                    float scale = mController.getZoomFactor();
                    FloatSize pageSize = mController.getPageSize();

                    RectF viewableRect = mController.getViewport();
                    float y = viewableRect.top;
                    
                    float dh = viewableRect.height()*(1 - pageSize.width/viewableRect.width()); 
                    final RectF r = new RectF(0.0f,
                                        y + dh/2,
                                        pageSize.width,
                                        (y + pageSize.width * viewableRect.height()/viewableRect.width()));
                    mController.post(new Runnable() {
                        public void run() {
                            animatedZoomTo(r);
                        }
                    });
                }
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction() & event.ACTION_MASK) {
        case MotionEvent.ACTION_DOWN:   return onTouchStart(event);
        case MotionEvent.ACTION_MOVE:   return onTouchMove(event);
        case MotionEvent.ACTION_UP:     return onTouchEnd(event);
        case MotionEvent.ACTION_CANCEL: return onTouchCancel(event);
        default:                        return false;
        }
    }

    public void geometryChanged(boolean aAbortFling) {
        populatePositionAndLength();

        if (aAbortFling) {
            
            
            
            switch (mState) {
            case FLING:
                mX.velocity = mY.velocity = 0.0f;
                mState = PanZoomState.NOTHING;
                
            case NOTHING:
                fling();
                break;
            }
        }
    }

    



    private boolean onTouchStart(MotionEvent event) {
        
        
        if (mFlingTimer != null) {
            mFlingTimer.cancel();
            mFlingTimer = null;
        }

        switch (mState) {
        case ANIMATED_ZOOM:
            return false;
        case FLING:
        case NOTHING:
            mState = PanZoomState.TOUCHING;
            mX.velocity = mY.velocity = 0.0f;
            mX.locked = mY.locked = false;
            mX.lastTouchPos = mX.firstTouchPos = mX.touchPos = event.getX(0);
            mY.lastTouchPos = mY.firstTouchPos = mY.touchPos = event.getY(0);
            mLastEventTime = event.getEventTime();
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
            if (panDistance(event) < PAN_THRESHOLD * GeckoAppShell.getDpi())
                return false;
            cancelTouch();
            
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
        case ANIMATED_ZOOM:
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
        case ANIMATED_ZOOM:
            return false;
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

    private float clampByFactor(float oldValue, float newValue, float factor) {
        float maxChange = Math.abs(oldValue * factor);
        return Math.min(oldValue + maxChange, Math.max(oldValue - maxChange, newValue));
    }

    private void track(float x, float y, float lastX, float lastY, float timeDelta) {
        if (mState == PanZoomState.PANNING_LOCKED) {
            
            double angle = Math.atan2(y - mY.firstTouchPos, x - mX.firstTouchPos); 
            angle = Math.abs(angle); 
            if (angle < AXIS_LOCK_ANGLE || angle > (Math.PI - AXIS_LOCK_ANGLE)) {
                
                mX.locked = false;
                mY.locked = true;
            } else if (Math.abs(angle - (Math.PI / 2)) < AXIS_LOCK_ANGLE) {
                
                mX.locked = true;
                mY.locked = false;
            } else {
                
                mState = PanZoomState.PANNING;
                mX.locked = mY.locked = false;
                angle = Math.abs(angle - (Math.PI / 2));  
                Log.i(LOGTAG, "Breaking axis lock at " + (angle * 180.0 / Math.PI) + " degrees");
            }
        }

        float newVelocityX = ((lastX - x) / timeDelta) * (1000.0f/60.0f);
        float newVelocityY = ((lastY - y) / timeDelta) * (1000.0f/60.0f);
        float maxChange = MAX_EVENT_ACCELERATION * timeDelta;

        
        
        
        if (Math.abs(mX.velocity) < 1.0f ||
            (((mX.velocity > 0) != (newVelocityX > 0)) &&
             !FloatUtils.fuzzyEquals(newVelocityX, 0.0f)))
            mX.velocity = newVelocityX;
        else
            mX.velocity = clampByFactor(mX.velocity, newVelocityX, maxChange);
        if (Math.abs(mY.velocity) < 1.0f ||
            (((mY.velocity > 0) != (newVelocityY > 0)) &&
             !FloatUtils.fuzzyEquals(newVelocityY, 0.0f)))
            mY.velocity = newVelocityY;
        else
            mY.velocity = clampByFactor(mY.velocity, newVelocityY, maxChange);
    }

    private void track(MotionEvent event) {
        mX.lastTouchPos = mX.touchPos;
        mY.lastTouchPos = mY.touchPos;

        for (int i = 0; i < event.getHistorySize(); i++) {
            float x = event.getHistoricalX(0, i);
            float y = event.getHistoricalY(0, i);
            long time = event.getHistoricalEventTime(i);

            float timeDelta = (float)(time - mLastEventTime);
            mLastEventTime = time;

            track(x, y, mX.touchPos, mY.touchPos, timeDelta);
            mX.touchPos = x; mY.touchPos = y;
        }

        float timeDelta = (float)(event.getEventTime() - mLastEventTime);
        mLastEventTime = event.getEventTime();

        track(event.getX(0), event.getY(0), mX.touchPos, mY.touchPos, timeDelta);

        mX.touchPos = event.getX(0);
        mY.touchPos = event.getY(0);

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

        mX.setFlingState(Axis.FlingStates.PANNING);
        mY.setFlingState(Axis.FlingStates.PANNING);
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
        mController.scrollTo(new PointF(mX.viewportPos, mY.viewportPos));
    }

    
    private void populatePositionAndLength() {
        FloatSize pageSize = mController.getPageSize();
        RectF visibleRect = mController.getViewport();

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

            
            mController.setForceRedraw();
            mController.notifyLayerClientOfGeometryChange();
        }
    }

    private float computeElasticity(float excess, float viewportLength) {
        return 1.0f - excess / (viewportLength * SNAP_LIMIT);
    }

    
    private static class Axis {
        public enum FlingStates {
            STOPPED,
            PANNING,
            FLINGING,
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
        public float lastTouchPos;              
        public float velocity;                  
        public boolean locked;                  

        private FlingStates mFlingState;        
        private EaseOutAnimation mSnapAnim;     

        
        public float viewportPos;
        private float mViewportLength;
        private int mScreenLength;
        private float mPageLength;

        public FlingStates getFlingState() { return mFlingState; }

        public void setFlingState(FlingStates aFlingState) {
            mFlingState = aFlingState;
        }

        public void setViewportLength(float viewportLength) { mViewportLength = viewportLength; }
        public void setScreenLength(int screenLength) { mScreenLength = screenLength; }
        public void setPageLength(float pageLength) { mPageLength = pageLength; }

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
                setFlingState(FlingStates.FLINGING);
                return;
            }

            float excess = getExcess();
            if (FloatUtils.fuzzyEquals(excess, 0.0f))
                setFlingState(FlingStates.STOPPED);
            else
                setFlingState(FlingStates.WAITING_TO_SNAP);
        }

        
        public void advanceFling() {
            switch (mFlingState) {
            case FLINGING:
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
            if (FloatUtils.fuzzyEquals(excess, 0.0f)) {
                velocity *= FRICTION;
                if (Math.abs(velocity) < 0.1f) {
                    velocity = 0.0f;
                    setFlingState(FlingStates.STOPPED);
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
                setFlingState(FlingStates.WAITING_TO_SNAP);
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
                
                setFlingState(FlingStates.STOPPED);
                return;
            }

            setFlingState(FlingStates.SNAPPING);
        }

        
        private void snap() {
            mSnapAnim.advance();
            viewportPos = mSnapAnim.getPosition();

            if (mSnapAnim.getFinished()) {
                mSnapAnim = null;
                setFlingState(FlingStates.STOPPED);
            }
        }

        
        public void displace() {
            if (locked)
                return;

            if (mFlingState == FlingStates.PANNING)
                viewportPos += lastTouchPos - touchPos;
            else
                viewportPos += velocity;
        }
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
        if (mState == PanZoomState.ANIMATED_ZOOM)
            return false;

        float newZoomFactor = mController.getZoomFactor() *
                              (detector.getCurrentSpan() / detector.getPreviousSpan());

        mController.scrollBy(new PointF(mLastZoomFocus.x - detector.getFocusX(),
                                        mLastZoomFocus.y - detector.getFocusY()));
        mController.scaleWithFocus(newZoomFactor, new PointF(detector.getFocusX(), detector.getFocusY()));

        mLastZoomFocus.set(detector.getFocusX(), detector.getFocusY());

        return true;
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        if (mState == PanZoomState.ANIMATED_ZOOM)
            return false;

        mState = PanZoomState.PINCHING;
        mLastZoomFocus = new PointF(detector.getFocusX(), detector.getFocusY());
        GeckoApp.mAppContext.hidePluginViews();
        cancelTouch();

        return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        PointF o = mController.getOrigin();
        if (mState == PanZoomState.ANIMATED_ZOOM)
            return;

        mState = PanZoomState.PANNING_HOLD_LOCKED;
        mX.firstTouchPos = mX.touchPos = detector.getFocusX();
        mY.firstTouchPos = mY.touchPos = detector.getFocusY();

        RectF viewport = mController.getViewport();
 
        FloatSize pageSize = mController.getPageSize();
        RectF pageRect = new RectF(0,0, pageSize.width, pageSize.height);

        if (!pageRect.contains(viewport)) {
            
            animatedZoomTo(viewport);
        } else {
            
            mController.setForceRedraw();
            mController.notifyLayerClientOfGeometryChange();
            GeckoApp.mAppContext.showPluginViews();
        }
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

    public boolean getRedrawHint() {
        return (mState != PanZoomState.PINCHING);
    }

    @Override
    public boolean onDown(MotionEvent motionEvent) {
        JSONObject ret = new JSONObject();
        try {
            PointF point = new PointF(motionEvent.getX(), motionEvent.getY());
            point = mController.convertViewPointToLayerPoint(point);
            ret.put("x", (int)Math.round(point.x));
            ret.put("y", (int)Math.round(point.y));
        } catch(Exception ex) {
            throw new RuntimeException(ex);
        }

        GeckoEvent e = new GeckoEvent("Gesture:ShowPress", ret.toString());
        GeckoAppShell.sendEventToGecko(e);
        return false;
    }

    @Override
    public boolean onSingleTapConfirmed(MotionEvent motionEvent) {
        JSONObject ret = new JSONObject();
        try {
            PointF point = new PointF(motionEvent.getX(), motionEvent.getY());
            point = mController.convertViewPointToLayerPoint(point);
            ret.put("x", (int)Math.round(point.x));
            ret.put("y", (int)Math.round(point.y));
        } catch(Exception ex) {
            throw new RuntimeException(ex);
        }

        GeckoEvent e = new GeckoEvent("Gesture:SingleTap", ret.toString());
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    private void cancelTouch() {
        GeckoEvent e = new GeckoEvent("Gesture:CancelTouch", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    @Override
    public boolean onDoubleTap(MotionEvent motionEvent) {
        JSONObject ret = new JSONObject();
        try {
            PointF point = new PointF(motionEvent.getX(), motionEvent.getY());
            point = mController.convertViewPointToLayerPoint(point);
            ret.put("x", (int)Math.round(point.x));
            ret.put("y", (int)Math.round(point.y));
        } catch(Exception ex) {
            throw new RuntimeException(ex);
        }

        GeckoEvent e = new GeckoEvent("Gesture:DoubleTap", ret.toString());
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    private Timer mZoomTimer;
    public boolean animatedZoomTo(RectF zoomToRect) {
        GeckoApp.mAppContext.hidePluginViews();

        if (mZoomTimer != null) {
            mZoomTimer.cancel();
        }

        mState = PanZoomState.ANIMATED_ZOOM;
        final float startZoom = mController.getZoomFactor();
        final PointF startPoint = mController.getOrigin();

        RectF viewport = mController.getViewport();

        float newHeight = zoomToRect.width() * viewport.height() / viewport.width();
        
        if (zoomToRect.height() < newHeight) {
            zoomToRect.top -= (newHeight - zoomToRect.height())/2;
            zoomToRect.bottom = zoomToRect.top + newHeight;
        }

        zoomToRect = mController.restrictToPageSize(zoomToRect);
        final float finalZoom = viewport.width() * startZoom / zoomToRect.width();
        zoomToRect = RectUtils.scale(zoomToRect, finalZoom/startZoom);
        final PointF finalPoint = new PointF(zoomToRect.left, zoomToRect.top);

        mZoomTimer = new Timer();
        final long startTime = new Date().getTime();

        mZoomTimer.scheduleAtFixedRate(new TimerTask() {
            public void run() {
                long now = new Date().getTime();
                final float dt = (float)(now - startTime)/ZOOM_DURATION;

                if (dt < 1) {
                    mController.post(new Runnable() {
                        public void run() {
                            PointF currentPoint = PointUtils.interpolate(finalPoint, startPoint, dt);
                            float  currentScale = startZoom + (finalZoom-startZoom)*dt;
                            mController.scaleWithOrigin(currentScale, currentPoint);
                        }
                    });
                } else {
                    mController.post(new Runnable() {
                        public void run() {
                            mController.scaleWithOrigin(finalZoom, finalPoint);
                            mController.setForceRedraw();
                            GeckoApp.mAppContext.showPluginViews();
                            mController.notifyLayerClientOfGeometryChange();
                            populatePositionAndLength();
                        }
                    });
                    mZoomTimer.cancel();
                    mZoomTimer = null;
                    mState = PanZoomState.NOTHING;
               }
            }
        }, 0, 1000L/60L);
        return true;
    }
}
