




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.generatorannotations.WrapEntireClassForJNI;







@WrapEntireClassForJNI
public class ProgressiveUpdateData {
    public float x;
    public float y;
    public float scale;
    public boolean abort;

    public void setViewport(ImmutableViewportMetrics viewport) {
        this.x = viewport.viewportRectLeft;
        this.y = viewport.viewportRectTop;
        this.scale = viewport.zoomFactor;
    }
}

