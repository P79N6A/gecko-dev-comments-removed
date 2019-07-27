




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.EventDispatcher;

import android.graphics.PointF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

public interface PanZoomController {
    
    
    public static final float PAN_THRESHOLD = 1/16f * GeckoAppShell.getDpi();

    
    public static final float CLICK_THRESHOLD = 1/50f * GeckoAppShell.getDpi();

    static class Factory {
        static PanZoomController create(PanZoomTarget target, View view, EventDispatcher dispatcher) {
            if (org.mozilla.gecko.AppConstants.MOZ_ANDROID_APZ) {
                return new NativePanZoomController(target, view, dispatcher);
            } else {
                return new JavaPanZoomController(target, view, dispatcher);
            }
        }
    }

    public void destroy();

    public boolean onTouchEvent(MotionEvent event);
    public boolean onMotionEvent(MotionEvent event);
    public boolean onKeyEvent(KeyEvent event);
    public void notifyDefaultActionPrevented(boolean prevented);

    public boolean getRedrawHint();
    public PointF getVelocityVector();

    public void pageRectUpdated();
    public void abortPanning();
    public void abortAnimation();

    public void setOverScrollMode(int overscrollMode);
    public int getOverScrollMode();

    public void setOverscrollHandler(final Overscroll controller);
}
