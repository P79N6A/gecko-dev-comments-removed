




package org.mozilla.gecko;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;




public class ServiceNotificationClient extends NotificationClient {
    private static final String LOGTAG = "GeckoServiceNotificationClient";

    private final ServiceConnection mConnection = new NotificationServiceConnection();
    private boolean mBound;
    private final Context mContext;

    public ServiceNotificationClient(Context context) {
        mContext = context;
    }

    @Override
    protected void bind() {
        super.bind();
        final Intent intent = new Intent(mContext, NotificationService.class);
        mContext.bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    protected void unbind() {
        
        
        
        
        
        
        super.unbind();

        if (mBound) {
            mBound = false;
            mContext.unbindService(mConnection);
        }
    }

    class NotificationServiceConnection implements ServiceConnection {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            final NotificationService.NotificationBinder binder =
                    (NotificationService.NotificationBinder) service;
            connectHandler(binder.getService().getNotificationHandler());
            mBound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            
            
            
            
            
            Log.e(LOGTAG, "Notification service disconnected", new Exception());
        }
    }
}
