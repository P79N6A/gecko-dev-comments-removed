




package org.mozilla.gecko.gfx;

import android.graphics.Canvas;

public interface Overscroll {
    
    public enum Axis {
        X,
        Y,
    };

    public void draw(final Canvas canvas, final ImmutableViewportMetrics metrics);
    public void setSize(final int width, final int height);
    public void setVelocity(final float velocity, final Axis axis);
    public void setDistance(final float distance, final Axis axis);
}
