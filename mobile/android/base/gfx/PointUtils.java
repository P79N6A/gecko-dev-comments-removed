




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.FloatUtils;
import android.graphics.Point;
import android.graphics.PointF;
import java.lang.Math;

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

   


   public static PointF interpolate(PointF startPoint, PointF endPoint, float weight) {
       float x = FloatUtils.interpolate(startPoint.x, endPoint.x, weight);
       float y = FloatUtils.interpolate(startPoint.y, endPoint.y, weight);
       return new PointF(x, y);
   }
}

