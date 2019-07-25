




package org.mozilla.gecko.util;

import android.content.Context;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

public final class ConfigurationUtils {
    private ConfigurationUtils() {}

    public static DisplayMetrics getDisplayMetrics(Context context) {
        DisplayMetrics metrics = new DisplayMetrics();
        getDefaultDisplay(context).getMetrics(metrics);
        return metrics;
    }

    public static int getDisplayRotation(Context context) {
        return getDefaultDisplay(context).getRotation();
    }

    public static Display getDefaultDisplay(Context context) {
        return getWindowManager(context).getDefaultDisplay();
    }

    private static WindowManager getWindowManager(Context context) {
        return (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
    }
}
