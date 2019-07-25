





































package org.mozilla.gecko.ui;

import org.json.JSONObject;
import org.json.JSONException;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.ViewportMetrics;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.FloatMath;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import java.util.Timer;
import java.util.TimerTask;







public class PanZoomController
    extends GestureDetector.SimpleOnGestureListener
    implements SimpleScaleGestureDetector.SimpleScaleGestureListener, GeckoEventListener
{
    private static final String LOGTAG = "GeckoPanZoomController";

    private static String MESSAGE_ZOOM_RECT = "Browser:ZoomToRect";
    private static String MESSAGE_ZOOM_PAGE = "Browser:ZoomToPageWidth";

    
    private static final float STOPPED_THRESHOLD = 4.0f;
    
    private static final float FLING_STOPPED_THRESHOLD = 0.1f;
    
    
    public static final float PAN_THRESHOLD = 0.1f;
    
    private static final double AXIS_LOCK_ANGLE = Math.PI / 6.0; 
    
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

    private final LayerController mController;
    private final SubdocumentScrollHelper mSubscroller;
    private final Axis mX;
    private final Axis mY;

    private Thread mMainThread;

    
    private Timer mAnimationTimer;
    
    private AnimationRunnable mAnimationRunnable;
    
    private PointF mLastZoomFocus;
    
    private long mLastEventTime;
    
    private PanZoomState mState;

    public PanZoomController(LayerController controller) {
        mController = controller;
        mSubscroller = new SubdocumentScrollHelper(this);
        mX = new AxisX(mSubscroller);
        mY = new AxisY(mSubscroller);

        mMainThread = GeckoApp.mAppContext.getMainLooper().getThread();
        checkMainThread();

        mState = PanZoomState.NOTHING;

        GeckoAppShell.registerGeckoEventListener(MESSAGE_ZOOM_RECT, this);
        GeckoAppShell.registerGeckoEventListener(MESSAGE_ZOOM_PAGE, this);
    }

    
    private void checkMainThread() {
        if (mMainThread != Thread.currentThread()) {
            
            Log.e(LOGTAG, "Uh-oh, we're running on the wrong thread!", new Exception());
        }
    }

    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if (MESSAGE_ZOOM_RECT.equals(event)) {
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
            } else if (MESSAGE_ZOOM_PAGE.equals(event)) {
                float scale = mController.getZoomFactor();
                FloatSize pageSize = mController.getPageSize();

                RectF viewableRect = mController.getViewport();
                float y = viewableRect.top;
                
                float newHeight = viewableRect.height() * pageSize.width / viewableRect.width();
                float dh = viewableRect.height() - newHeight; 
                final RectF r = new RectF(0.0f,
                                    y + dh/2,
                                    pageSize.width,
                                    y + dh/2 + newHeight);
                mController.post(new Runnable() {
                    public void run() {
                        animatedZoomTo(r);
                    }
                });
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

    
    @SuppressWarnings("fallthrough")
    public void abortAnimation() {
        checkMainThread();
        
        
        
        
        switch (mState) {
        case FLING:
            mX.stopFling();
            mY.stopFling();
            mState = PanZoomState.NOTHING;
            
        case ANIMATED_ZOOM:
            
            
            
        case NOTHING:
            
            
            mController.setViewportMetrics(getValidViewportMetrics());
            mController.notifyLayerClientOfGeometryChange();
            break;
        }
    }

    
    public void pageSizeUpdated() {
        if (mState == PanZoomState.NOTHING) {
            ViewportMetrics validated = getValidViewportMetrics();
            if (! mController.getViewportMetrics().fuzzyEquals(validated)) {
                
                
                mController.setViewportMetrics(validated);
                mController.notifyLayerClientOfGeometryChange();
            }
        }
    }

    



    private boolean onTouchStart(MotionEvent event) {
        Log.d(LOGTAG, "onTouchStart in state " + mState);
        
        
        stopAnimationTimer();
        mSubscroller.cancel();

        switch (mState) {
        case ANIMATED_ZOOM:
            return false;
        case FLING:
        case NOTHING:
            startTouch(event.getX(0), event.getY(0), event.getEventTime());
            return false;
        case TOUCHING:
        case PANNING:
        case PANNING_LOCKED:
        case PANNING_HOLD:
        case PANNING_HOLD_LOCKED:
        case PINCHING:
            Log.e(LOGTAG, "Received impossible touch down while in " + mState);
            return false;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in onTouchStart");
        return false;
    }

    @SuppressWarnings("fallthrough")
    private boolean onTouchMove(MotionEvent event) {
        Log.d(LOGTAG, "onTouchMove in state " + mState);

        switch (mState) {
        case NOTHING:
        case FLING:
            
            Log.e(LOGTAG, "Received impossible touch move while in " + mState);
            return false;
        case TOUCHING:
            if (panDistance(event) < PAN_THRESHOLD * GeckoAppShell.getDpi()) {
                return false;
            }
            cancelTouch();
            GeckoApp.mAppContext.hidePlugins(false );
            
        case PANNING_HOLD_LOCKED:
            GeckoApp.mAppContext.mAutoCompletePopup.hide();
            mState = PanZoomState.PANNING_LOCKED;
            
        case PANNING_LOCKED:
            track(event);
            return true;
        case PANNING_HOLD:
            GeckoApp.mAppContext.mAutoCompletePopup.hide();
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
        Log.d(LOGTAG, "onTouchEnd in " + mState);

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
            mState = PanZoomState.NOTHING;
            return true;
        case ANIMATED_ZOOM:
            return false;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in onTouchEnd");
        return false;
    }

    private boolean onTouchCancel(MotionEvent event) {
        Log.d(LOGTAG, "onTouchCancel in " + mState);

        mState = PanZoomState.NOTHING;
        
        bounce();
        return false;
    }

    private void startTouch(float x, float y, long time) {
        mX.startTouch(x);
        mY.startTouch(y);
        mState = PanZoomState.TOUCHING;
        mLastEventTime = time;
    }

    private float panDistance(MotionEvent move) {
        float dx = mX.panDistance(move.getX(0));
        float dy = mY.panDistance(move.getY(0));
        return FloatMath.sqrt(dx * dx + dy * dy);
    }

    private void track(float x, float y, long time) {
        float timeDelta = (float)(time - mLastEventTime);
        if (FloatUtils.fuzzyEquals(timeDelta, 0)) {
            
            
            return;
        }
        mLastEventTime = time;

        if (mState == PanZoomState.PANNING_LOCKED) {
            
            double angle = Math.atan2(mY.panDistance(y), mX.panDistance(x)); 
            angle = Math.abs(angle); 
            if (angle < AXIS_LOCK_ANGLE || angle > (Math.PI - AXIS_LOCK_ANGLE)) {
                
                mX.setLocked(false);
                mY.setLocked(true);
            } else if (Math.abs(angle - (Math.PI / 2)) < AXIS_LOCK_ANGLE) {
                
                mX.setLocked(true);
                mY.setLocked(false);
            } else {
                
                mState = PanZoomState.PANNING;
                mX.setLocked(false);
                mY.setLocked(false);
                angle = Math.abs(angle - (Math.PI / 2));  
            }
        }

        mX.updateWithTouchAt(x, timeDelta);
        mY.updateWithTouchAt(y, timeDelta);
    }

    private void track(MotionEvent event) {
        mX.saveTouchPos();
        mY.saveTouchPos();

        for (int i = 0; i < event.getHistorySize(); i++) {
            track(event.getHistoricalX(0, i),
                  event.getHistoricalY(0, i),
                  event.getHistoricalEventTime(i));
        }
        track(event.getX(0), event.getY(0), event.getEventTime());

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

        mX.startPan();
        mY.startPan();
        updatePosition();
    }

    private void fling() {
        updatePosition();

        stopAnimationTimer();

        boolean stopped = stopped();
        mX.startFling(stopped);
        mY.startFling(stopped);

        startAnimationTimer(new FlingRunnable());
    }

    
    private void bounce(ViewportMetrics metrics) {
        stopAnimationTimer();

        ViewportMetrics bounceStartMetrics = new ViewportMetrics(mController.getViewportMetrics());
        if (bounceStartMetrics.fuzzyEquals(metrics)) {
            mState = PanZoomState.NOTHING;
            return;
        }

        mState = PanZoomState.FLING;
        Log.d(LOGTAG, "end bounce at " + metrics);

        startAnimationTimer(new BounceRunnable(bounceStartMetrics, metrics));
    }

    
    private void bounce() {
        bounce(getValidViewportMetrics());
    }

    
    private void startAnimationTimer(final AnimationRunnable runnable) {
        if (mAnimationTimer != null) {
            Log.e(LOGTAG, "Attempted to start a new fling without canceling the old one!");
            stopAnimationTimer();
        }

        GeckoApp.mAppContext.hidePlugins(false );

        mAnimationTimer = new Timer("Animation Timer");
        mAnimationRunnable = runnable;
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
        if (mAnimationRunnable != null) {
            mAnimationRunnable.terminate();
            mAnimationRunnable = null;
        }

        GeckoApp.mAppContext.showPlugins();
    }

    private float getVelocity() {
        float xvel = mX.getRealVelocity();
        float yvel = mY.getRealVelocity();
        return FloatMath.sqrt(xvel * xvel + yvel * yvel);
    }

    private boolean stopped() {
        return getVelocity() < STOPPED_THRESHOLD;
    }

    PointF getDisplacement() {
        return new PointF(mX.resetDisplacement(), mY.resetDisplacement());
    }

    private void updatePosition() {
        mX.displace();
        mY.displace();
        PointF displacement = getDisplacement();
        if (! mSubscroller.scrollBy(displacement)) {
            synchronized (mController) {
                mController.scrollBy(displacement);
            }
        }
    }

    private abstract class AnimationRunnable implements Runnable {
        private boolean mAnimationTerminated;

        
        public final void run() {
            





            if (mAnimationTerminated) {
                return;
            }
            animateFrame();
        }

        protected abstract void animateFrame();

        
        protected final void terminate() {
            mAnimationTerminated = true;
        }
    }

    
    private class BounceRunnable extends AnimationRunnable {
        
        private int mBounceFrame;
        



        private ViewportMetrics mBounceStartMetrics;
        private ViewportMetrics mBounceEndMetrics;

        BounceRunnable(ViewportMetrics startMetrics, ViewportMetrics endMetrics) {
            mBounceStartMetrics = startMetrics;
            mBounceEndMetrics = endMetrics;
        }

        protected void animateFrame() {
            




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
            mState = PanZoomState.NOTHING;
        }

        
        private void advanceBounce() {
            synchronized (mController) {
                float t = EASE_OUT_ANIMATION_FRAMES[mBounceFrame];
                ViewportMetrics newMetrics = mBounceStartMetrics.interpolate(mBounceEndMetrics, t);
                mController.setViewportMetrics(newMetrics);
                mController.notifyLayerClientOfGeometryChange();
                mBounceFrame++;
            }
        }

        
        private void finishBounce() {
            synchronized (mController) {
                mController.setViewportMetrics(mBounceEndMetrics);
                mController.notifyLayerClientOfGeometryChange();
                mBounceFrame = -1;
            }
        }
    }

    
    private class FlingRunnable extends AnimationRunnable {
        protected void animateFrame() {
            




            if (mState != PanZoomState.FLING) {
                finishAnimation();
                return;
            }

            
            boolean flingingX = mX.advanceFling();
            boolean flingingY = mY.advanceFling();

            boolean overscrolled = (mX.overscrolled() || mY.overscrolled());

            
            if (flingingX || flingingY) {
                updatePosition();

                





                float threshold = (overscrolled && !mSubscroller.scrolling() ? STOPPED_THRESHOLD : FLING_STOPPED_THRESHOLD);
                if (getVelocity() >= threshold) {
                    
                    return;
                }

                mX.stopFling();
                mY.stopFling();
            }

            
            if (overscrolled) {
                bounce();
            } else {
                finishAnimation();
                mState = PanZoomState.NOTHING;
            }
        }
    }

    private void finishAnimation() {
        checkMainThread();

        Log.d(LOGTAG, "Finishing animation at " + mController.getViewportMetrics());
        stopAnimationTimer();

        
        GeckoApp.mAppContext.showPlugins();
        mController.setForceRedraw();
        mController.notifyLayerClientOfGeometryChange();
    }

    
    private ViewportMetrics getValidViewportMetrics() {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mController.getViewportMetrics());
        Log.d(LOGTAG, "generating valid viewport using " + viewportMetrics);

        
        float zoomFactor = viewportMetrics.getZoomFactor();
        FloatSize pageSize = viewportMetrics.getPageSize();
        RectF viewport = viewportMetrics.getViewport();

        float focusX = viewport.width() / 2.0f;
        float focusY = viewport.height() / 2.0f;
        float minZoomFactor = 0.0f;
        if (viewport.width() > pageSize.width && pageSize.width > 0) {
            float scaleFactor = viewport.width() / pageSize.width;
            minZoomFactor = Math.max(minZoomFactor, zoomFactor * scaleFactor);
            focusX = 0.0f;
        }
        if (viewport.height() > pageSize.height && pageSize.height > 0) {
            float scaleFactor = viewport.height() / pageSize.height;
            minZoomFactor = Math.max(minZoomFactor, zoomFactor * scaleFactor);
            focusY = 0.0f;
        }

        if (!FloatUtils.fuzzyEquals(minZoomFactor, 0.0f)) {
            
            
            
            
            
            PointF center = new PointF(focusX, focusY);
            viewportMetrics.scaleTo(minZoomFactor, center);
        } else if (zoomFactor > MAX_ZOOM) {
            PointF center = new PointF(viewport.width() / 2.0f, viewport.height() / 2.0f);
            viewportMetrics.scaleTo(MAX_ZOOM, center);
        }

        
        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());
        Log.d(LOGTAG, "generated valid viewport as " + viewportMetrics);

        return viewportMetrics;
    }

    private class AxisX extends Axis {
        AxisX(SubdocumentScrollHelper subscroller) { super(subscroller); }
        @Override
        public float getOrigin() { return mController.getOrigin().x; }
        @Override
        protected float getViewportLength() { return mController.getViewportSize().width; }
        @Override
        protected float getPageLength() { return mController.getPageSize().width; }
    }

    private class AxisY extends Axis {
        AxisY(SubdocumentScrollHelper subscroller) { super(subscroller); }
        @Override
        public float getOrigin() { return mController.getOrigin().y; }
        @Override
        protected float getViewportLength() { return mController.getViewportSize().height; }
        @Override
        protected float getPageLength() { return mController.getPageSize().height; }
    }

    


    @Override
    public boolean onScaleBegin(SimpleScaleGestureDetector detector) {
        Log.d(LOGTAG, "onScaleBegin in " + mState);

        if (mState == PanZoomState.ANIMATED_ZOOM)
            return false;

        mState = PanZoomState.PINCHING;
        mLastZoomFocus = new PointF(detector.getFocusX(), detector.getFocusY());
        GeckoApp.mAppContext.hidePlugins(false );
        GeckoApp.mAppContext.mAutoCompletePopup.hide();
        cancelTouch();

        return true;
    }

    @Override
    public boolean onScale(SimpleScaleGestureDetector detector) {
        Log.d(LOGTAG, "onScale in state " + mState);

        if (mState == PanZoomState.ANIMATED_ZOOM)
            return false;

        float prevSpan = detector.getPreviousSpan();
        if (FloatUtils.fuzzyEquals(prevSpan, 0.0f)) {
            
            return true;
        }

        float spanRatio = detector.getCurrentSpan() / prevSpan;

        



        float resistance = Math.min(mX.getEdgeResistance(), mY.getEdgeResistance());
        if (spanRatio > 1.0f)
            spanRatio = 1.0f + (spanRatio - 1.0f) * resistance;
        else
            spanRatio = 1.0f - (1.0f - spanRatio) * resistance;

        synchronized (mController) {
            float newZoomFactor = mController.getZoomFactor() * spanRatio;
            if (newZoomFactor >= MAX_ZOOM) {
                
                
                
                float excessZoom = newZoomFactor - MAX_ZOOM;
                excessZoom = 1.0f - (float)Math.exp(-excessZoom);
                newZoomFactor = MAX_ZOOM + excessZoom;
            }

            mController.scrollBy(new PointF(mLastZoomFocus.x - detector.getFocusX(),
                                            mLastZoomFocus.y - detector.getFocusY()));
            PointF focus = new PointF(detector.getFocusX(), detector.getFocusY());
            mController.scaleWithFocus(newZoomFactor, focus);
        }

        mLastZoomFocus.set(detector.getFocusX(), detector.getFocusY());

        return true;
    }

    @Override
    public void onScaleEnd(SimpleScaleGestureDetector detector) {
        Log.d(LOGTAG, "onScaleEnd in " + mState);

        if (mState == PanZoomState.ANIMATED_ZOOM)
            return;

        
        startTouch(detector.getFocusX(), detector.getFocusY(), detector.getEventTime());

        
        GeckoApp.mAppContext.showPlugins();
        mController.setForceRedraw();
        mController.notifyLayerClientOfGeometryChange();
    }

    public boolean getRedrawHint() {
        return (mState == PanZoomState.NOTHING || mState == PanZoomState.FLING);
    }

    private void sendPointToGecko(String event, MotionEvent motionEvent) {
        String json;
        try {
            PointF point = new PointF(motionEvent.getX(), motionEvent.getY());
            point = mController.convertViewPointToLayerPoint(point);
            if (point == null) {
                return;
            }
            json = PointUtils.toJSON(point).toString();
        } catch (Exception e) {
            Log.e(LOGTAG, "Unable to convert point to JSON for " + event, e);
            return;
        }

        GeckoAppShell.sendEventToGecko(new GeckoEvent(event, json));
    }

    @Override
    public void onLongPress(MotionEvent motionEvent) {
        sendPointToGecko("Gesture:LongPress", motionEvent);
    }

    @Override
    public boolean onDown(MotionEvent motionEvent) {
        sendPointToGecko("Gesture:ShowPress", motionEvent);
        return false;
    }

    @Override
    public boolean onSingleTapConfirmed(MotionEvent motionEvent) {
        GeckoApp.mAppContext.mAutoCompletePopup.hide();
        sendPointToGecko("Gesture:SingleTap", motionEvent);
        return true;
    }

    @Override
    public boolean onDoubleTap(MotionEvent motionEvent) {
        sendPointToGecko("Gesture:DoubleTap", motionEvent);
        return true;
    }

    public void cancelTouch() {
        GeckoEvent e = new GeckoEvent("Gesture:CancelTouch", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    private boolean animatedZoomTo(RectF zoomToRect) {
        GeckoApp.mAppContext.mAutoCompletePopup.hide();

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
