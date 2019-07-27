




package org.mozilla.gecko.util;

import android.app.Activity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import org.mozilla.gecko.AppConstants.Versions;

public class ActivityUtils {
    private ActivityUtils() {
    }

    public static void setFullScreen(Activity activity, boolean fullscreen) {
        
        Window window = activity.getWindow();

        if (Versions.feature11Plus) {
            final int newVis;
            if (fullscreen) {
                newVis = View.SYSTEM_UI_FLAG_FULLSCREEN |
                         View.SYSTEM_UI_FLAG_LOW_PROFILE;
            } else {
                newVis = View.SYSTEM_UI_FLAG_VISIBLE;
            }

            window.getDecorView().setSystemUiVisibility(newVis);
        } else {
            window.setFlags(fullscreen ?
                            WindowManager.LayoutParams.FLAG_FULLSCREEN : 0,
                            WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
    }
}
