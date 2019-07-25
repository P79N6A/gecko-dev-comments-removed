




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.IntPoint;
import org.json.JSONException;
import org.json.JSONObject;

public class IntRect implements Cloneable {
    public final int x, y, width, height;

    public IntRect(int inX, int inY, int inWidth, int inHeight) {
        x = inX; y = inY; width = inWidth; height = inHeight;
    }

    public IntRect(JSONObject json) {
        try {
            x = json.getInt("x");
            y = json.getInt("y");
            width = json.getInt("width");
            height = json.getInt("height");
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public Object clone() { return new IntRect(x, y, width, height); }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof IntRect))
            return false;
        IntRect otherRect = (IntRect)other;
        return x == otherRect.x && y == otherRect.y && width == otherRect.width &&
            height == otherRect.height;
    }

    @Override
    public String toString() { return "(" + x + "," + y + "," + width + "," + height + ")"; }

    public IntPoint getOrigin() { return new IntPoint(x, y); }
    public IntPoint getCenter() { return new IntPoint(x + width / 2, y + height / 2); }

    public int getRight() { return x + width; }
    public int getBottom() { return y + height; }

    
    public IntRect contract(int lessWidth, int lessHeight) {
        float halfWidth = width / 2.0f - lessWidth, halfHeight = height / 2.0f - lessHeight;
        IntPoint center = getCenter();
        return new IntRect((int)Math.round((float)center.x - halfWidth),
                           (int)Math.round((float)center.y - halfHeight),
                           (int)Math.round(halfWidth * 2.0f),
                           (int)Math.round(halfHeight * 2.0f));
    }
}


