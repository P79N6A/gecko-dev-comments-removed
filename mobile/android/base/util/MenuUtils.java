




package org.mozilla.gecko.util;

import android.view.Menu;
import android.view.MenuItem;

public class MenuUtils {
    



    public static void safeSetVisible(Menu menu, int id, boolean visible) {
        MenuItem item = menu.findItem(id);
        if (item != null) {
            item.setVisible(visible);
        }
    }

    



    public static void safeSetEnabled(Menu menu, int id, boolean enabled) {
        MenuItem item = menu.findItem(id);
        if (item != null) {
            item.setEnabled(enabled);
        }
    }
}
