




package org.mozilla.gecko;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

public class NotificationService extends Service {
    private final IBinder mBinder = new NotificationBinder();
    private final NotificationHandler mHandler = new NotificationHandler(this) {
        @Override
        protected void setForegroundNotification(AlertNotification notification) {
            super.setForegroundNotification(notification);

            if (notification == null) {
                stopForeground(true);
            } else {
                startForeground(notification.getId(), notification);
            }
        }
    };

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
