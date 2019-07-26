




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.BitmapUtils;

import android.app.PendingIntent;
import android.content.Context;
import android.net.Uri;

import java.util.concurrent.ConcurrentHashMap;

public class NotificationHandler {
    private final ConcurrentHashMap<Integer, AlertNotification>
            mAlertNotifications = new ConcurrentHashMap<Integer, AlertNotification>();
    private final Context mContext;

    









    private AlertNotification mForegroundNotification;

    public NotificationHandler(Context context) {
        mContext = context;
    }

    









    public void add(int notificationID, String aImageUrl, String aAlertTitle,
                    String aAlertText, PendingIntent contentIntent) {
        
        remove(notificationID);

        Uri imageUri = Uri.parse(aImageUrl);
        int icon = BitmapUtils.getResource(imageUri, R.drawable.ic_status_logo);
        final AlertNotification notification = new AlertNotification(mContext, notificationID,
                icon, aAlertTitle, aAlertText, System.currentTimeMillis(), imageUri);

        notification.setLatestEventInfo(mContext, aAlertTitle, aAlertText, contentIntent);

        notification.show();
        mAlertNotifications.put(notification.getId(), notification);
    }

    







    public void update(int notificationID, long aProgress, long aProgressMax, String aAlertText) {
        final AlertNotification notification = mAlertNotifications.get(notificationID);
        if (notification == null) {
            return;
        }

        notification.updateProgress(aAlertText, aProgress, aProgressMax);

        if (mForegroundNotification == null && notification.isProgressStyle()) {
            setForegroundNotification(notification);
        }

        
        if (aProgress == aProgressMax) {
            remove(notificationID);
        }
    }

    




    public void remove(int notificationID) {
        final AlertNotification notification = mAlertNotifications.remove(notificationID);
        if (notification != null) {
            updateForegroundNotification(notification);
            notification.cancel();
        }
    }

    







    public boolean isDone() {
        return mAlertNotifications.isEmpty();
    }

    





    public boolean isProgressStyle(int notificationID) {
        final AlertNotification notification = mAlertNotifications.get(notificationID);
        return notification != null && notification.isProgressStyle();
    }

    protected void setForegroundNotification(AlertNotification notification) {
        mForegroundNotification = notification;
    }

    private void updateForegroundNotification(AlertNotification oldNotification) {
        if (mForegroundNotification == oldNotification) {
            
            
            
            AlertNotification foregroundNotification = null;
            for (final AlertNotification notification : mAlertNotifications.values()) {
                if (notification.isProgressStyle()) {
                    foregroundNotification = notification;
                    break;
                }
            }

            setForegroundNotification(foregroundNotification);
        }
    }
}
