




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.ZoomConstraints;

import android.graphics.PointF;

public interface PanZoomTarget {
    public ImmutableViewportMetrics getViewportMetrics();
    public ZoomConstraints getZoomConstraints();
    public boolean isFullScreen();

    public void setAnimationTarget(ImmutableViewportMetrics viewport);
    public void setViewportMetrics(ImmutableViewportMetrics viewport);
    
    public void forceRedraw();

    public boolean post(Runnable action);
    public Object getLock();
    public PointF convertViewPointToLayerPoint(PointF viewPoint);
}
