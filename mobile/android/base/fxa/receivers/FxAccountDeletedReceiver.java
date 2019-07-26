



package org.mozilla.gecko.fxa.receivers;

import org.mozilla.gecko.background.common.log.Logger;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class FxAccountDeletedReceiver extends BroadcastReceiver {
  public static final String LOG_TAG = FxAccountDeletedReceiver.class.getSimpleName();

  








  @Override
  public void onReceive(final Context context, Intent broadcastIntent) {
    Logger.debug(LOG_TAG, "FxAccount deleted broadcast received.");

    Intent serviceIntent = new Intent(context, FxAccountDeletedService.class);
    serviceIntent.putExtras(broadcastIntent);
    context.startService(serviceIntent);
  }
}
