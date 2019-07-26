



package org.mozilla.gecko.background.announcements;

import org.mozilla.gecko.background.common.log.Logger;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;




public class AnnouncementsStartReceiver extends BroadcastReceiver {

  private static final String LOG_TAG = "AnnounceStartRec";

  @Override
  public void onReceive(Context context, Intent intent) {
    if (AnnouncementsConstants.DISABLED) {
      return;
    }

    Logger.debug(LOG_TAG, "AnnouncementsStartReceiver.onReceive().");
    Intent service = new Intent(context, AnnouncementsService.class);
    context.startService(service);
  }
}
