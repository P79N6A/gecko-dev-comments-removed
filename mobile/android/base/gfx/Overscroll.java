




package org.mozilla.gecko.gfx;

import android.content.Context;
import android.graphics.Canvas;
import android.support.v4.widget.EdgeEffectCompat;
import android.support.v4.view.ViewCompat;
import android.view.View;

public class Overscroll {
    
    private static final int TOP = 0;
    private static final int BOTTOM = 1;
    private static final int LEFT = 2;
    private static final int RIGHT = 3;

    
    private final EdgeEffectCompat[] mEdges = new EdgeEffectCompat[4];

    
    private final View mView;

    
    public enum Axis {
        X,
        Y,
    };

    public Overscroll(final View v) {
        mView = v;
        Context context = v.getContext();
        for (int i = 0; i < 4; i++) {
            mEdges[i] = new EdgeEffectCompat(context);
        }
    }

    public void setSize(final int width, final int height) {
        mEdges[LEFT].setSize(height, width);
        mEdges[RIGHT].setSize(height, width);
        mEdges[TOP].setSize(width, height);
        mEdges[BOTTOM].setSize(width, height);
    }

    private EdgeEffectCompat getEdgeForAxisAndSide(final Axis axis, final float side) {
        if (axis == Axis.Y) {
            if (side < 0) {
                return mEdges[TOP];
            } else {
                return mEdges[BOTTOM];
            }
        } else {
            if (side < 0) {
                return mEdges[LEFT];
            } else {
                return mEdges[RIGHT];
            }
        }
    }

    public void setVelocity(final float velocity, final Axis axis) {
        final EdgeEffectCompat edge = getEdgeForAxisAndSide(axis, velocity);

        
        if (!edge.isFinished()) {
            edge.onRelease();
        } else {
            
            edge.onAbsorb((int)velocity);
        }

        ViewCompat.postInvalidateOnAnimation(mView);
    }

    public void setDistance(final float distance, final Axis axis) {
        
        if (distance == 0.0f) {
            return;
        }

        final EdgeEffectCompat edge = getEdgeForAxisAndSide(axis, (int)distance);
        edge.onPull(distance / (axis == Axis.X ? mView.getWidth() : mView.getHeight()));
        ViewCompat.postInvalidateOnAnimation(mView);
    }

    public void draw(final Canvas canvas, final ImmutableViewportMetrics metrics) {
        if (metrics == null) {
            return;
        }

        
        boolean invalidate = false;
        if (!mEdges[TOP].isFinished()) {
            invalidate |= draw(mEdges[TOP], canvas, metrics.marginLeft, metrics.marginTop, 0);
        }

        if (!mEdges[BOTTOM].isFinished()) {
            invalidate |= draw(mEdges[BOTTOM], canvas, mView.getWidth(), mView.getHeight(), 180);
        }

        if (!mEdges[LEFT].isFinished()) {
            invalidate |= draw(mEdges[LEFT], canvas, metrics.marginLeft, mView.getHeight(), 270);
        }

        if (!mEdges[RIGHT].isFinished()) {
            invalidate |= draw(mEdges[RIGHT], canvas, mView.getWidth(), metrics.marginTop, 90);
        }

        
        if (invalidate) {
            ViewCompat.postInvalidateOnAnimation(mView);
        }
    }

    public boolean draw(final EdgeEffectCompat edge, final Canvas canvas, final float translateX, final float translateY, final float rotation) {
        final int state = canvas.save();
        canvas.translate(translateX, translateY);
        canvas.rotate(rotation);
        boolean invalidate = edge.draw(canvas);
        canvas.restoreToCount(state);

        return invalidate;
    }
}
