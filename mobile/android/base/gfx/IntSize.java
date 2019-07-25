




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.FloatSize;
import org.json.JSONException;
import org.json.JSONObject;
import java.lang.Math;

public class IntSize {
    public final int width, height;

    public IntSize(IntSize size) { width = size.width; height = size.height; }
    public IntSize(int inWidth, int inHeight) { width = inWidth; height = inHeight; }

    public IntSize(FloatSize size) {
        width = Math.round(size.width);
        height = Math.round(size.height);
    }

    public IntSize(JSONObject json) {
        try {
            width = json.getInt("width");
            height = json.getInt("height");
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    public int getArea() {
        return width * height;
    }

    public boolean equals(IntSize size) {
        return ((size.width == width) && (size.height == height));
    }

    public boolean isPositive() {
        return (width > 0 && height > 0);
    }

    @Override
    public String toString() { return "(" + width + "," + height + ")"; }

    public IntSize scale(float factor) {
        return new IntSize(Math.round(width * factor),
                           Math.round(height * factor));
    }

    
    public static int nextPowerOfTwo(int value) {
        
        if (0 == value--) {
            return 1;
        }
        value = (value >> 1) | value;
        value = (value >> 2) | value;
        value = (value >> 4) | value;
        value = (value >> 8) | value;
        value = (value >> 16) | value;
        return value + 1;
    }

    public static int nextPowerOfTwo(float value) {
        return nextPowerOfTwo((int) value);
    }

    public IntSize nextPowerOfTwo() {
        return new IntSize(nextPowerOfTwo(width), nextPowerOfTwo(height));
    }
}

