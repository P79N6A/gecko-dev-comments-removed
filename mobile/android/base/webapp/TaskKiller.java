




package org.mozilla.gecko.webapp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;








public class TaskKiller extends BroadcastReceiver {

    private static final String LOGTAG = "GeckoWebappTaskKiller";

    @Override
    public void onReceive(Context context, Intent intent) {
        String packageName = intent.getStringExtra("packageName");
        int slot = Allocator.getInstance(context).getIndexForApp(packageName);
        if (slot >= 0) {
            EventListener.killWebappSlot(context, slot);
        } else {
            Log.w(LOGTAG, "Asked to kill " + packageName + " but this runtime (" + context.getPackageName() + ") doesn't know about it.");
        }
    }

}
