




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.generatorannotations.WrapEntireClassForJNI;







@WrapEntireClassForJNI
public class ProgressiveUpdateData {
    public float x;
    public float y;
    public float width;
    public float height;
    public float scale;
    public boolean abort;

    public void setViewport(ImmutableViewportMetrics viewport) {
        this.x = viewport.viewportRectLeft;
        this.y = viewport.viewportRectTop;
        this.width = viewport.viewportRectRight - this.x;
        this.height = viewport.viewportRectBottom - this.y;
        this.scale = viewport.zoomFactor;
    }
}

