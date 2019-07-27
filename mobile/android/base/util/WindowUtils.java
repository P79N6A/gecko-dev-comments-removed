




package org.mozilla.gecko.util;

import org.mozilla.gecko.AppConstants.Versions;

import android.content.Context;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

import java.lang.reflect.Method;

public class WindowUtils {
    private static final String LOGTAG = "Gecko" + WindowUtils.class.getSimpleName();

    private WindowUtils() {  }

    








    public static int getLargestDimension(final Context context) {
        final Display display = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();

        if (Versions.feature17Plus) {
            final DisplayMetrics realMetrics = new DisplayMetrics();
            display.getRealMetrics(realMetrics);
            return Math.max(realMetrics.widthPixels, realMetrics.heightPixels);

        } else if (Versions.feature14Plus) {
            int tempWidth;
            int tempHeight;
            try {
                final Method getRawH = Display.class.getMethod("getRawHeight");
                final Method getRawW = Display.class.getMethod("getRawWidth");
                tempWidth = (Integer) getRawW.invoke(display);
                tempHeight = (Integer) getRawH.invoke(display);
            } catch (Exception e) {
                
                tempWidth = display.getWidth();
                tempHeight = display.getHeight();
                Log.w(LOGTAG, "Couldn't use reflection to get the real display metrics.");
            }

            return Math.max(tempWidth, tempHeight);

        } else {
            
            return Math.max(display.getWidth(), display.getHeight());
        }
    }
}
