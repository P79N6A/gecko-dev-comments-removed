



package org.mozilla.mozstumbler.service.utils;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;
import org.mozilla.mozstumbler.service.AppGlobals;

public final class NetworkUtils {
    private static final String LOG_TAG = AppGlobals.LOG_PREFIX + NetworkUtils.class.getSimpleName();

    ConnectivityManager mConnectivityManager;
    static NetworkUtils sInstance;

    
    static public void createGlobalInstance(Context context) {
        sInstance = new NetworkUtils();
        sInstance.mConnectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    
    public static NetworkUtils getInstance() {
        assert(sInstance != null);
        return sInstance;
    }

    public synchronized boolean isWifiAvailable() {
        if (mConnectivityManager == null) {
            Log.e(LOG_TAG, "ConnectivityManager is null!");
            return false;
        }

        NetworkInfo aNet = mConnectivityManager.getActiveNetworkInfo();
        return (aNet != null && aNet.getType() == ConnectivityManager.TYPE_WIFI);
    }

}
