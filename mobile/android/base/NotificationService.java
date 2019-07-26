




package org.mozilla.gecko;

import java.lang.reflect.Field;
import java.util.concurrent.ConcurrentHashMap;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.IBinder;

public class NotificationService extends Service {
    private final IBinder mBinder = new NotificationBinder();

    private final ConcurrentHashMap<Integer, AlertNotification>
            mAlertNotifications = new ConcurrentHashMap<Integer, AlertNotification>();

    









    private AlertNotification mForegroundNotification;

    public class NotificationBinder extends Binder {
        NotificationService getService() {
            
            return NotificationService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    









    public void add(int notificationID, String aImageUrl, String aAlertTitle,
                    String aAlertText, PendingIntent contentIntent, PendingIntent clearIntent) {
        
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

        final AlertNotification notification = new AlertNotification(this, notificationID,
                icon, aAlertTitle, aAlertText, System.currentTimeMillis(), imageUri);

        notification.setLatestEventInfo(this, aAlertTitle, aAlertText, contentIntent);
        notification.deleteIntent = clearIntent;

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

    private void setForegroundNotification(AlertNotification notification) {
        mForegroundNotification = notification;
        if (notification == null) {
            stopForeground(true);
        } else {
            startForeground(notification.getId(), notification);
        }
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
