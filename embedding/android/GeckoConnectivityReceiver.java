




































package org.mozilla.gecko;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class GeckoConnectivityReceiver
    extends BroadcastReceiver
{
    



    private static final String LINK_DATA_UP = "up";
    private static final String LINK_DATA_DOWN = "down";
    private static final String LINK_DATA_UNKNOWN = "unknown";

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
}
