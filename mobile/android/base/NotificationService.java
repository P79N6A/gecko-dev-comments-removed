




package org.mozilla.gecko;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

public class NotificationService extends Service {
    private final IBinder mBinder = new NotificationBinder();
    private NotificationHandler mHandler;

    @Override
    public void onCreate() {
        
        
        mHandler = new NotificationHandler(this) {
            @Override
            protected void setForegroundNotification(int id, Notification notification) {
                super.setForegroundNotification(id, notification);

                if (notification == null) {
                    stopForeground(true);
                } else {
                    startForeground(id, notification);
                }
            }
        };
    }

    public class NotificationBinder extends Binder {
        NotificationService getService() {
            
            return NotificationService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public NotificationHandler getNotificationHandler() {
        return mHandler;
    }
}
