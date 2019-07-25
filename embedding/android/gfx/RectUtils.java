




































package org.mozilla.gecko.gfx;

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
        float halfLessWidth = (float)lessWidth / 2.0f;
        float halfLessHeight = (float)lessHeight / 2.0f;
        return new Rect((int)Math.round((float)rect.left + halfLessWidth),
                        (int)Math.round((float)rect.top + halfLessHeight),
                        (int)Math.round((float)rect.right - halfLessWidth),
                        (int)Math.round((float)rect.bottom - halfLessHeight));
    }

    public static RectF contract(RectF rect, float lessWidth, float lessHeight) {
        float halfLessWidth = lessWidth / 2;
        float halfLessHeight = lessHeight / 2;
        return new RectF(rect.left + halfLessWidth,
                         rect.top + halfLessHeight,
                         rect.right - halfLessWidth,
                         rect.bottom - halfLessHeight);
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

    public static RectF clamp(RectF rect, RectF dest) {
        float width = Math.min(rect.width(), dest.width());
        float height = Math.min(rect.height(), dest.height());
        float x = Math.max(dest.left, Math.min(dest.width()-rect.width(), rect.left));
        float y = Math.max(dest.top, Math.min(dest.height()-rect.height(), rect.top));
        return new RectF(x, y, width, height);
    }

    public static RectF blend(RectF aRect1, RectF aRect2, float aBlendAmount) {
        return new RectF((aRect1.left-aRect2.left)*aBlendAmount + aRect2.left,
                         (aRect1.top-aRect2.top)*aBlendAmount + aRect2.top,
                         (aRect1.width()-aRect2.width())*aBlendAmount + aRect2.width(),
                         (aRect1.height()-aRect2.height())*aBlendAmount + aRect2.height());
    }
}
