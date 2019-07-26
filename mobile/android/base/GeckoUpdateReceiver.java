




package org.mozilla.gecko;

import org.mozilla.gecko.updater.UpdateServiceHelper;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class GeckoUpdateReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent) {
        if (UpdateServiceHelper.ACTION_CHECK_UPDATE_RESULT.equals(intent.getAction())) {
            String result = intent.getStringExtra("result");
            if (GeckoApp.mAppContext != null && result != null) {
                GeckoApp.mAppContext.notifyCheckUpdateResult(result);
            }
        }
    }
}
