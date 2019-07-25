




































package org.mozilla.gecko;

import java.lang.Math;

public final class FloatUtils {
    public static boolean fuzzyEquals(float a, float b) {
        return (Math.abs(a - b) < 1e-6);
    }

    




    public static float interpolate(float from, float to, float t) {
        return from + (to - from) * t;
    }
}
