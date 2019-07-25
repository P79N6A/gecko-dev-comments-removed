




































package org.mozilla.gecko;

import android.app.Activity;
import android.graphics.drawable.Drawable;
import android.view.View;

public class GeckoActionBar {

    public static View getCustomView(Activity activity) {
        return activity.getActionBar().getCustomView();
    }
 
    public static void hide(Activity activity) {
        activity.getActionBar().hide();
    }

    public static void setBackgroundDrawable(Activity activity, Drawable drawable) {
         activity.getActionBar().setBackgroundDrawable(drawable);
    }

    public static void setDisplayOptions(Activity activity, int options, int mask) {
         activity.getActionBar().setDisplayOptions(options, mask);
    }

    public static void setCustomView(Activity activity, View view) {
         activity.getActionBar().setCustomView(view);
    }

    public static void setDisplayHomeAsUpEnabled(Activity activity, boolean enabled) {
         activity.getActionBar().setDisplayHomeAsUpEnabled(enabled);
    } 
}
