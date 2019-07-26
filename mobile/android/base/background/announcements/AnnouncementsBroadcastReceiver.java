



package org.mozilla.gecko.background.announcements;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;










public class AnnouncementsBroadcastReceiver extends BroadcastReceiver {

  


  @Override
  public void onReceive(Context context, Intent intent) {
    Intent service = new Intent(context, AnnouncementsBroadcastService.class);
    service.putExtras(intent);
    service.setAction(intent.getAction());
    context.startService(service);
  }
}