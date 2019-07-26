




package org.mozilla.gecko;

import java.lang.reflect.Field;
import java.util.concurrent.ConcurrentHashMap;

import android.app.PendingIntent;
import android.content.Context;
import android.net.Uri;

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

        int icon = R.drawable.ic_status_logo;

        Uri imageUri = Uri.parse(aImageUrl);
        final String scheme = imageUri.getScheme();
        if ("drawable".equals(scheme)) {
            String resource = imageUri.getSchemeSpecificPart();
            resource = resource.substring(resource.lastIndexOf('/') + 1);
            try {
                final Class<R.drawable> drawableClass = R.drawable.class;
                final Field f = drawableClass.getField(resource);
                icon = f.getInt(null);
            } catch (final Exception e) {} 
            imageUri = null;
        }

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
