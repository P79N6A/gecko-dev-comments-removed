



package org.mozilla.gecko.background.healthreport.upload;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;















public class HealthReportBroadcastReceiver extends BroadcastReceiver {
  public static final String LOG_TAG = HealthReportBroadcastReceiver.class.getSimpleName();

  


  @Override
  public void onReceive(Context context, Intent intent) {
    if (HealthReportConstants.UPLOAD_FEATURE_DISABLED) {
      Logger.debug(LOG_TAG, "Health report upload feature is compile-time disabled; not forwarding intent.");
      return;
    }

    Logger.debug(LOG_TAG, "Health report upload feature is compile-time enabled; forwarding intent.");
    Intent service = new Intent(context, HealthReportBroadcastService.class);
    service.putExtras(intent);
    service.setAction(intent.getAction());
    context.startService(service);
  }
}
