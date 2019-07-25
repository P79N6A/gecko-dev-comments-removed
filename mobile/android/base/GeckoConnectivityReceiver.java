




































package org.mozilla.gecko;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class GeckoConnectivityReceiver extends BroadcastReceiver {
    



    private static final String LINK_DATA_UP = "up";
    private static final String LINK_DATA_DOWN = "down";
    private static final String LINK_DATA_UNKNOWN = "unknown";

    private IntentFilter mFilter;

    private static boolean isRegistered = false;

    public GeckoConnectivityReceiver() {
        mFilter = new IntentFilter();
        mFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String status;
        ConnectivityManager cm = (ConnectivityManager)
            context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();
        if (info == null)
            status = LINK_DATA_UNKNOWN;
        else if (!info.isConnected())
            status = LINK_DATA_DOWN;
        else
            status = LINK_DATA_UP;

        if (GeckoApp.checkLaunchState(GeckoApp.LaunchState.GeckoRunning))
            GeckoAppShell.onChangeNetworkLinkStatus(status);
    }

    public void registerFor(Activity activity) {
        if (!isRegistered) {
            activity.registerReceiver(this, mFilter);
            isRegistered = true;
        }
    }

    public void unregisterFor(Activity activity) {
        if (isRegistered) {
            activity.unregisterReceiver(this);
            isRegistered = false;
        }
    }
}
