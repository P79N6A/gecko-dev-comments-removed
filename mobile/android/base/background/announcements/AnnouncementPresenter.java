



package org.mozilla.gecko.background.announcements;

import java.net.URI;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.GlobalConstants;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;




public class AnnouncementPresenter {

  












  public static void displayAnnouncement(final Context context,
                                         final int notificationID,
                                         final String title,
                                         final String body,
                                         final URI uri) {
    final String ns = Context.NOTIFICATION_SERVICE;
    final NotificationManager notificationManager = (NotificationManager) context.getSystemService(ns);

    
    Uri u = Uri.parse(uri.toASCIIString());
    Intent intent = new Intent(Intent.ACTION_VIEW, u);

    
    intent.setClassName(GlobalConstants.BROWSER_INTENT_PACKAGE, GlobalConstants.BROWSER_INTENT_CLASS);
    PendingIntent contentIntent = PendingIntent.getActivity(context, 0, intent, 0);

    final int icon = R.drawable.ic_status_logo;

    
    final long when = System.currentTimeMillis();
    Notification notification = new Notification(icon, title, when);
    notification.flags = Notification.FLAG_AUTO_CANCEL;
    notification.setLatestEventInfo(context, title, body, contentIntent);

    
    







    
    notificationManager.notify(notificationID, notification);
  }

  public static void displayAnnouncement(final Context context,
                                         final Announcement snippet) {
    final int notificationID = snippet.getId();
    final String title = snippet.getTitle();
    final String body = snippet.getText();
    final URI uri = snippet.getUri();
    AnnouncementPresenter.displayAnnouncement(context, notificationID, title, body, uri);
  }
}
