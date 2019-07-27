



package org.mozilla.gecko;

import android.content.Context;

import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.HardwareUtils;

@RobocopTarget
public class NewTabletUI {
    public static boolean isEnabled(final Context context) {
        return HardwareUtils.isTablet();
    }
}
