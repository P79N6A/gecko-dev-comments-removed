




package org.mozilla.gecko.gfx;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.graphics.PointF;
import android.graphics.RectF;
import android.util.FloatMath;
import android.util.Log;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;







class JavaPanZoomController
    extends GestureDetector.SimpleOnGestureListener
    implements PanZoomController, SimpleScaleGestureDetector.SimpleScaleGestureListener, GeckoEventListener
{
    private static final String LOGTAG = "GeckoPanZoomController";

    private static final String MESSAGE_ZOOM_RECT = "Browser:ZoomToRect";
    private static final String MESSAGE_ZOOM_PAGE = "Browser:ZoomToPageWidth";
    private static final String MESSAGE_TOUCH_LISTENER = "Tab:HasTouchListener";

    
    private static final float STOPPED_THRESHOLD = 4.0f;

    
    private static final float FLING_STOPPED_THRESHOLD = 0.1f;

    
    private static final double AXIS_LOCK_ANGLE = Math.PI / 6.0; 

    
    private static final double AXIS_BREAKOUT_ANGLE = Math.PI / 8.0;

    
    public static final float AXIS_BREAKOUT_THRESHOLD = 1/32f * GeckoAppShell.getDpi();

    
    private static final float MAX_ZOOM = 4.0f;

    
    private static final float MAX_SCROLL = 0.075f * GeckoAppShell.getDpi();

    
    private static final float MAX_ZOOM_DELTA = 0.125f;

    
    private static final int BOUNCE_ANIMATION_DURATION = 250000000;

    private enum PanZoomState {
        NOTHING,                
        FLING,                  
        TOUCHING,               
        PANNING_LOCKED_X,       
        PANNING_LOCKED_Y,       
        PANNING,                
        PANNING_HOLD,           

        PANNING_HOLD_LOCKED_X,  
        PANNING_HOLD_LOCKED_Y,  
        PINCHING,               
        ANIMATED_ZOOM,          
        BOUNCE,                 
        WAITING_LISTENERS,      


        AUTONAV,                


    }

    private enum AxisLockMode {
        STANDARD,       
        FREE,           
        STICKY          
    }

    private final PanZoomTarget mTarget;
    private final SubdocumentScrollHelper mSubscroller;
    private final Axis mX;
    private final Axis mY;
    private final TouchEventHandler mTouchEventHandler;
    private final EventDispatcher mEventDispatcher;

    
    private PanZoomRenderTask mAnimationRenderTask;
    
    private PointF mLastZoomFocus;
    
    private long mLastEventTime;
    
    private PanZoomState mState;
    
    private float mAutonavZoomDelta;
    
    private AxisLockMode mMode;
    
    private boolean mWaitForDoubleTap;
    
    private boolean mNegateWheelScrollY;
    
    private boolean mDefaultPrevented;
    
    private boolean isLongpressEnabled;
    
    private boolean mIgnoreLongPress;

    
    private Overscroll mOverscroll;

    public JavaPanZoomController(PanZoomTarget target, View view, EventDispatcher eventDispatcher) {
        mTarget = target;
        mSubscroller = new SubdocumentScrollHelper(eventDispatcher);
        mX = new AxisX(mSubscroller);
        mY = new AxisY(mSubscroller);
        mTouchEventHandler = new TouchEventHandler(view.getContext(), view, this);
        isLongpressEnabled = true;

        checkMainThread();

        setState(PanZoomState.NOTHING);

        mEventDispatcher = eventDispatcher;
        mEventDispatcher.registerGeckoThreadListener(this,
            MESSAGE_ZOOM_RECT,
            MESSAGE_ZOOM_PAGE,
            MESSAGE_TOUCH_LISTENER);

        mMode = AxisLockMode.STANDARD;

        String[] prefs = { "ui.scrolling.axis_lock_mode",
                           "ui.scrolling.negate_wheel_scrollY",
                           "ui.scrolling.gamepad_dead_zone" };
        PrefsHelper.getPrefs(prefs, new PrefsHelper.PrefHandlerBase() {
            @Override public void prefValue(String pref, String value) {
                if (pref.equals("ui.scrolling.axis_lock_mode")) {
                    if (value.equals("standard")) {
                        mMode = AxisLockMode.STANDARD;
                    } else if (value.equals("free")) {
                        mMode = AxisLockMode.FREE;
                    } else {
                        mMode = AxisLockMode.STICKY;
                    }
                }
            }

            @Override public void prefValue(String pref, int value) {
                if (pref.equals("ui.scrolling.gamepad_dead_zone")) {
                    GamepadUtils.overrideDeadZoneThreshold(value / 1000f);
                }
            }

            @Override public void prefValue(String pref, boolean value) {
                if (pref.equals("ui.scrolling.negate_wheel_scrollY")) {
                    mNegateWheelScrollY = value;
                }
            }

            @Override
            public boolean isObserver() {
                return true;
            }

        });

        Axis.initPrefs();
    }

    @Override
    public void destroy() {
        mEventDispatcher.unregisterGeckoThreadListener(this,
            MESSAGE_ZOOM_RECT,
            MESSAGE_ZOOM_PAGE,
            MESSAGE_TOUCH_LISTENER);
        mSubscroller.destroy();
        mTouchEventHandler.destroy();
    }

    private final static float easeOut(float t) {
        
        
        t = t-1;
        return -t*t+1;
    }

    private void setState(PanZoomState state) {
        if (state != mState) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("PanZoom:StateChange", state.toString()));
            mState = state;

            
            if (state == PanZoomState.NOTHING) {
                mTarget.panZoomStopped();
            }
        }
    }

    private ImmutableViewportMetrics getMetrics() {
        return mTarget.getViewportMetrics();
    }

    private void checkMainThread() {
        if (!ThreadUtils.isOnUiThread()) {
            
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
                if (message.optBoolean("animate", true)) {
                    mTarget.post(new Runnable() {
                        @Override
                        public void run() {
                            animatedZoomTo(zoomRect);
                        }
                    });
                } else {
                    mTarget.setViewportMetrics(getMetricsToZoomTo(zoomRect));
                }
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
                if (message.optBoolean("animate", true)) {
                    mTarget.post(new Runnable() {
                        @Override
                        public void run() {
                            animatedZoomTo(r);
                        }
                    });
                } else {
                    mTarget.setViewportMetrics(getMetricsToZoomTo(r));
                }
            } else if (MESSAGE_TOUCH_LISTENER.equals(event)) {
                int tabId = message.getInt("tabID");
                final Tab tab = Tabs.getInstance().getTab(tabId);
                
                if (tab == null) {
                    return;
                }

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
        if (Versions.preHCMR1) {
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
        if (Versions.preHCMR1) {
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
            case MotionEvent.ACTION_MOVE: return handleJoystickNav(event);
            }
            break;
        }
        return false;
    }

    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return mTouchEventHandler.handleEvent(event);
    }

    boolean handleEvent(MotionEvent event, boolean defaultPrevented) {
        mDefaultPrevented = defaultPrevented;

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
                mTarget.forceRedraw(null);
            }
            break;
        }
    }

    
    public void startingNewEventBlock(MotionEvent event, boolean waitingForTouchListeners) {
        checkMainThread();
        mSubscroller.cancel();
        mIgnoreLongPress = false;
        if (waitingForTouchListeners) {
            if ((event.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_DOWN) {
                
                
                
                setState(PanZoomState.WAITING_LISTENERS);
            } else if ((event.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_POINTER_DOWN) {
                
                
                
                
                
                
                mIgnoreLongPress = true;
            }
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
        
        
        stopAnimationTask();

        switch (mState) {
        case ANIMATED_ZOOM:
            
            
            
            mTarget.forceRedraw(null);
            
        case FLING:
        case AUTONAV:
        case BOUNCE:
        case NOTHING:
        case WAITING_LISTENERS:
            startTouch(event.getX(0), event.getY(0), event.getEventTime());
            return false;
        case TOUCHING:
        case PANNING:
        case PANNING_LOCKED_X:
        case PANNING_LOCKED_Y:
        case PANNING_HOLD:
        case PANNING_HOLD_LOCKED_X:
        case PANNING_HOLD_LOCKED_Y:
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
        case AUTONAV:
        case BOUNCE:
        case WAITING_LISTENERS:
            
            Log.e(LOGTAG, "Received impossible touch move while in " + mState);
            
        case ANIMATED_ZOOM:
        case NOTHING:
            
            
            return false;

        case TOUCHING:
            
            if (mTarget.getFullScreenState() == FullScreenState.NON_ROOT_ELEMENT && !mSubscroller.scrolling()) {
                return false;
            }
            if (panDistance(event) < PanZoomController.PAN_THRESHOLD) {
                return false;
            }
            cancelTouch();
            startPanning(event.getX(0), event.getY(0), event.getEventTime());
            track(event);
            return true;

        case PANNING_HOLD_LOCKED_X:
            setState(PanZoomState.PANNING_LOCKED_X);
            track(event);
            return true;
        case PANNING_HOLD_LOCKED_Y:
            setState(PanZoomState.PANNING_LOCKED_Y);
            
        case PANNING_LOCKED_X:
        case PANNING_LOCKED_Y:
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
        case AUTONAV:
        case BOUNCE:
        case ANIMATED_ZOOM:
        case NOTHING:
            
            
            return false;

        case WAITING_LISTENERS:
            if (!mDefaultPrevented) {
              
              Log.e(LOGTAG, "Received impossible touch end while in " + mState);
            }
            
        case TOUCHING:
            
            
            
            bounce();
            return false;

        case PANNING:
        case PANNING_LOCKED_X:
        case PANNING_LOCKED_Y:
        case PANNING_HOLD:
        case PANNING_HOLD_LOCKED_X:
        case PANNING_HOLD_LOCKED_Y:
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

        
        bounce();
        return false;
    }

    private boolean handlePointerScroll(MotionEvent event) {
        if (mState == PanZoomState.NOTHING || mState == PanZoomState.FLING) {
            float scrollX = event.getAxisValue(MotionEvent.AXIS_HSCROLL);
            float scrollY = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
            if (mNegateWheelScrollY) {
                scrollY *= -1.0;
            }
            scrollBy(scrollX * MAX_SCROLL, scrollY * MAX_SCROLL);
            bounce();
            return true;
        }
        return false;
    }

    private float filterDeadZone(MotionEvent event, int axis) {
        return (GamepadUtils.isValueInDeadZone(event, axis) ? 0 : event.getAxisValue(axis));
    }

    private float normalizeJoystickScroll(MotionEvent event, int axis) {
        return filterDeadZone(event, axis) * MAX_SCROLL;
    }

    private float normalizeJoystickZoom(MotionEvent event, int axis) {
        
        return filterDeadZone(event, axis) * -MAX_ZOOM_DELTA;
    }

    
    
    private boolean handleJoystickNav(MotionEvent event) {
        float velocityX = normalizeJoystickScroll(event, MotionEvent.AXIS_X);
        float velocityY = normalizeJoystickScroll(event, MotionEvent.AXIS_Y);
        float zoomDelta = normalizeJoystickZoom(event, MotionEvent.AXIS_RZ);

        if (velocityX == 0 && velocityY == 0 && zoomDelta == 0) {
            if (mState == PanZoomState.AUTONAV) {
                bounce(); 
                return true;
            }
            return false;
        }

        if (mState == PanZoomState.NOTHING) {
            setState(PanZoomState.AUTONAV);
            startAnimationRenderTask(new AutonavRenderTask());
        }
        if (mState == PanZoomState.AUTONAV) {
            mX.setAutoscrollVelocity(velocityX);
            mY.setAutoscrollVelocity(velocityY);
            mAutonavZoomDelta = zoomDelta;
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

        if (mMode == AxisLockMode.STANDARD || mMode == AxisLockMode.STICKY) {
            if (!mX.scrollable() || !mY.scrollable()) {
                setState(PanZoomState.PANNING);
            } else if (angle < AXIS_LOCK_ANGLE || angle > (Math.PI - AXIS_LOCK_ANGLE)) {
                mY.setScrollingDisabled(true);
                setState(PanZoomState.PANNING_LOCKED_X);
            } else if (Math.abs(angle - (Math.PI / 2)) < AXIS_LOCK_ANGLE) {
                mX.setScrollingDisabled(true);
                setState(PanZoomState.PANNING_LOCKED_Y);
            } else {
                setState(PanZoomState.PANNING);
            }
        } else if (mMode == AxisLockMode.FREE) {
            setState(PanZoomState.PANNING);
        }
    }

    private float panDistance(MotionEvent move) {
        float dx = mX.panDistance(move.getX(0));
        float dy = mY.panDistance(move.getY(0));
        return FloatMath.sqrt(dx * dx + dy * dy);
    }

    private void track(float x, float y, long time) {
        float timeDelta = (time - mLastEventTime);
        if (FloatUtils.fuzzyEquals(timeDelta, 0)) {
            
            
            return;
        }
        mLastEventTime = time;


        
        if (mMode == AxisLockMode.STICKY) {
            float dx = mX.panDistance(x);
            float dy = mY.panDistance(y);
            double angle = Math.atan2(dy, dx); 
            angle = Math.abs(angle); 

            if (Math.abs(dx) > AXIS_BREAKOUT_THRESHOLD || Math.abs(dy) > AXIS_BREAKOUT_THRESHOLD) {
                if (mState == PanZoomState.PANNING_LOCKED_X) {
                    if (angle > AXIS_BREAKOUT_ANGLE && angle < (Math.PI - AXIS_BREAKOUT_ANGLE)) {
                        mY.setScrollingDisabled(false);
                        setState(PanZoomState.PANNING);
                    }
                 } else if (mState == PanZoomState.PANNING_LOCKED_Y) {
                    if (Math.abs(angle - (Math.PI / 2)) > AXIS_BREAKOUT_ANGLE) {
                        mX.setScrollingDisabled(false);
                        setState(PanZoomState.PANNING);
                    }
                }
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
                setState(PanZoomState.PANNING_HOLD);
            } else if (mState == PanZoomState.PANNING_LOCKED_X) {
                setState(PanZoomState.PANNING_HOLD_LOCKED_X);
            } else if (mState == PanZoomState.PANNING_LOCKED_Y) {
                setState(PanZoomState.PANNING_HOLD_LOCKED_Y);
            } else {
                
                Log.e(LOGTAG, "Impossible case " + mState + " when stopped in track");
                setState(PanZoomState.PANNING_HOLD);
            }
        }

        mX.startPan();
        mY.startPan();
        updatePosition();
    }

    private void scrollBy(float dx, float dy) {
        mTarget.scrollBy(dx, dy);
    }

    private void fling() {
        updatePosition();

        stopAnimationTask();

        boolean stopped = stopped();
        mX.startFling(stopped);
        mY.startFling(stopped);

        startAnimationRenderTask(new FlingRenderTask());
    }

    
    private void bounce(ImmutableViewportMetrics metrics, PanZoomState state) {
        stopAnimationTask();

        ImmutableViewportMetrics bounceStartMetrics = getMetrics();
        if (bounceStartMetrics.fuzzyEquals(metrics)) {
            setState(PanZoomState.NOTHING);
            return;
        }

        setState(state);

        
        
        
        
        mTarget.setAnimationTarget(metrics);
        startAnimationRenderTask(new BounceRenderTask(bounceStartMetrics, metrics));
    }

    
    private void bounce() {
        bounce(getValidViewportMetrics(), PanZoomState.BOUNCE);
    }

    
    private void startAnimationRenderTask(final PanZoomRenderTask task) {
        if (mAnimationRenderTask != null) {
            Log.e(LOGTAG, "Attempted to start a new task without canceling the old one!");
            stopAnimationTask();
        }

        mAnimationRenderTask = task;
        mTarget.postRenderTask(mAnimationRenderTask);
    }

    
    private void stopAnimationTask() {
        if (mAnimationRenderTask != null) {
            mAnimationRenderTask.terminate();
            mTarget.removeRenderTask(mAnimationRenderTask);
            mAnimationRenderTask = null;
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
        if (mDefaultPrevented || mSubscroller.scrollBy(displacement)) {
            synchronized (mTarget.getLock()) {
                mTarget.scrollMarginsBy(displacement.x, displacement.y);
            }
        } else {
            synchronized (mTarget.getLock()) {
                scrollBy(displacement.x, displacement.y);
            }
        }
    }

    



    private abstract class PanZoomRenderTask extends RenderTask {

        


        protected long mCurrentFrameStartTime;
        


        protected long mLastFrameTimeDelta;

        private final Runnable mRunnable = new Runnable() {
            @Override
            public final void run() {
                if (mContinueAnimation) {
                    animateFrame();
                }
            }
        };

        private boolean mContinueAnimation = true;

        public PanZoomRenderTask() {
            super(false);
        }

        @Override
        protected final boolean internalRun(long timeDelta, long currentFrameStartTime) {

            mCurrentFrameStartTime = currentFrameStartTime;
            mLastFrameTimeDelta = timeDelta;

            mTarget.post(mRunnable);
            return mContinueAnimation;
        }

        


        protected abstract void animateFrame();

        


        public void terminate() {
            mContinueAnimation = false;
        }
    }

    private class AutonavRenderTask extends PanZoomRenderTask {
        public AutonavRenderTask() {
            super();
        }

        @Override
        protected void animateFrame() {
            if (mState != PanZoomState.AUTONAV) {
                finishAnimation();
                return;
            }

            updatePosition();
            synchronized (mTarget.getLock()) {
                mTarget.setViewportMetrics(applyZoomDelta(getMetrics(), mAutonavZoomDelta));
            }
        }
    }

    
    private class BounceRenderTask extends PanZoomRenderTask {

        



        private final ImmutableViewportMetrics mBounceStartMetrics;
        private final ImmutableViewportMetrics mBounceEndMetrics;
        
        private long mBounceDuration;

        BounceRenderTask(ImmutableViewportMetrics startMetrics, ImmutableViewportMetrics endMetrics) {
            super();
            mBounceStartMetrics = startMetrics;
            mBounceEndMetrics = endMetrics;
        }

        @Override
        protected void animateFrame() {
            




            if (!(mState == PanZoomState.BOUNCE || mState == PanZoomState.ANIMATED_ZOOM)) {
                finishAnimation();
                return;
            }

            
            mBounceDuration = mCurrentFrameStartTime - getStartTime();
            if (mBounceDuration < BOUNCE_ANIMATION_DURATION) {
                advanceBounce();
                return;
            }

            
            finishBounce();
            finishAnimation();
            setState(PanZoomState.NOTHING);
        }

        
        private void advanceBounce() {
            synchronized (mTarget.getLock()) {
                float t = easeOut((float)mBounceDuration / BOUNCE_ANIMATION_DURATION);
                ImmutableViewportMetrics newMetrics = mBounceStartMetrics.interpolate(mBounceEndMetrics, t);
                mTarget.setViewportMetrics(newMetrics);
            }
        }

        
        private void finishBounce() {
            synchronized (mTarget.getLock()) {
                mTarget.setViewportMetrics(mBounceEndMetrics);
            }
        }
    }

    
    private class FlingRenderTask extends PanZoomRenderTask {

        public FlingRenderTask() {
            super();
        }

        @Override
        protected void animateFrame() {
            




            if (mState != PanZoomState.FLING) {
                finishAnimation();
                return;
            }

            
            boolean flingingX = mX.advanceFling(mLastFrameTimeDelta);
            boolean flingingY = mY.advanceFling(mLastFrameTimeDelta);

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

        stopAnimationTask();

        
        mTarget.forceRedraw(null);
    }

    
    private ImmutableViewportMetrics getValidViewportMetrics() {
        return getValidViewportMetrics(getMetrics());
    }

    private ImmutableViewportMetrics getValidViewportMetrics(ImmutableViewportMetrics viewportMetrics) {
        
        float zoomFactor = viewportMetrics.zoomFactor;
        RectF pageRect = viewportMetrics.getPageRect();
        RectF viewport = viewportMetrics.getViewport();
        RectF maxMargins = mTarget.getMaxMargins();
        RectF margins = new RectF(Math.max(maxMargins.left, viewportMetrics.marginLeft),
                                  Math.max(maxMargins.top, viewportMetrics.marginTop),
                                  Math.max(maxMargins.right, viewportMetrics.marginRight),
                                  Math.max(maxMargins.bottom, viewportMetrics.marginBottom));

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
            float pageWidth = pageRect.width() + margins.left + margins.right;
            float scaleFactor = viewport.width() / pageWidth;
            minZoomFactor = Math.max(minZoomFactor, zoomFactor * scaleFactor);
            if (viewport.width() > pageWidth)
                focusX = 0.0f;
        }
        if (pageRect.height() > 0) {
            float pageHeight = pageRect.height() + margins.top + margins.bottom;
            float scaleFactor = viewport.height() / pageHeight;
            minZoomFactor = Math.max(minZoomFactor, zoomFactor * scaleFactor);
            if (viewport.height() > pageHeight)
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
        protected float getPageStart() { return getMetrics().pageRectLeft; }
        @Override
        protected float getMarginStart() { return mTarget.getMaxMargins().left - getMetrics().marginLeft; }
        @Override
        protected float getMarginEnd() { return mTarget.getMaxMargins().right - getMetrics().marginRight; }
        @Override
        protected float getPageLength() { return getMetrics().getPageWidthWithMargins(); }
        @Override
        protected boolean marginsHidden() {
            ImmutableViewportMetrics metrics = getMetrics();
            RectF maxMargins = mTarget.getMaxMargins();
            return (metrics.marginLeft < maxMargins.left || metrics.marginRight < maxMargins.right);
        }
        @Override
        protected void overscrollFling(final float velocity) {
            if (mOverscroll != null) {
                mOverscroll.setVelocity(velocity, Overscroll.Axis.X);
            }
        }
        @Override
        protected void overscrollPan(final float distance) {
            if (mOverscroll != null) {
                mOverscroll.setDistance(distance, Overscroll.Axis.X);
            }
        }
    }

    private class AxisY extends Axis {
        AxisY(SubdocumentScrollHelper subscroller) { super(subscroller); }
        @Override
        public float getOrigin() { return getMetrics().viewportRectTop; }
        @Override
        protected float getViewportLength() { return getMetrics().getHeight(); }
        @Override
        protected float getPageStart() { return getMetrics().pageRectTop; }
        @Override
        protected float getPageLength() { return getMetrics().getPageHeightWithMargins(); }
        @Override
        protected float getMarginStart() { return mTarget.getMaxMargins().top - getMetrics().marginTop; }
        @Override
        protected float getMarginEnd() { return mTarget.getMaxMargins().bottom - getMetrics().marginBottom; }
        @Override
        protected boolean marginsHidden() {
            ImmutableViewportMetrics metrics = getMetrics();
            RectF maxMargins = mTarget.getMaxMargins();
            return (metrics.marginTop < maxMargins.top || metrics.marginBottom < maxMargins.bottom);
        }
        @Override
        protected void overscrollFling(final float velocity) {
            if (mOverscroll != null) {
                mOverscroll.setVelocity(velocity, Overscroll.Axis.Y);
            }
        }
        @Override
        protected void overscrollPan(final float distance) {
            if (mOverscroll != null) {
                mOverscroll.setDistance(distance, Overscroll.Axis.Y);
            }
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
        if (mTarget.getFullScreenState() != FullScreenState.NONE)
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

            
            if (mX.getOverScrollMode() == View.OVER_SCROLL_NEVER || mY.getOverScrollMode() == View.OVER_SCROLL_NEVER) {
                target = getValidViewportMetrics(target);
            }
            mTarget.setViewportMetrics(target);
        }

        GeckoEvent event = GeckoEvent.createNativeGestureEvent(GeckoEvent.ACTION_MAGNIFY, mLastZoomFocus, getMetrics().zoomFactor);
        GeckoAppShell.sendEventToGecko(event);

        return true;
    }

    private ImmutableViewportMetrics applyZoomDelta(ImmutableViewportMetrics metrics, float zoomDelta) {
        float oldZoom = metrics.zoomFactor;
        float newZoom = oldZoom + zoomDelta;
        float adjustedZoom = getAdjustedZoomFactor(newZoom / oldZoom);
        
        PointF center = new PointF(metrics.getWidth() / 2.0f, metrics.getHeight() / 2.0f);
        metrics = metrics.scaleTo(adjustedZoom, center);
        return metrics;
    }

    private boolean animatedScale(float zoomDelta) {
        if (mState != PanZoomState.NOTHING && mState != PanZoomState.BOUNCE) {
            return false;
        }
        synchronized (mTarget.getLock()) {
            ImmutableViewportMetrics metrics = applyZoomDelta(getMetrics(), zoomDelta);
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

        
        mTarget.forceRedraw(null);

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
    public boolean onDown(MotionEvent motionEvent) {
        mWaitForDoubleTap = mTarget.getZoomConstraints().getAllowDoubleTapZoom();
        return false;
    }

    @Override
    public void onShowPress(MotionEvent motionEvent) {
        
        
        
        
        
        
        mWaitForDoubleTap = false;
    }

    



    public void setIsLongpressEnabled(boolean isLongpressEnabled) {
        this.isLongpressEnabled = isLongpressEnabled;
    }

    @Override
    public void onLongPress(MotionEvent motionEvent) {
        if (!isLongpressEnabled || mIgnoreLongPress) {
            return;
        }

        GeckoEvent e = GeckoEvent.createLongPressEvent(motionEvent);
        GeckoAppShell.sendEventToGecko(e);
    }

    @Override
    public boolean onSingleTapUp(MotionEvent motionEvent) {
        
        
        if (!mWaitForDoubleTap) {
            sendPointToGecko("Gesture:SingleTap", motionEvent);
        }
        
        return false;
    }

    @Override
    public boolean onSingleTapConfirmed(MotionEvent motionEvent) {
        
        if (mWaitForDoubleTap) {
            sendPointToGecko("Gesture:SingleTap", motionEvent);
        }
        return true;
    }

    @Override
    public boolean onDoubleTap(MotionEvent motionEvent) {
        sendPointToGecko("Gesture:DoubleTap", motionEvent);
        return true;
    }

    private void cancelTouch() {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Gesture:CancelTouch", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    





    private ImmutableViewportMetrics getMetricsToZoomTo(RectF zoomToRect) {
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
        return finalMetrics;
    }

    private boolean animatedZoomTo(RectF zoomToRect) {
        bounce(getMetricsToZoomTo(zoomToRect), PanZoomState.ANIMATED_ZOOM);
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

    @Override
    public void setOverscrollHandler(final Overscroll handler) {
        mOverscroll = handler;
    }
}
