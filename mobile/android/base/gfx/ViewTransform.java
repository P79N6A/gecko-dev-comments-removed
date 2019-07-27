




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.generatorannotations.WrapEntireClassForJNI;

@WrapEntireClassForJNI
public class ViewTransform {
    public float x;
    public float y;
    public float scale;
    public float fixedLayerMarginLeft;
    public float fixedLayerMarginTop;
    public float fixedLayerMarginRight;
    public float fixedLayerMarginBottom;
    public float offsetX;
    public float offsetY;

    public ViewTransform(float inX, float inY, float inScale) {
        x = inX;
        y = inY;
        scale = inScale;
    }
}

