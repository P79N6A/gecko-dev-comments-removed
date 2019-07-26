




package org.mozilla.gecko;

import org.mozilla.gecko.updater.UpdateServiceHelper;

import android.content.*;

public class GeckoUpdateReceiver
    extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent) {
        if (UpdateServiceHelper.ACTION_CHECK_UPDATE_RESULT.equals(intent.getAction())) {
            GeckoAppShell.notifyCheckUpdateResult(intent.getBooleanExtra("result", false));
        }
    }
}
