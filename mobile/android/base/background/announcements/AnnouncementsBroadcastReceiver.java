



package org.mozilla.gecko.background.announcements;

import org.mozilla.gecko.background.BackgroundService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;










public class AnnouncementsBroadcastReceiver extends BroadcastReceiver {

  


  @Override
  public void onReceive(Context context, Intent intent) {
    if (AnnouncementsConstants.DISABLED) {
      return;
    }

    BackgroundService.runIntentInService(context, intent, AnnouncementsBroadcastService.class);
  }
}
