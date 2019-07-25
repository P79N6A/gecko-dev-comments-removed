




































package org.mozilla.gecko;

import android.telephony.*;

public class GeckoPhoneStateListener
    extends PhoneStateListener
{
    @Override
    public void onDataConnectionStateChanged(int state, int networkType) {
        GeckoAppShell.onNetworkStateChange(true);
    }
}

