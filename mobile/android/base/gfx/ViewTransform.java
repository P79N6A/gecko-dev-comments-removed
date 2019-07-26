




package org.mozilla.gecko.gfx;

public class ViewTransform {
    public float x;
    public float y;
    public float scale;
    public float fixedLayerMarginLeft;
    public float fixedLayerMarginTop;
    public float fixedLayerMarginRight;
    public float fixedLayerMarginBottom;

    public ViewTransform(float inX, float inY, float inScale) {
        x = inX;
        y = inY;
        scale = inScale;
        fixedLayerMarginLeft = 0;
        fixedLayerMarginTop = 0;
        fixedLayerMarginRight = 0;
        fixedLayerMarginBottom = 0;
    }
}

