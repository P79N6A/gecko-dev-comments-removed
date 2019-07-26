




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.graphics.PointF;
import android.graphics.RectF;
import android.os.Build;
import android.util.FloatMath;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;

import java.util.Timer;
import java.util.TimerTask;







class JavaPanZoomController
    extends GestureDetector.SimpleOnGestureListener
    implements PanZoomController, SimpleScaleGestureDetector.SimpleScaleGestureListener, GeckoEventListener
{
    private static final String LOGTAG = "GeckoPanZoomController";

    private static String MESSAGE_ZOOM_RECT = "Browser:ZoomToRect";
    private static String MESSAGE_ZOOM_PAGE = "Browser:ZoomToPageWidth";
    private static String MESSAGE_TOUCH_LISTENER = "Tab:HasTouchListener";

    
    private static final float STOPPED_THRESHOLD = 4.0f;

    
    private static final float FLING_STOPPED_THRESHOLD = 0.1f;

    
    
    public static final float PAN_THRESHOLD = 1/16f * GeckoAppShell.getDpi();

    
    private static final double AXIS_LOCK_ANGLE = Math.PI / 6.0; 

    
    private static final float MAX_ZOOM = 4.0f;

    
    private static final float MAX_SCROLL = 0.075f * GeckoAppShell.getDpi();

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
        BOUNCE,         

        WAITING_LISTENERS, 


        AUTOSCROLL,     


    }

    private final PanZoomTarget mTarget;
    private final SubdocumentScrollHelper mSubscroller;
    private final Axis mX;
    private final Axis mY;
    private final TouchEventHandler mTouchEventHandler;
    private final EventDispatcher mEventDispatcher;
    private Thread mMainThread;

    
    private Timer mAnimationTimer;
    
    private AnimationRunnable mAnimationRunnable;
    
    private PointF mLastZoomFocus;
    
    private long mLastEventTime;
    
    private PanZoomState mState;

    public JavaPanZoomController(PanZoomTarget target, View view, EventDispatcher eventDispatcher) {
        mTarget = target;
        mSubscroller = new SubdocumentScrollHelper(eventDispatcher);
        mX = new AxisX(mSubscroller);
        mY = new AxisY(mSubscroller);
        mTouchEventHandler = new TouchEventHandler(view.getContext(), view, this);

        mMainThread = GeckoApp.mAppContext.getMainLooper().getThread();
        checkMainThread();

        setState(PanZoomState.NOTHING);

        mEventDispatcher = eventDispatcher;
        registerEventListener(MESSAGE_ZOOM_RECT);
        registerEventListener(MESSAGE_ZOOM_PAGE);
        registerEventListener(MESSAGE_TOUCH_LISTENER);

        Axis.initPrefs();
    }

    @Override
    public void destroy() {
        unregisterEventListener(MESSAGE_ZOOM_RECT);
        unregisterEventListener(MESSAGE_ZOOM_PAGE);
        unregisterEventListener(MESSAGE_TOUCH_LISTENER);
        mSubscroller.destroy();
        mTouchEventHandler.destroy();
    }

    private final static float easeOut(float t) {
        
        
        t = t-1;
        return -t*t+1;
    }

    private void registerEventListener(String event) {
        mEventDispatcher.registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        mEventDispatcher.unregisterEventListener(event, this);
    }

    private void setState(PanZoomState state) {
        if (state != mState) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("PanZoom:StateChange", state.toString()));
            mState = state;
        }
    }

    private ImmutableViewportMetrics getMetrics() {
        return mTarget.getViewportMetrics();
    }

    
    private void checkMainThread() {
        if (mMainThread != Thread.currentThread()) {
            
            Log.e(LOGTAG, "Uh-oh, we're running on the wrong thread!", new Exception());
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (MESSAGE_ZOOM_RECT.equals(event)) {
                float x = (float)message.getDouble("x");
                float y = (float)message.getDouble("y");
                final RectF zoomRect = new RectF(x, y,
                                     x + (float)message.getDouble("w"),
                                     y + (float)message.getDouble("h"));
                mTarget.post(new Runnable() {
                    @Override
                    public void run() {
                        animatedZoomTo(zoomRect);
                    }
                });
            } else if (MESSAGE_ZOOM_PAGE.equals(event)) {
                ImmutableViewportMetrics metrics = getMetrics();
                RectF cssPageRect = metrics.getCssPageRect();

                RectF viewableRect = metrics.getCssViewport();
                float y = viewableRect.top;
                
                float newHeight = viewableRect.height() * cssPageRect.width() / viewableRect.width();
                float dh = viewableRect.height() - newHeight; 
                final RectF r = new RectF(0.0f,
                                    y + dh/2,
                                    cssPageRect.width(),
                                    y + dh/2 + newHeight);
                mTarget.post(new Runnable() {
                    @Override
                    public void run() {
                        animatedZoomTo(r);
                    }
                });
            } else if (MESSAGE_TOUCH_LISTENER.equals(event)) {
                int tabId = message.getInt("tabID");
                final Tab tab = Tabs.getInstance().getTab(tabId);
                tab.setHasTouchListeners(true);
                mTarget.post(new Runnable() {
                    @Override
                    public void run() {
                        if (Tabs.getInstance().isSelectedTab(tab))
                            mTouchEventHandler.setWaitForTouchListeners(true);
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    
    @Override
    public boolean onKeyEvent(KeyEvent event) {
        if (Build.VERSION.SDK_INT <= 11) {
            return false;
        }

        if ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            && event.getAction() == KeyEvent.ACTION_DOWN) {

            switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_ZOOM_IN:
                return animatedScale(0.2f);
            case KeyEvent.KEYCODE_ZOOM_OUT:
                return animatedScale(-0.2f);
            }
        }
        return false;
    }

    
    @Override
    public boolean onMotionEvent(MotionEvent event) {
        if (Build.VERSION.SDK_INT <= 11) {
            return false;
        }

        switch (event.getSource() & InputDevice.SOURCE_CLASS_MASK) {
        case InputDevice.SOURCE_CLASS_POINTER:
            switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_SCROLL: return handlePointerScroll(event);
            }
            break;
        case InputDevice.SOURCE_CLASS_JOYSTICK:
            switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_MOVE: return handleJoystickScroll(event);
            }
            break;
        }
        return false;
    }

    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return mTouchEventHandler.handleEvent(event);
    }

    boolean handleEvent(MotionEvent event) {
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_DOWN:   return handleTouchStart(event);
        case MotionEvent.ACTION_MOVE:   return handleTouchMove(event);
        case MotionEvent.ACTION_UP:     return handleTouchEnd(event);
        case MotionEvent.ACTION_CANCEL: return handleTouchCancel(event);
        }
        return false;
    }

    
    @Override
    public void notifyDefaultActionPrevented(boolean prevented) {
        mTouchEventHandler.handleEventListenerAction(!prevented);
    }

    
    @Override
    public void abortAnimation() {
        checkMainThread();
        
        
        
        
        switch (mState) {
        case FLING:
            mX.stopFling();
            mY.stopFling();
            
        case BOUNCE:
        case ANIMATED_ZOOM:
            
            
            setState(PanZoomState.NOTHING);
            
        case NOTHING:
            
            
            synchronized (mTarget.getLock()) {
                mTarget.setViewportMetrics(getValidViewportMetrics());
                mTarget.forceRedraw();
            }
            break;
        }
    }

    
    public void startingNewEventBlock(MotionEvent event, boolean waitingForTouchListeners) {
        checkMainThread();
        mSubscroller.cancel();
        if (waitingForTouchListeners && (event.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_DOWN) {
            
            
            
            setState(PanZoomState.WAITING_LISTENERS);
        }
    }

    
    public void preventedTouchFinished() {
        checkMainThread();
        if (mState == PanZoomState.WAITING_LISTENERS) {
            
            
            
            bounce();
        }
    }

    
    @Override
    public void pageRectUpdated() {
        if (mState == PanZoomState.NOTHING) {
            synchronized (mTarget.getLock()) {
                ImmutableViewportMetrics validated = getValidViewportMetrics();
                if (!getMetrics().fuzzyEquals(validated)) {
                    
                    
                    mTarget.setViewportMetrics(validated);
                }
            }
        }
    }

    



    private boolean handleTouchStart(MotionEvent event) {
        
        
        stopAnimationTimer();

        switch (mState) {
        case ANIMATED_ZOOM:
            
            
            
            mTarget.forceRedraw();
            
        case FLING:
        case AUTOSCROLL:
        case BOUNCE:
        case NOTHING:
        case WAITING_LISTENERS:
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
        Log.e(LOGTAG, "Unhandled case " + mState + " in handleTouchStart");
        return false;
    }

    private boolean handleTouchMove(MotionEvent event) {

        switch (mState) {
        case FLING:
        case AUTOSCROLL:
        case BOUNCE:
        case WAITING_LISTENERS:
            
            Log.e(LOGTAG, "Received impossible touch move while in " + mState);
            
        case ANIMATED_ZOOM:
        case NOTHING:
            
            
            return false;

        case TOUCHING:
            
            if (mTarget.isFullScreen() || panDistance(event) < PAN_THRESHOLD) {
                return false;
            }
            cancelTouch();
            startPanning(event.getX(0), event.getY(0), event.getEventTime());
            track(event);
            return true;

        case PANNING_HOLD_LOCKED:
            setState(PanZoomState.PANNING_LOCKED);
            
        case PANNING_LOCKED:
            track(event);
            return true;

        case PANNING_HOLD:
            setState(PanZoomState.PANNING);
            
        case PANNING:
            track(event);
            return true;

        case PINCHING:
            
            return false;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in handleTouchMove");
        return false;
    }

    private boolean handleTouchEnd(MotionEvent event) {

        switch (mState) {
        case FLING:
        case AUTOSCROLL:
        case BOUNCE:
        case WAITING_LISTENERS:
            
            Log.e(LOGTAG, "Received impossible touch end while in " + mState);
            
        case ANIMATED_ZOOM:
        case NOTHING:
            
            
            return false;

        case TOUCHING:
            
            
            
            bounce();
            return false;

        case PANNING:
        case PANNING_LOCKED:
        case PANNING_HOLD:
        case PANNING_HOLD_LOCKED:
            setState(PanZoomState.FLING);
            fling();
            return true;

        case PINCHING:
            setState(PanZoomState.NOTHING);
            return true;
        }
        Log.e(LOGTAG, "Unhandled case " + mState + " in handleTouchEnd");
        return false;
    }

    private boolean handleTouchCancel(MotionEvent event) {
        cancelTouch();

        if (mState == PanZoomState.WAITING_LISTENERS) {
            
            
            
            
            
            
            return false;
        }

        
        bounce();
        return false;
    }

    private boolean handlePointerScroll(MotionEvent event) {
        if (mState == PanZoomState.NOTHING || mState == PanZoomState.FLING) {
            float scrollX = event.getAxisValue(MotionEvent.AXIS_HSCROLL);
            float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL);

            scrollBy(scrollX * MAX_SCROLL, scrollY * MAX_SCROLL);
            bounce();
            return true;
        }
        return false;
    }

    private float normalizeJoystick(float value, InputDevice.MotionRange range) {
        
        
        
        if (Math.abs(value) < 1e-2) {
            return 0;
        }
        
        return value * MAX_SCROLL;
    }

    
    
    private boolean handleJoystickScroll(MotionEvent event) {
        float velocityX = normalizeJoystick(event.getX(0), event.getDevice().getMotionRange(MotionEvent.AXIS_X));
        float velocityY = normalizeJoystick(event.getY(0), event.getDevice().getMotionRange(MotionEvent.AXIS_Y));

        if (velocityX == 0 && velocityY == 0) {
            if (mState == PanZoomState.AUTOSCROLL) {
                bounce(); 
                return true;
            }
            return false;
        }

        if (mState == PanZoomState.NOTHING) {
            setState(PanZoomState.AUTOSCROLL);
            startAnimationTimer(new AutoscrollRunnable());
        }
        if (mState == PanZoomState.AUTOSCROLL) {
            mX.setAutoscrollVelocity(velocityX);
            mY.setAutoscrollVelocity(velocityY);
            return true;
        }
        return false;
    }

    private void startTouch(float x, float y, long time) {
        mX.startTouch(x);
        mY.startTouch(y);
        setState(PanZoomState.TOUCHING);
        mLastEventTime = time;
    }

    private void startPanning(float x, float y, long time) {
        float dx = mX.panDistance(x);
        float dy = mY.panDistance(y);
        double angle = Math.atan2(dy, dx); 
        angle = Math.abs(angle); 

        
        
        mX.startTouch(x);
        mY.startTouch(y);
        mLastEventTime = time;

        if (!mX.scrollable() || !mY.scrollable()) {
            setState(PanZoomState.PANNING);
        } else if (angle < AXIS_LOCK_ANGLE || angle > (Math.PI - AXIS_LOCK_ANGLE)) {
            mY.setScrollingDisabled(true);
            setState(PanZoomState.PANNING_LOCKED);
        } else if (Math.abs(angle - (Math.PI / 2)) < AXIS_LOCK_ANGLE) {
            mX.setScrollingDisabled(true);
            setState(PanZoomState.PANNING_LOCKED);
        } else {
            setState(PanZoomState.PANNING);
        }
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
                setState(PanZoomState.PANNING_HOLD);
            } else if (mState == PanZoomState.PANNING_LOCKED) {
                setState(PanZoomState.PANNING_HOLD_LOCKED);
            } else {
                
                Log.e(LOGTAG, "Impossible case " + mState + " when stopped in track");
                setState(PanZoomState.PANNING_HOLD_LOCKED);
            }
        }

        mX.startPan();
        mY.startPan();
        updatePosition();
    }

    private void scrollBy(float dx, float dy) {
        ImmutableViewportMetrics scrolled = getMetrics().offsetViewportBy(dx, dy);
        mTarget.setViewportMetrics(scrolled);
    }

    private void fling() {
        updatePosition();

        stopAnimationTimer();

        boolean stopped = stopped();
        mX.startFling(stopped);
        mY.startFling(stopped);

        startAnimationTimer(new FlingRunnable());
    }

    
    private void bounce(ImmutableViewportMetrics metrics, PanZoomState state) {
        stopAnimationTimer();

        ImmutableViewportMetrics bounceStartMetrics = getMetrics();
        if (bounceStartMetrics.fuzzyEquals(metrics)) {
            setState(PanZoomState.NOTHING);
            return;
        }

        setState(state);

        
        
        
        
        mTarget.setAnimationTarget(metrics);
        startAnimationTimer(new BounceRunnable(bounceStartMetrics, metrics));
    }

    
    private void bounce() {
        bounce(getValidViewportMetrics(), PanZoomState.BOUNCE);
    }

    
    private void startAnimationTimer(final AnimationRunnable runnable) {
        if (mAnimationTimer != null) {
            Log.e(LOGTAG, "Attempted to start a new timer without canceling the old one!");
            stopAnimationTimer();
        }

        mAnimationTimer = new Timer("Animation Timer");
        mAnimationRunnable = runnable;
        mAnimationTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() { mTarget.post(runnable); }
        }, 0, (int)Axis.MS_PER_FRAME);
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
    }

    private float getVelocity() {
        float xvel = mX.getRealVelocity();
        float yvel = mY.getRealVelocity();
        return FloatMath.sqrt(xvel * xvel + yvel * yvel);
    }

    @Override
    public PointF getVelocityVector() {
        return new PointF(mX.getRealVelocity(), mY.getRealVelocity());
    }

    private boolean stopped() {
        return getVelocity() < STOPPED_THRESHOLD;
    }

    PointF resetDisplacement() {
        return new PointF(mX.resetDisplacement(), mY.resetDisplacement());
    }

    private void updatePosition() {
        mX.displace();
        mY.displace();
        PointF displacement = resetDisplacement();
        if (FloatUtils.fuzzyEquals(displacement.x, 0.0f) && FloatUtils.fuzzyEquals(displacement.y, 0.0f)) {
            return;
        }
        if (! mSubscroller.scrollBy(displacement)) {
            synchronized (mTarget.getLock()) {
                scrollBy(displacement.x, displacement.y);
            }
        }
    }

    private abstract class AnimationRunnable implements Runnable {
        private boolean mAnimationTerminated;

        
        @Override
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

    private class AutoscrollRunnable extends AnimationRunnable {
        protected void animateFrame() {
            if (mState != PanZoomState.AUTOSCROLL) {
                finishAnimation();
                return;
            }

            updatePosition();
        }
    }

    
    private class BounceRunnable extends AnimationRunnable {
        
        private int mBounceFrame;
        



        private ImmutableViewportMetrics mBounceStartMetrics;
        private ImmutableViewportMetrics mBounceEndMetrics;

        BounceRunnable(ImmutableViewportMetrics startMetrics, ImmutableViewportMetrics endMetrics) {
            mBounceStartMetrics = startMetrics;
            mBounceEndMetrics = endMetrics;
        }

        @Override
        protected void animateFrame() {
            




            if (!(mState == PanZoomState.BOUNCE || mState == PanZoomState.ANIMATED_ZOOM)) {
                finishAnimation();
                return;
            }

            
            if (mBounceFrame < (int)(256f/Axis.MS_PER_FRAME)) {
                advanceBounce();
                return;
            }

            
            finishBounce();
            finishAnimation();
            setState(PanZoomState.NOTHING);
        }

        
        private void advanceBounce() {
            synchronized (mTarget.getLock()) {
                float t = easeOut(mBounceFrame * Axis.MS_PER_FRAME / 256f);
                ImmutableViewportMetrics newMetrics = mBounceStartMetrics.interpolate(mBounceEndMetrics, t);
                mTarget.setViewportMetrics(newMetrics);
                mBounceFrame++;
            }
        }

        
        private void finishBounce() {
            synchronized (mTarget.getLock()) {
                mTarget.setViewportMetrics(mBounceEndMetrics);
                mBounceFrame = -1;
            }
        }
    }

    
    private class FlingRunnable extends AnimationRunnable {
        @Override
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
                setState(PanZoomState.NOTHING);
            }
        }
    }

    private void finishAnimation() {
        checkMainThread();

        stopAnimationTimer();

        
        mTarget.forceRedraw();
    }

    
    private ImmutableViewportMetrics getValidViewportMetrics() {
        return getValidViewportMetrics(getMetrics());
    }

    private ImmutableViewportMetrics getValidViewportMetrics(ImmutableViewportMetrics viewportMetrics) {
        
        float zoomFactor = viewportMetrics.zoomFactor;
        RectF pageRect = viewportMetrics.getPageRect();
        RectF viewport = viewportMetrics.getViewport();

        float focusX = viewport.width() / 2.0f;
        float focusY = viewport.height() / 2.0f;

        float minZoomFactor = 0.0f;
        float maxZoomFactor = MAX_ZOOM;

        ZoomConstraints constraints = mTarget.getZoomConstraints();

        if (constraints.getMinZoom() > 0)
            minZoomFactor = constraints.getMinZoom();
        if (constraints.getMaxZoom() > 0)
            maxZoomFactor = constraints.getMaxZoom();

        if (!constraints.getAllowZoom()) {
            
            maxZoomFactor = minZoomFactor = constraints.getDefaultZoom();
        }

        
        if (pageRect.width() > 0) {
            float scaleFactor = viewport.width() / pageRect.width();
            minZoomFactor = Math.max(minZoomFactor, zoomFactor * scaleFactor);
            if (viewport.width() > pageRect.width())
                focusX = 0.0f;
        }
        if (pageRect.height() > 0) {
            float scaleFactor = viewport.height() / pageRect.height();
            minZoomFactor = Math.max(minZoomFactor, zoomFactor * scaleFactor);
            if (viewport.height() > pageRect.height())
                focusY = 0.0f;
        }

        maxZoomFactor = Math.max(maxZoomFactor, minZoomFactor);

        if (zoomFactor < minZoomFactor) {
            
            
            
            
            
            PointF center = new PointF(focusX, focusY);
            viewportMetrics = viewportMetrics.scaleTo(minZoomFactor, center);
        } else if (zoomFactor > maxZoomFactor) {
            PointF center = new PointF(viewport.width() / 2.0f, viewport.height() / 2.0f);
            viewportMetrics = viewportMetrics.scaleTo(maxZoomFactor, center);
        }

        
        viewportMetrics = viewportMetrics.clampWithMargins();

        return viewportMetrics;
    }

    private class AxisX extends Axis {
        AxisX(SubdocumentScrollHelper subscroller) { super(subscroller); }
        @Override
        public float getOrigin() { return getMetrics().viewportRectLeft; }
        @Override
        protected float getViewportLength() { return getMetrics().getWidth(); }
        @Override
        protected float getPageStart() {
            ImmutableViewportMetrics metrics = getMetrics();
            return metrics.pageRectLeft - metrics.fixedLayerMarginLeft;
        }
        @Override
        protected float getPageLength() {
            ImmutableViewportMetrics metrics = getMetrics();
            return metrics.getPageWidth() + metrics.fixedLayerMarginLeft + metrics.fixedLayerMarginRight;
        }
    }

    private class AxisY extends Axis {
        AxisY(SubdocumentScrollHelper subscroller) { super(subscroller); }
        @Override
        public float getOrigin() { return getMetrics().viewportRectTop; }
        @Override
        protected float getViewportLength() { return getMetrics().getHeight(); }
        @Override
        protected float getPageStart() {
            ImmutableViewportMetrics metrics = getMetrics();
            return metrics.pageRectTop - metrics.fixedLayerMarginTop;
        }
        @Override
        protected float getPageLength() {
            ImmutableViewportMetrics metrics = getMetrics();
            return metrics.getPageHeight() + metrics.fixedLayerMarginTop + metrics.fixedLayerMarginBottom;
        }
    }

    


    @Override
    public boolean onScaleBegin(SimpleScaleGestureDetector detector) {
        if (mState == PanZoomState.ANIMATED_ZOOM)
            return false;

        if (!mTarget.getZoomConstraints().getAllowZoom())
            return false;

        setState(PanZoomState.PINCHING);
        mLastZoomFocus = new PointF(detector.getFocusX(), detector.getFocusY());
        cancelTouch();

        GeckoAppShell.sendEventToGecko(GeckoEvent.createNativeGestureEvent(GeckoEvent.ACTION_MAGNIFY_START, mLastZoomFocus, getMetrics().zoomFactor));

        return true;
    }

    @Override
    public boolean onScale(SimpleScaleGestureDetector detector) {
        if (mTarget.isFullScreen())
            return false;

        if (mState != PanZoomState.PINCHING)
            return false;

        float prevSpan = detector.getPreviousSpan();
        if (FloatUtils.fuzzyEquals(prevSpan, 0.0f)) {
            
            return true;
        }

        synchronized (mTarget.getLock()) {
            float zoomFactor = getAdjustedZoomFactor(detector.getCurrentSpan() / prevSpan);
            scrollBy(mLastZoomFocus.x - detector.getFocusX(),
                     mLastZoomFocus.y - detector.getFocusY());
            mLastZoomFocus.set(detector.getFocusX(), detector.getFocusY());
            ImmutableViewportMetrics target = getMetrics().scaleTo(zoomFactor, mLastZoomFocus);
            mTarget.setViewportMetrics(target);
        }

        GeckoEvent event = GeckoEvent.createNativeGestureEvent(GeckoEvent.ACTION_MAGNIFY, mLastZoomFocus, getMetrics().zoomFactor);
        GeckoAppShell.sendEventToGecko(event);

        return true;
    }

    private boolean animatedScale(float zoomDelta) {
        if (mState != PanZoomState.NOTHING && mState != PanZoomState.BOUNCE) {
            return false;
        }
        synchronized (mTarget.getLock()) {
            ImmutableViewportMetrics metrics = getMetrics();
            float oldZoom = metrics.zoomFactor;
            float newZoom = oldZoom + zoomDelta;
            float adjustedZoom = getAdjustedZoomFactor(newZoom / oldZoom);
            PointF center = new PointF(metrics.getWidth() / 2.0f, metrics.getHeight() / 2.0f);
            metrics = metrics.scaleTo(adjustedZoom, center);
            bounce(getValidViewportMetrics(metrics), PanZoomState.BOUNCE);
        }
        return true;
    }

    private float getAdjustedZoomFactor(float zoomRatio) {
        



        float resistance = Math.min(mX.getEdgeResistance(true), mY.getEdgeResistance(true));
        if (zoomRatio > 1.0f)
            zoomRatio = 1.0f + (zoomRatio - 1.0f) * resistance;
        else
            zoomRatio = 1.0f - (1.0f - zoomRatio) * resistance;

        float newZoomFactor = getMetrics().zoomFactor * zoomRatio;
        float minZoomFactor = 0.0f;
        float maxZoomFactor = MAX_ZOOM;

        ZoomConstraints constraints = mTarget.getZoomConstraints();

        if (constraints.getMinZoom() > 0)
            minZoomFactor = constraints.getMinZoom();
        if (constraints.getMaxZoom() > 0)
            maxZoomFactor = constraints.getMaxZoom();

        if (newZoomFactor < minZoomFactor) {
            
            
            
            final float rate = 0.5f; 
            float excessZoom = minZoomFactor - newZoomFactor;
            excessZoom = 1.0f - (float)Math.exp(-excessZoom * rate);
            newZoomFactor = minZoomFactor * (1.0f - excessZoom / 2.0f);
        }

        if (newZoomFactor > maxZoomFactor) {
            
            
            
            float excessZoom = newZoomFactor - maxZoomFactor;
            excessZoom = 1.0f - (float)Math.exp(-excessZoom);
            newZoomFactor = maxZoomFactor + excessZoom;
        }

        return newZoomFactor;
    }

    @Override
    public void onScaleEnd(SimpleScaleGestureDetector detector) {
        if (mState == PanZoomState.ANIMATED_ZOOM)
            return;

        
        startTouch(detector.getFocusX(), detector.getFocusY(), detector.getEventTime());

        
        mTarget.forceRedraw();

        PointF point = new PointF(detector.getFocusX(), detector.getFocusY());
        GeckoEvent event = GeckoEvent.createNativeGestureEvent(GeckoEvent.ACTION_MAGNIFY_END, point, getMetrics().zoomFactor);

        if (event == null) {
            return;
        }

        GeckoAppShell.sendEventToGecko(event);
    }

    @Override
    public boolean getRedrawHint() {
        switch (mState) {
            case PINCHING:
            case ANIMATED_ZOOM:
            case BOUNCE:
                
                
                
                return false;
            default:
                
                return true;
        }
    }

    private void sendPointToGecko(String event, MotionEvent motionEvent) {
        String json;
        try {
            PointF point = new PointF(motionEvent.getX(), motionEvent.getY());
            point = mTarget.convertViewPointToLayerPoint(point);
            if (point == null) {
                return;
            }
            json = PointUtils.toJSON(point).toString();
        } catch (Exception e) {
            Log.e(LOGTAG, "Unable to convert point to JSON for " + event, e);
            return;
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(event, json));
    }

    @Override
    public void onLongPress(MotionEvent motionEvent) {
        sendPointToGecko("Gesture:LongPress", motionEvent);
    }

    @Override
    public boolean onSingleTapUp(MotionEvent motionEvent) {
        
        if (!mTarget.getZoomConstraints().getAllowZoom()) {
            sendPointToGecko("Gesture:SingleTap", motionEvent);
        }
        
        return false;
    }

    @Override
    public boolean onSingleTapConfirmed(MotionEvent motionEvent) {
        
        if (mTarget.getZoomConstraints().getAllowZoom()) {
            sendPointToGecko("Gesture:SingleTap", motionEvent);
        }
        return true;
    }

    @Override
    public boolean onDoubleTap(MotionEvent motionEvent) {
        if (mTarget.getZoomConstraints().getAllowZoom()) {
            sendPointToGecko("Gesture:DoubleTap", motionEvent);
        }
        return true;
    }

    private void cancelTouch() {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Gesture:CancelTouch", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    





    private boolean animatedZoomTo(RectF zoomToRect) {
        final float startZoom = getMetrics().zoomFactor;

        RectF viewport = getMetrics().getViewport();
        
        
        
        
        float targetRatio = viewport.width() / viewport.height();
        float rectRatio = zoomToRect.width() / zoomToRect.height();
        if (FloatUtils.fuzzyEquals(targetRatio, rectRatio)) {
            
        } else if (targetRatio < rectRatio) {
            
            float newHeight = zoomToRect.width() / targetRatio;
            zoomToRect.top -= (newHeight - zoomToRect.height()) / 2;
            zoomToRect.bottom = zoomToRect.top + newHeight;
        } else { 
            
            float newWidth = targetRatio * zoomToRect.height();
            zoomToRect.left -= (newWidth - zoomToRect.width()) / 2;
            zoomToRect.right = zoomToRect.left + newWidth;
        }

        float finalZoom = viewport.width() / zoomToRect.width();

        ImmutableViewportMetrics finalMetrics = getMetrics();
        finalMetrics = finalMetrics.setViewportOrigin(
            zoomToRect.left * finalMetrics.zoomFactor,
            zoomToRect.top * finalMetrics.zoomFactor);
        finalMetrics = finalMetrics.scaleTo(finalZoom, new PointF(0.0f, 0.0f));

        
        
        finalMetrics = getValidViewportMetrics(finalMetrics);

        bounce(finalMetrics, PanZoomState.ANIMATED_ZOOM);
        return true;
    }

    
    @Override
    public void abortPanning() {
        checkMainThread();
        bounce();
    }

    @Override
    public void setOverScrollMode(int overscrollMode) {
        mX.setOverScrollMode(overscrollMode);
        mY.setOverScrollMode(overscrollMode);
    }

    @Override
    public int getOverScrollMode() {
        return mX.getOverScrollMode();
    }
}
