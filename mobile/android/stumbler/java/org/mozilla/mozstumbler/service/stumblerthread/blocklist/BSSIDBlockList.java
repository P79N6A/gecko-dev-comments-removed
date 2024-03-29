



package org.mozilla.mozstumbler.service.stumblerthread.blocklist;

import android.net.wifi.ScanResult;
import android.util.Log;
import org.mozilla.mozstumbler.service.AppGlobals;
import java.util.Locale;
import java.util.regex.Pattern;

public final class BSSIDBlockList {
    private static final String LOG_TAG = AppGlobals.makeLogTag(BSSIDBlockList.class.getSimpleName());
    private static final String NULL_BSSID = "000000000000";
    private static final String WILDCARD_BSSID = "ffffffffffff";
    private static final Pattern BSSID_PATTERN = Pattern.compile("([0-9a-f]{12})");
    private static String[] sOuiList = new String[]{};

    private BSSIDBlockList() {
    }

    public static void setFilterList(String[] list) {
        sOuiList = list;
    }

    public static boolean contains(ScanResult scanResult) {
        String BSSID = scanResult.BSSID;
        if (BSSID == null || NULL_BSSID.equals(BSSID) || WILDCARD_BSSID.equals(BSSID)) {
            return true; 
        }

        if (!isCanonicalBSSID(BSSID)) {
            Log.w(LOG_TAG, "", new IllegalArgumentException("Unexpected BSSID format: " + BSSID));
            return true; 
        }

        for (String oui : sOuiList) {
            if (BSSID.startsWith(oui)) {
                return true; 
            }
        }

        return false; 
    }

    public static String canonicalizeBSSID(String BSSID) {
        if (BSSID == null) {
            return "";
        }

        if (isCanonicalBSSID(BSSID)) {
            return BSSID;
        }

        
        BSSID = BSSID.toLowerCase(Locale.US).replaceAll("[\\-\\.:]", "");

        return isCanonicalBSSID(BSSID) ? BSSID : "";
    }

    private static boolean isCanonicalBSSID(String BSSID) {
        return BSSID_PATTERN.matcher(BSSID).matches();
    }
}
