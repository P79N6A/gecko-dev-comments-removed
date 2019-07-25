




































package org.mozilla.gecko;

import android.content.*;

public class GeckoConnectivityReceiver
    extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent) {
        GeckoAppShell.onNetworkStateChange(true);
    }
}
