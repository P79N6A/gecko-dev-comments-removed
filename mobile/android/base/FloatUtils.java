




































package org.mozilla.gecko;

import java.lang.Math;

public final class FloatUtils {
    public static boolean fuzzyEquals(float a, float b) {
        return (Math.abs(a - b) < 1e-6);
    }
}
