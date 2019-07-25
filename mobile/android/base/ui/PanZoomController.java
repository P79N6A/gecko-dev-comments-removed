




































package org.mozilla.gecko.ui;

import org.json.JSONObject;
import org.json.JSONException;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.RectUtils;
import org.mozilla.gecko.gfx.ViewportMetrics;
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
    
    
    private static final float PAN_THRESHOLD = 0.1f;
    
    private static final double AXIS_LOCK_ANGLE = Math.PI / 6.0; 
    
    
    private static final float MAX_EVENT_ACCELERATION = 0.012f;
    
    
    private static final float MIN_SCROLLABLE_DISTANCE = 0.5f;
    
    private static final float MAX_ZOOM = 4.0f;

    
    private static final float[] EASE_OUT_ANIMATION_FRAMES = {
        0.00000f,   
        0.10211f,   
        0.19864f,   
        0.29043f,   
        0.37816f,   
        0.46155f,   
        0.54054f,   
        0.61496f,   
        0.68467f,   
        0.74910f,   
        0.80794f,   
        0.86069f,   
        0.90651f,   
        0.94471f,   
        0.97401f,   
        0.99309f,   
    };

    
    private Timer mAnimationTimer;
    
    private AxisX mX;
    
    private AxisY mY;
    
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
        ANIMATED_ZOOM,  
        BOUNCING,       
    }

    private PanZoomState mState;

    private boolean mOverridePanning;
    private boolean mOverrideScrollAck;
    private boolean mOverrideScrollPending;

    
    private int mBounceFrame;
    



    private ViewportMetrics mBounceStartMetrics, mBounceEndMetrics;

    public PanZoomController(LayerController controller) {
        mController = controller;
        mX = new AxisX(); mY = new AxisY();
        mState = PanZoomState.NOTHING;
        mBounceFrame = -1;

        GeckoAppShell.registerGeckoEventListener("Browser:ZoomToRect", this);
        GeckoAppShell.registerGeckoEventListener("Browser:ZoomToPageWidth", this);
        GeckoAppShell.registerGeckoEventListener("Panning:Override", this);
        GeckoAppShell.registerGeckoEventListener("Panning:CancelOverride", this);
        GeckoAppShell.registerGeckoEventListener("Gesture:ScrollAck", this);
    }

    protected void finalize() throws Throwable {
        GeckoAppShell.unregisterGeckoEventListener("Browser:ZoomToRect", this);
        GeckoAppShell.unregisterGeckoEventListener("Browser:ZoomToPageWidth", this);
        GeckoAppShell.unregisterGeckoEventListener("Panning:Override", this);
        GeckoAppShell.unregisterGeckoEventListener("Panning:CancelOverride", this);
        GeckoAppShell.unregisterGeckoEventListener("Gesture:ScrollAck", this);
        super.finalize();
    }

    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if ("Panning:Override".equals(event)) {
                mOverridePanning = true;
                mOverrideScrollAck = true;
            } else if ("Panning:CancelOverride".equals(event)) {
                mOverridePanning = false;
            } else if ("Gesture:ScrollAck".equals(event)) {
                mController.post(new Runnable() {
                    public void run() {
                        mOverrideScrollAck = true;
                        if (mOverridePanning && mOverrideScrollPending)
                            updatePosition();
                    }
                });
            } else if (event.equals("Browser:ZoomToRect")) {
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
        if (aAbortFling) {
            
            
            
            switch (mState) {
            case FLING:
                mX.velocity = mY.velocity = 0.0f;
                mState = PanZoomState.NOTHING;
                
            case NOTHING:
                bounce();
                break;
            }
        }
    }

    



    private boolean onTouchStart(MotionEvent event) {
        
        
        stopAnimationTimer();
        mOverridePanning = false;

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
            
            
            
            bounce();
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
        
        bounce();
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

        mX.setFlingState(Axis.FlingStates.PANNING); mY.setFlingState(Axis.FlingStates.PANNING);
        mX.displace(); mY.displace();
        updatePosition();
    }

    private void fling() {
        if (mState != PanZoomState.FLING)
            mX.velocity = mY.velocity = 0.0f;

        mX.disableSnap = mY.disableSnap = mOverridePanning;

        mX.displace(); mY.displace();
        updatePosition();

        stopAnimationTimer();

        boolean stopped = stopped();
        mX.startFling(stopped); mY.startFling(stopped);

        startAnimationTimer(new FlingRunnable());
    }

    
    private void bounce(ViewportMetrics metrics) {
        stopAnimationTimer();

        mBounceFrame = 0;
        mState = PanZoomState.FLING;
        mX.setFlingState(Axis.FlingStates.SNAPPING); mY.setFlingState(Axis.FlingStates.SNAPPING);
        mBounceStartMetrics = new ViewportMetrics(mController.getViewportMetrics());
        mBounceEndMetrics = metrics;

        startAnimationTimer(new BounceRunnable());
    }

    
    private void bounce() {
        bounce(getValidViewportMetrics());
    }

    
    private void startAnimationTimer(final Runnable runnable) {
        if (mAnimationTimer != null) {
            Log.e(LOGTAG, "Attempted to start a new fling without canceling the old one!");
            stopAnimationTimer();
        }

        mAnimationTimer = new Timer();
        mAnimationTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() { mController.post(runnable); }
        }, 0, 1000L/60L);
    }

    
    private void stopAnimationTimer() {
        if (mAnimationTimer != null) {
            mAnimationTimer.cancel();
            mAnimationTimer = null;
        }
    }

    private boolean stopped() {
        float absVelocity = (float)Math.sqrt(mX.velocity * mX.velocity +
                                             mY.velocity * mY.velocity);
        return absVelocity < STOPPED_THRESHOLD;
    }

    private void updatePosition() {
        if (mOverridePanning) {
            if (!mOverrideScrollAck) {
                mOverrideScrollPending = true;
                return;
            }

            mOverrideScrollPending = false;
            JSONObject json = new JSONObject();

            try {
                json.put("x", mX.displacement);
                json.put("y", mY.displacement);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error forming Gesture:Scroll message: " + e);
            }

            GeckoEvent e = new GeckoEvent("Gesture:Scroll", json.toString());
            GeckoAppShell.sendEventToGecko(e);
            mOverrideScrollAck = false;
        } else {
            mController.scrollBy(new PointF(mX.displacement, mY.displacement));
        }

        mX.displacement = mY.displacement = 0;
    }

    
    private class BounceRunnable implements Runnable {
        public void run() {
            




            if (mState != PanZoomState.FLING) {
                finishAnimation();
                return;
            }

            
            if (mBounceFrame < EASE_OUT_ANIMATION_FRAMES.length) {
                advanceBounce();
                return;
            }

            
            finishBounce();
            finishAnimation();
        }

        
        private void advanceBounce() {
            float t = EASE_OUT_ANIMATION_FRAMES[mBounceFrame];
            ViewportMetrics newMetrics = mBounceStartMetrics.interpolate(mBounceEndMetrics, t);
            mController.setViewportMetrics(newMetrics);
            mController.notifyLayerClientOfGeometryChange();
            mBounceFrame++;
        }

        
        private void finishBounce() {
            mController.setViewportMetrics(mBounceEndMetrics);
            mController.notifyLayerClientOfGeometryChange();
            mBounceFrame = -1;
        }
    }

    
    private class FlingRunnable implements Runnable {
        public void run() {
            




            if (mState != PanZoomState.FLING) {
                finishAnimation();
                return;
            }

            
            boolean flingingX = mX.getFlingState() == Axis.FlingStates.FLINGING;
            boolean flingingY = mY.getFlingState() == Axis.FlingStates.FLINGING;
            if (flingingX)
                mX.advanceFling();
            if (flingingY)
                mY.advanceFling();

            
            if (flingingX || flingingY) {
                mX.displace(); mY.displace();
                updatePosition();
            }

            
            PointF velocityVector = new PointF(mX.getRealVelocity(), mY.getRealVelocity());
            if (PointUtils.distance(velocityVector) >= STOPPED_THRESHOLD)
                return;

            



            boolean overscrolledX = mX.getOverscroll() != Axis.Overscroll.NONE;
            boolean overscrolledY = mY.getOverscroll() != Axis.Overscroll.NONE;
            if (!mOverridePanning && (overscrolledX || overscrolledY))
                bounce();
            else
                finishAnimation();
        }
    }

    private void finishAnimation() {
        mState = PanZoomState.NOTHING;
        stopAnimationTimer();

        
        mController.setForceRedraw();
        mController.notifyLayerClientOfGeometryChange();
    }

    private float computeElasticity(float excess, float viewportLength) {
        return 1.0f - excess / (viewportLength * SNAP_LIMIT);
    }

    
    private abstract static class Axis {
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
        public boolean disableSnap;             

        private FlingStates mFlingState;        

        public abstract float getOrigin();
        protected abstract float getViewportLength();
        protected abstract float getPageLength();

        public float displacement;

        private int mSnapFrame;
        private float mSnapPos, mSnapEndPos;

        public Axis() { mSnapFrame = -1; }

        public FlingStates getFlingState() { return mFlingState; }

        public void setFlingState(FlingStates aFlingState) {
            mFlingState = aFlingState;
        }

        private float getViewportEnd() { return getOrigin() + getViewportLength(); }

        public Overscroll getOverscroll() {
            boolean minus = (getOrigin() < 0.0f);
            boolean plus = (getViewportEnd() > getPageLength());
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
            case MINUS:     return -getOrigin();
            case PLUS:      return getViewportEnd() - getPageLength();
            case BOTH:      return getViewportEnd() - getPageLength() - getOrigin();
            default:        return 0.0f;
            }
        }

        



        private boolean scrollable() {
            return getViewportLength() <= getPageLength() - MIN_SCROLLABLE_DISTANCE;
        }

        



        public float getEdgeResistance() {
            float excess = getExcess();
            return (excess > 0.0f) ? SNAP_LIMIT - excess / getViewportLength() : 1.0f;
        }

        
        public float getRealVelocity() {
            return locked ? 0.0f : velocity;
        }

        public void startFling(boolean stopped) {
            if (!stopped) {
                setFlingState(FlingStates.FLINGING);
                return;
            }

            if (disableSnap || FloatUtils.fuzzyEquals(getExcess(), 0.0f))
                setFlingState(FlingStates.STOPPED);
            else
                setFlingState(FlingStates.WAITING_TO_SNAP);
        }

        
        public void advanceFling() {
            
            float excess = getExcess();
            if (disableSnap || FloatUtils.fuzzyEquals(excess, 0.0f)) {
                velocity *= FRICTION;
                if (Math.abs(velocity) < 0.1f) {
                    velocity = 0.0f;
                    setFlingState(FlingStates.STOPPED);
                }
                return;
            }

            
            float elasticity = 1.0f - excess / (getViewportLength() * SNAP_LIMIT);
            if (getOverscroll() == Overscroll.MINUS)
                velocity = Math.min((velocity + OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);
            else 
                velocity = Math.max((velocity - OVERSCROLL_DECEL_RATE) * elasticity, 0.0f);

            if (Math.abs(velocity) < 0.3f) {
                velocity = 0.0f;
                setFlingState(FlingStates.WAITING_TO_SNAP);
            }
        }

        
        public void displace() {
            if (locked || !scrollable())
                return;

            if (mFlingState == FlingStates.PANNING)
                displacement += (lastTouchPos - touchPos) * getEdgeResistance();
            else
                displacement += velocity;
        }
    }

    
    private ViewportMetrics getValidViewportMetrics() {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mController.getViewportMetrics());

        
        float zoomFactor = viewportMetrics.getZoomFactor();
        FloatSize pageSize = viewportMetrics.getPageSize();
        RectF viewport = viewportMetrics.getViewport();

        float minZoomFactor = 0.0f;
        if (viewport.width() > pageSize.width && pageSize.width > 0) {
            float scaleFactor = viewport.width() / pageSize.width;
            minZoomFactor = (float)Math.max(minZoomFactor, zoomFactor * scaleFactor);
        }
        if (viewport.height() > pageSize.height && pageSize.height > 0) {
            float scaleFactor = viewport.height() / pageSize.height;
            minZoomFactor = (float)Math.max(minZoomFactor, zoomFactor * scaleFactor);
        }

        if (!FloatUtils.fuzzyEquals(minZoomFactor, 0.0f)) {
            PointF center = new PointF(viewport.width() / 2.0f, viewport.height() / 2.0f);
            viewportMetrics.scaleTo(minZoomFactor, center);
        } else if (zoomFactor > MAX_ZOOM) {
            PointF center = new PointF(viewport.width() / 2.0f, viewport.height() / 2.0f);
            viewportMetrics.scaleTo(MAX_ZOOM, center);
        }

        
        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        return viewportMetrics;
    }

    private class AxisX extends Axis {
        @Override
        public float getOrigin() { return mController.getOrigin().x; }
        @Override
        protected float getViewportLength() { return mController.getViewportSize().width; }
        @Override
        protected float getPageLength() { return mController.getPageSize().width; }
    }

    private class AxisY extends Axis {
        @Override
        public float getOrigin() { return mController.getOrigin().y; }
        @Override
        protected float getViewportLength() { return mController.getViewportSize().height; }
        @Override
        protected float getPageLength() { return mController.getPageSize().height; }
    }

    


    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        if (mState == PanZoomState.ANIMATED_ZOOM)
            return false;

        float spanRatio = detector.getCurrentSpan() / detector.getPreviousSpan();

        



        float resistance = Math.min(mX.getEdgeResistance(), mY.getEdgeResistance());
        if (spanRatio > 1.0f)
            spanRatio = 1.0f + (spanRatio - 1.0f) * resistance;
        else
            spanRatio = 1.0f - (1.0f - spanRatio) * resistance;

        float newZoomFactor = mController.getZoomFactor() * spanRatio;

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
        mX.firstTouchPos = mX.lastTouchPos = mX.touchPos = detector.getFocusX();
        mY.firstTouchPos = mY.lastTouchPos = mY.touchPos = detector.getFocusY();

        RectF viewport = mController.getViewport();

        FloatSize pageSize = mController.getPageSize();
        RectF pageRect = new RectF(0,0, pageSize.width, pageSize.height);

        
        mController.setForceRedraw();
        mController.notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.showPluginViews();

        
        bounce();
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
        return (mState == PanZoomState.NOTHING || mState == PanZoomState.FLING);
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

    public boolean animatedZoomTo(RectF zoomToRect) {
        GeckoApp.mAppContext.hidePluginViews();

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
        float finalZoom = viewport.width() * startZoom / zoomToRect.width();

        ViewportMetrics finalMetrics = new ViewportMetrics(mController.getViewportMetrics());
        finalMetrics.setOrigin(new PointF(zoomToRect.left, zoomToRect.top));
        finalMetrics.scaleTo(finalZoom, new PointF(0.0f, 0.0f));

        bounce(finalMetrics);
        return true;
    }
}
