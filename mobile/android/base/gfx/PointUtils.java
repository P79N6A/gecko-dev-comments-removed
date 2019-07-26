




package org.mozilla.gecko.gfx;

import org.json.JSONException;
import org.json.JSONObject;

import android.graphics.Point;
import android.graphics.PointF;

public final class PointUtils {
    public static PointF add(PointF one, PointF two) {
        return new PointF(one.x + two.x, one.y + two.y);
    }

    public static PointF subtract(PointF one, PointF two) {
        return new PointF(one.x - two.x, one.y - two.y);
    }

    public static PointF scale(PointF point, float factor) {
        return new PointF(point.x * factor, point.y * factor);
    }

    public static Point round(PointF point) {
        return new Point(Math.round(point.x), Math.round(point.y));
    }

   
   public static float distance(PointF point) {
        return (float)Math.sqrt(point.x * point.x + point.y * point.y);
   }

    
    public static float distance(PointF one, PointF two) {
        return PointF.length(one.x - two.x, one.y - two.y);
    }

    public static JSONObject toJSON(PointF point) throws JSONException {
        
        int x = Math.round(point.x);
        int y = Math.round(point.y);
        JSONObject json = new JSONObject();
        json.put("x", x);
        json.put("y", y);
        return json;
    }
}

