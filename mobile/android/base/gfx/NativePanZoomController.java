




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.graphics.PointF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

class NativePanZoomController implements PanZoomController, GeckoEventListener {
    private final PanZoomTarget mTarget;
    private final EventDispatcher mDispatcher;

    NativePanZoomController(PanZoomTarget target, View view, EventDispatcher dispatcher) {
        mTarget = target;
        mDispatcher = dispatcher;
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            init();
        } else {
            mDispatcher.registerGeckoThreadListener(this, "Gecko:Ready");
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        if ("Gecko:Ready".equals(event)) {
            mDispatcher.unregisterGeckoThreadListener(this, "Gecko:Ready");
            init();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        GeckoEvent wrapped = GeckoEvent.createMotionEvent(event, true);
        return handleTouchEvent(wrapped);
    }

    @Override
    public boolean onMotionEvent(MotionEvent event) {
        
        return false;
    }

    @Override
    public boolean onKeyEvent(KeyEvent event) {
        
        return false;
    }

    @Override
    public PointF getVelocityVector() {
        
        return new PointF(0, 0);
    }

    @Override
    public void pageRectUpdated() {
        
    }

    @Override
    public void abortPanning() {
        
    }

    @Override
    public void notifyDefaultActionPrevented(boolean prevented) {
        
        
        throw new IllegalStateException("APZCCallbackHandler::NotifyDefaultPrevented should be getting called, not this!");
    }

    @Override
    public native void abortAnimation();

    private native void init();
    private native boolean handleTouchEvent(GeckoEvent event);
    private native void handleMotionEvent(GeckoEvent event);

    @Override
    public native void destroy();
    @Override
    public native boolean getRedrawHint();
    @Override
    public native void setOverScrollMode(int overscrollMode);
    @Override
    public native int getOverScrollMode();

    @WrapElementForJNI(allowMultithread = true, stubName = "RequestContentRepaintWrapper")
    private void requestContentRepaint(float x, float y, float width, float height, float resolution) {
        mTarget.forceRedraw(new DisplayPortMetrics(x, y, x + width, y + height, resolution));
    }

    @Override
    public void setOverscrollHandler(final Overscroll listener) {
    }
}
