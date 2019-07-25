




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.IntPoint;

public class FloatPoint {
    public final float x, y;

    public FloatPoint(float inX, float inY) {
        x = inX; y = inY;
    }

    public FloatPoint(IntPoint intPoint) {
        x = intPoint.x; y = intPoint.y;
    }

    @Override
    public String toString() {
        return "(" + x + ", " + y + ")";
    }

    public FloatPoint add(FloatPoint other) {
        return new FloatPoint(x + other.x, y + other.y);
    }

    public FloatPoint subtract(FloatPoint other) {
        return new FloatPoint(x - other.x, y - other.y);
    }

    public FloatPoint scale(float factor) {
        return new FloatPoint(x * factor, y * factor);
    }
}


