




































package org.mozilla.gecko.gfx;

import org.json.JSONException;
import org.json.JSONObject;

public class IntSize {
    public final int width, height;

    public IntSize(IntSize size) { width = size.width; height = size.height; }
    public IntSize(int inWidth, int inHeight) { width = inWidth; height = inHeight; }

    public IntSize(JSONObject json) {
        try {
            width = json.getInt("width");
            height = json.getInt("height");
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    public boolean equals(IntSize size) {
        return ((size.width == width) && (size.height == height));
    }

    @Override
    public String toString() { return "(" + width + "," + height + ")"; }

    public IntSize scale(float factor) {
        return new IntSize((int)Math.round(width * factor),
                           (int)Math.round(height * factor));
    }
}

