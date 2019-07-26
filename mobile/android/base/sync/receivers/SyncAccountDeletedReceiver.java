



package org.mozilla.gecko.sync.receivers;

import org.mozilla.gecko.background.common.log.Logger;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class SyncAccountDeletedReceiver extends BroadcastReceiver {
  public static final String LOG_TAG = "SyncAccountDeletedReceiver";

  








  @Override
  public void onReceive(final Context context, Intent broadcastIntent) {
    Logger.debug(LOG_TAG, "Sync Account Deleted broadcast received.");

    Intent serviceIntent = new Intent(context, SyncAccountDeletedService.class);
    serviceIntent.putExtras(broadcastIntent);
    context.startService(serviceIntent);
  }
}
