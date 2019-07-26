




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.BitmapUtils;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.net.Uri;
import android.util.Log;

import java.util.concurrent.ConcurrentHashMap;

public class NotificationHandler {
    private final ConcurrentHashMap<Integer, Notification>
            mNotifications = new ConcurrentHashMap<Integer, Notification>();
    private final Context mContext;
    private final NotificationManager mNotificationManager;

    









    private Notification mForegroundNotification;
    private int mForegroundNotificationId;

    public NotificationHandler(Context context) {
        mContext = context;
        mNotificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    









    public void add(int notificationID, String aImageUrl, String aAlertTitle,
                    String aAlertText, PendingIntent contentIntent) {
        
        remove(notificationID);

        Uri imageUri = Uri.parse(aImageUrl);
        int icon = BitmapUtils.getResource(imageUri, R.drawable.ic_status_logo);
        final AlertNotification notification = new AlertNotification(mContext, notificationID,
                icon, aAlertTitle, aAlertText, System.currentTimeMillis(), imageUri);

        notification.setLatestEventInfo(mContext, aAlertTitle, aAlertText, contentIntent);

        mNotificationManager.notify(notificationID, notification);
        mNotifications.put(notificationID, notification);
    }

    





    public void add(int id, Notification notification) {
        mNotificationManager.notify(id, notification);
        mNotifications.put(id, notification);

        if (mForegroundNotification == null && isOngoing(notification)) {
            setForegroundNotification(id, notification);
        }
    }

    







    public void update(int notificationID, long aProgress, long aProgressMax, String aAlertText) {
        final Notification notification = mNotifications.get(notificationID);
        if (notification == null) {
            return;
        }

        if (notification instanceof AlertNotification) {
            AlertNotification alert = (AlertNotification)notification;
            alert.updateProgress(aAlertText, aProgress, aProgressMax);
        }

        if (mForegroundNotification == null && isOngoing(notification)) {
            setForegroundNotification(notificationID, notification);
        }
    }

    




    public void remove(int notificationID) {
        final Notification notification = mNotifications.remove(notificationID);
        if (notification != null) {
            updateForegroundNotification(notificationID, notification);
        }
        mNotificationManager.cancel(notificationID);
    }

    







    public boolean isDone() {
        return mNotifications.isEmpty();
    }

    





    public boolean isOngoing(int notificationID) {
        final Notification notification = mNotifications.get(notificationID);
        return isOngoing(notification);
    }

    





    public boolean isOngoing(Notification notification) {
        if (notification != null && (isProgressStyle(notification) || ((notification.flags & Notification.FLAG_ONGOING_EVENT) > 0))) {
            return true;
        }
        return false;
    }

    






    private boolean isProgressStyle(Notification notification) {
        if (notification != null && notification instanceof AlertNotification) {
            return ((AlertNotification)notification).isProgressStyle();
        }
        return false;
    }

    protected void setForegroundNotification(int id, Notification notification) {
        mForegroundNotificationId = id;
        mForegroundNotification = notification;
    }

    private void updateForegroundNotification(int oldId, Notification oldNotification) {
        if (mForegroundNotificationId == oldId) {
            
            
            
            Notification foregroundNotification = null;
            int foregroundId = 0;
            for (final Integer id : mNotifications.keySet()) {
                final Notification notification = mNotifications.get(id);
                if (isOngoing(notification)) {
                    foregroundNotification = notification;
                    foregroundId = id;
                    break;
                }
            }
            setForegroundNotification(foregroundId, foregroundNotification);
        }
    }
}
