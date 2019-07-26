




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.util.EventDispatcher;

import android.graphics.PointF;

public interface PanZoomController {
    
    
    public static final float PAN_THRESHOLD = 1/16f * GeckoAppShell.getDpi();

    static class Factory {
        static PanZoomController create(PanZoomTarget target, EventDispatcher dispatcher) {
            return new JavaPanZoomController(target, dispatcher);
        }
    }

    public void destroy();

    public boolean getRedrawHint();
    public PointF getVelocityVector();

    public void pageRectUpdated();
    public void abortPanning();
    public void abortAnimation();

    public void setOverScrollMode(int overscrollMode);
    public int getOverScrollMode();
}
