




































package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import android.graphics.Rect;

public class FloatRect {
    public final float x, y, width, height;

    public FloatRect(float inX, float inY, float inWidth, float inHeight) {
        x = inX; y = inY; width = inWidth; height = inHeight;
    }

    public FloatRect(Rect intRect) {
        x = intRect.left; y = intRect.top; width = intRect.width(); height = intRect.height();
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof FloatRect))
            return false;
        FloatRect otherRect = (FloatRect)other;
        return x == otherRect.x && y == otherRect.y &&
            width == otherRect.width && height == otherRect.height;
    }

    public float getRight() { return x + width; }
    public float getBottom() { return y + height; }

    public PointF getOrigin() { return new PointF(x, y); }
    public PointF getCenter() { return new PointF(x + width / 2, y + height / 2); }

    
    public FloatRect intersect(FloatRect other) {
        float left = Math.max(x, other.x);
        float top = Math.max(y, other.y);
        float right = Math.min(getRight(), other.getRight());
        float bottom = Math.min(getBottom(), other.getBottom());
        return new FloatRect(left, top, Math.max(right - left, 0), Math.max(bottom - top, 0));
    }

    
    public boolean contains(FloatRect other) {
        return x <= other.x && y <= other.y &&
               getRight() >= other.getRight() &&
               getBottom() >= other.getBottom();
    }

    
    public FloatRect contract(float lessWidth, float lessHeight) {
        float halfWidth = width / 2.0f - lessWidth, halfHeight = height / 2.0f - lessHeight;
        PointF center = getCenter();
        return new FloatRect(center.x - halfWidth, center.y - halfHeight,
                             halfWidth * 2.0f, halfHeight * 2.0f);
    }

    
    public FloatRect scaleAll(float factor) {
        return new FloatRect(x * factor, y * factor, width * factor, height * factor);
    }
}

