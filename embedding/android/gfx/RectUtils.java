




































package org.mozilla.gecko.gfx;

import android.graphics.Rect;
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
}
