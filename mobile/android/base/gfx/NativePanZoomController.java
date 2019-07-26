




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.graphics.PointF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

class NativePanZoomController implements PanZoomController, GeckoEventListener {
    private final EventDispatcher mDispatcher;

    NativePanZoomController(PanZoomTarget target, View view, EventDispatcher dispatcher) {
        mDispatcher = dispatcher;
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

    public void abortAnimation() {
        
    }

    private native void init();
    private native void handleTouchEvent(GeckoEvent event);
    private native void handleMotionEvent(GeckoEvent event);

    public native void destroy();
    public native void notifyDefaultActionPrevented(boolean prevented);
    public native boolean getRedrawHint();
    public native void setOverScrollMode(int overscrollMode);
    public native int getOverScrollMode();
}
