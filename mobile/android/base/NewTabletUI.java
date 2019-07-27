



package org.mozilla.gecko;

import android.content.Context;
import android.content.SharedPreferences;

import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.util.HardwareUtils;

public class NewTabletUI {
    
    
    private static final boolean DEFAULT = true;

    private static Boolean sNewTabletUI;

    public static synchronized boolean isEnabled(Context context) {
        if (!HardwareUtils.isTablet()) {
            return false;
        }

        if (sNewTabletUI == null) {
            final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);
            sNewTabletUI = prefs.getBoolean(GeckoPreferences.PREFS_NEW_TABLET_UI, DEFAULT);
        }

        return sNewTabletUI;
    }
}