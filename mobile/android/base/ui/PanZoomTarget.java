




package org.mozilla.gecko.ui;

import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;

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
