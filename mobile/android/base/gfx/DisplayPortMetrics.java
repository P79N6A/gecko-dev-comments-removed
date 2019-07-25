




package org.mozilla.gecko.gfx;

import android.graphics.RectF;









public final class DisplayPortMetrics {
    private final RectF mPosition;
    private final float mResolution;

    public DisplayPortMetrics() {
        this(0, 0, 0, 0, 1);
    }

    public DisplayPortMetrics(float left, float top, float right, float bottom, float resolution) {
        mPosition = new RectF(left, top, right, bottom);
        mResolution = resolution;
    }

    public boolean contains(RectF rect) {
        return mPosition.contains(rect);
    }

    public String toJSON() {
        StringBuffer sb = new StringBuffer(256);
        sb.append("{ \"left\": ").append(mPosition.left)
          .append(", \"top\": ").append(mPosition.top)
          .append(", \"right\": ").append(mPosition.right)
          .append(", \"bottom\": ").append(mPosition.bottom)
          .append(", \"resolution\": ").append(mResolution)
          .append('}');
        return sb.toString();
    }

    @Override
    public String toString() {
        return "DisplayPortMetrics v=(" + mPosition.left + ","
                + mPosition.top + "," + mPosition.right + ","
                + mPosition.bottom + ") z=" + mResolution;
    }
}
