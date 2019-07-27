




package org.mozilla.gecko.tabs;

import android.graphics.Path;




public class TabCurve {

    public enum Direction {
        LEFT(-1),
        RIGHT(1);

        private final int value;

        private Direction(int value) {
            this.value = value;
        }
    }

    
    private static final float ASPECT_RATIO = 0.729f;

    
    private static final float W_M1 = 0.343f;
    private static final float W_M2 = 0.514f;
    private static final float W_M3 = 0.723f;

    
    private static final float H_M1 = 0.25f;
    private static final float H_M2 = 0.5f;
    private static final float H_M3 = 0.72f;
    private static final float H_M4 = 0.961f;

    private TabCurve() {
    }

    public static int getWidthForHeight(int height) {
        return (int) (height * ASPECT_RATIO);
    }

    public static void drawFromTop(Path path, int from, int height, Direction dir) {
        final int width = getWidthForHeight(height);

        path.cubicTo(from + width * W_M1 * dir.value, 0.0f,
                     from + width * W_M2 * dir.value, height * H_M1,
                     from + width * W_M2 * dir.value, height * H_M2);
        path.cubicTo(from + width * W_M2 * dir.value, height * H_M3,
                     from + width * W_M3 * dir.value, height * H_M4,
                     from + width * dir.value, height);
    }

    public static void drawFromBottom(Path path, int from, int height, Direction dir) {
        final int width = getWidthForHeight(height);

        path.cubicTo(from + width * (1f - W_M3) * dir.value, height * H_M4,
                     from + width * (1f - W_M2) * dir.value, height * H_M3,
                     from + width * (1f - W_M2) * dir.value, height * H_M2);
        path.cubicTo(from + width * (1f - W_M2) * dir.value, height * H_M1,
                     from + width * (1f - W_M1) * dir.value, 0.0f,
                     from + width * dir.value, 0.0f);
    }
}
