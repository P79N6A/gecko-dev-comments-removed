




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.util.FloatUtils;

import android.graphics.RectF;









public final class DisplayPortMetrics {
    public final float resolution;
    private final RectF mPosition;

    public DisplayPortMetrics() {
        this(0, 0, 0, 0, 1);
    }

    public DisplayPortMetrics(float left, float top, float right, float bottom, float resolution) {
        this.resolution = resolution;
        mPosition = new RectF(left, top, right, bottom);
    }

    public float getLeft() {
        return mPosition.left;
    }

    public float getTop() {
        return mPosition.top;
    }

    public float getRight() {
        return mPosition.right;
    }

    public float getBottom() {
        return mPosition.bottom;
    }

    public boolean contains(RectF rect) {
        return mPosition.contains(rect);
    }

    public boolean fuzzyEquals(DisplayPortMetrics metrics) {
        return RectUtils.fuzzyEquals(mPosition, metrics.mPosition)
            && FloatUtils.fuzzyEquals(resolution, metrics.resolution);
    }

    public String toJSON() {
        StringBuilder sb = new StringBuilder(256);
        sb.append("{ \"left\": ").append(mPosition.left)
          .append(", \"top\": ").append(mPosition.top)
          .append(", \"right\": ").append(mPosition.right)
          .append(", \"bottom\": ").append(mPosition.bottom)
          .append(", \"resolution\": ").append(resolution)
          .append('}');
        return sb.toString();
    }

    @Override
    public String toString() {
        return "DisplayPortMetrics v=(" + mPosition.left + "," + mPosition.top + "," + mPosition.right + ","
                + mPosition.bottom + ") z=" + resolution;
    }
}
