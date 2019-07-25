




































package org.mozilla.gecko.gfx;

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
       float x = (startPoint.x-endPoint.x)*weight + endPoint.x;
       float y = (startPoint.y-endPoint.y)*weight + endPoint.y;
       return new PointF(x, y);
   }
}

