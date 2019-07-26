




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.ZoomConstraints;

import android.graphics.PointF;

public interface PanZoomTarget {
    public ImmutableViewportMetrics getViewportMetrics();
    public ZoomConstraints getZoomConstraints();
    public boolean isFullScreen();

    public void setAnimationTarget(ImmutableViewportMetrics viewport);
    public void setViewportMetrics(ImmutableViewportMetrics viewport);
    public void scrollBy(float dx, float dy);
    public void panZoomStopped();
    
    public void forceRedraw(DisplayPortMetrics displayPort);

    public boolean post(Runnable action);
    public boolean postDelayed(Runnable action, long delayMillis);
    public Object getLock();
    public PointF convertViewPointToLayerPoint(PointF viewPoint);
}
