




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.graphics.PointF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

class NativePanZoomController implements PanZoomController, GeckoEventListener {
    private final PanZoomTarget mTarget;
    private final EventDispatcher mDispatcher;
    private final CallbackRunnable mCallbackRunnable;

    NativePanZoomController(PanZoomTarget target, View view, EventDispatcher dispatcher) {
        mTarget = target;
        mDispatcher = dispatcher;
        mCallbackRunnable = new CallbackRunnable();
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            init();
        } else {
            mDispatcher.registerEventListener("Gecko:Ready", this);
        }
    }

    public void handleMessage(String event, JSONObject message) {
        if ("Gecko:Ready".equals(event)) {
            mDispatcher.unregisterEventListener("Gecko:Ready", this);
            init();
        }
    }

    public boolean onTouchEvent(MotionEvent event) {
        GeckoEvent wrapped = GeckoEvent.createMotionEvent(event, true);
        handleTouchEvent(wrapped);
        return false;
    }

    public boolean onMotionEvent(MotionEvent event) {
        
        return false;
    }

    public boolean onKeyEvent(KeyEvent event) {
        
        return false;
    }

    public PointF getVelocityVector() {
        
        return new PointF(0, 0);
    }

    public void pageRectUpdated() {
        
    }

    public void abortPanning() {
        
    }

    public native void abortAnimation();

    private native void init();
    private native void handleTouchEvent(GeckoEvent event);
    private native void handleMotionEvent(GeckoEvent event);
    private native long runDelayedCallback();

    public native void destroy();
    public native void notifyDefaultActionPrevented(boolean prevented);
    public native boolean getRedrawHint();
    public native void setOverScrollMode(int overscrollMode);
    public native int getOverScrollMode();

    @WrapElementForJNI(allowMultithread = true, stubName = "RequestContentRepaintWrapper")
    private void requestContentRepaint(float x, float y, float width, float height, float resolution) {
        mTarget.forceRedraw(new DisplayPortMetrics(x, y, x + width, y + height, resolution));
    }

    @WrapElementForJNI(allowMultithread = true, stubName = "PostDelayedCallbackWrapper")
    private void postDelayedCallback(long delay) {
        mTarget.postDelayed(mCallbackRunnable, delay);
    }

    class CallbackRunnable implements Runnable {
        @Override
        public void run() {
            long nextDelay = runDelayedCallback();
            if (nextDelay >= 0) {
                mTarget.postDelayed(this, nextDelay);
            }
        }
    }

    public native void updateScrollOffset(float cssX, float cssY);
}
