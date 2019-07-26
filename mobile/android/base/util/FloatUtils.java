




package org.mozilla.gecko.util;

import android.graphics.PointF;

import java.lang.IllegalArgumentException;

public final class FloatUtils {
    private FloatUtils() {}

    public static boolean fuzzyEquals(float a, float b) {
        return (Math.abs(a - b) < 1e-6);
    }

    public static boolean fuzzyEquals(PointF a, PointF b) {
        return fuzzyEquals(a.x, b.x) && fuzzyEquals(a.y, b.y);
    }

    




    public static float interpolate(float from, float to, float t) {
        return from + (to - from) * t;
    }

    



    public static float clamp(float value, float low, float high) {
        if (high < low) {
          throw new IllegalArgumentException(
              "clamp called with invalid parameters (" + high + " < " + low + ")" );
        }
        return Math.max(low, Math.min(high, value));
    }
}
