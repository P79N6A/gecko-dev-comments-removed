




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.FloatUtils;
import android.graphics.Rect;
import android.graphics.RectF;
import org.json.JSONException;
import org.json.JSONObject;

public final class RectUtils {
    public static Rect create(JSONObject json) {
        try {
            int x = json.getInt("x");
            int y = json.getInt("y");
            int width = json.getInt("width");
            int height = json.getInt("height");
            return new Rect(x, y, x + width, y + height);
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    public static Rect contract(Rect rect, int lessWidth, int lessHeight) {
        float halfLessWidth = lessWidth / 2.0f;
        float halfLessHeight = lessHeight / 2.0f;
        return new Rect(Math.round(rect.left + halfLessWidth),
                        Math.round(rect.top + halfLessHeight),
                        Math.round(rect.right - halfLessWidth),
                        Math.round(rect.bottom - halfLessHeight));
    }

    public static RectF contract(RectF rect, float lessWidth, float lessHeight) {
        float halfLessWidth = lessWidth / 2;
        float halfLessHeight = lessHeight / 2;
        return new RectF(rect.left + halfLessWidth,
                         rect.top + halfLessHeight,
                         rect.right - halfLessWidth,
                         rect.bottom - halfLessHeight);
    }

    public static RectF expand(RectF rect, float moreWidth, float moreHeight) {
        float halfMoreWidth = moreWidth / 2;
        float halfMoreHeight = moreHeight / 2;
        return new RectF(rect.left - halfMoreWidth,
                         rect.top - halfMoreHeight,
                         rect.right + halfMoreWidth,
                         rect.bottom + halfMoreHeight);
    }

    public static RectF intersect(RectF one, RectF two) {
        float left = Math.max(one.left, two.left);
        float top = Math.max(one.top, two.top);
        float right = Math.min(one.right, two.right);
        float bottom = Math.min(one.bottom, two.bottom);
        return new RectF(left, top, Math.max(right, left), Math.max(bottom, top));
    }

    public static RectF scale(RectF rect, float scale) {
        float x = rect.left * scale;
        float y = rect.top * scale;
        return new RectF(x, y,
                         x + (rect.width() * scale),
                         y + (rect.height() * scale));
    }

    
    public static Rect round(RectF rect) {
        return new Rect(Math.round(rect.left), Math.round(rect.top),
                        Math.round(rect.right), Math.round(rect.bottom));
    }

    
    public static Rect roundOut(RectF rect) {
        return new Rect((int)Math.floor(rect.left), (int)Math.floor(rect.top),
                        (int)Math.ceil(rect.right), (int)Math.ceil(rect.bottom));
    }

    public static IntSize getSize(Rect rect) {
        return new IntSize(rect.width(), rect.height());
    }

    



    public static RectF interpolate(RectF from, RectF to, float t) {
        return new RectF(FloatUtils.interpolate(from.left, to.left, t),
                         FloatUtils.interpolate(from.top, to.top, t),
                         FloatUtils.interpolate(from.right, to.right, t),
                         FloatUtils.interpolate(from.bottom, to.bottom, t));
    }

    public static boolean fuzzyEquals(RectF a, RectF b) {
        return FloatUtils.fuzzyEquals(a.top, b.top)
            && FloatUtils.fuzzyEquals(a.left, b.left)
            && FloatUtils.fuzzyEquals(a.right, b.right)
            && FloatUtils.fuzzyEquals(a.bottom, b.bottom);
    }
}
