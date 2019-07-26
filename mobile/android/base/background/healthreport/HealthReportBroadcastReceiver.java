



package org.mozilla.gecko.background.healthreport;

import org.mozilla.gecko.background.common.log.Logger;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;















public class HealthReportBroadcastReceiver extends BroadcastReceiver {
  public static final String LOG_TAG = HealthReportBroadcastReceiver.class.getSimpleName();

  


  @Override
  public void onReceive(Context context, Intent intent) {
    Logger.debug(LOG_TAG, "Received intent - forwarding to BroadcastService.");
    Intent service = new Intent(context, HealthReportBroadcastService.class);
    service.putExtras(intent);
    service.setAction(intent.getAction());
    context.startService(service);
  }
}
