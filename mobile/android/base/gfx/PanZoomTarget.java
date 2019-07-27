




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.ZoomConstraints;

import android.graphics.PointF;
import android.graphics.RectF;

public interface PanZoomTarget {
    public ImmutableViewportMetrics getViewportMetrics();
    public ZoomConstraints getZoomConstraints();
    public FullScreenState getFullScreenState();
    public RectF getMaxMargins();

    public void setAnimationTarget(ImmutableViewportMetrics viewport);
    public void setViewportMetrics(ImmutableViewportMetrics viewport);
    public void scrollBy(float dx, float dy);
    public void scrollMarginsBy(float dx, float dy);
    public void panZoomStopped();
    
    public void forceRedraw(DisplayPortMetrics displayPort);

    public boolean post(Runnable action);
    public void postRenderTask(RenderTask task);
    public void removeRenderTask(RenderTask task);
    public Object getLock();
    public PointF convertViewPointToLayerPoint(PointF viewPoint);
}
