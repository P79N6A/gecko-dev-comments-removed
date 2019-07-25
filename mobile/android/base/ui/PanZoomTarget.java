




package org.mozilla.gecko.ui;

import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.ViewportMetrics;

import android.graphics.PointF;

public interface PanZoomTarget {
    public ImmutableViewportMetrics getViewportMetrics();
    public ZoomConstraints getZoomConstraints();

    public void setAnimationTarget(ViewportMetrics viewport);
    public void setViewportMetrics(ViewportMetrics viewport);
    public void setForceRedraw();

    public boolean post(Runnable action);
    public Object getLock();
    public PointF convertViewPointToLayerPoint(PointF viewPoint);
}
