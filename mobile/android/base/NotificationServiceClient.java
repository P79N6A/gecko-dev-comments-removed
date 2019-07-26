




package org.mozilla.gecko;

import java.util.LinkedList;
import java.util.concurrent.ConcurrentHashMap;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;




public class NotificationServiceClient {
    private static final String LOGTAG = "GeckoNotificationServiceClient";

    private volatile NotificationService mService;
    private final ServiceConnection mConnection = new NotificationServiceConnection();
    private boolean mBound;
    private final Context mContext;
    private final LinkedList<Runnable> mTaskQueue = new LinkedList<Runnable>();
    private final ConcurrentHashMap<Integer, UpdateRunnable> mUpdatesMap =
            new ConcurrentHashMap<Integer, UpdateRunnable>();

    public NotificationServiceClient(Context context) {
        mContext = context;
    }

    




    private class UpdateRunnable implements Runnable {
        private long mProgress;
        private long mProgressMax;
        private String mAlertText;
        final private int mNotificationID;

        public UpdateRunnable(int notificationID) {
            mNotificationID = notificationID;
        }

        public synchronized boolean updateProgress(long progress, long progressMax, String alertText) {
            if (progress == mProgress
                    && mProgressMax == progressMax
                    && TextUtils.equals(mAlertText, alertText)) {
                return false;
            }

            mProgress = progress;
            mProgressMax = progressMax;
            mAlertText = alertText;
            return true;
        }

        @Override
        public void run() {
            long progress;
            long progressMax;
            String alertText;

            synchronized (this) {
                progress = mProgress;
                progressMax = mProgressMax;
                alertText = mAlertText;
            }

            mService.update(mNotificationID, progress, progressMax, alertText);
        }
    };

    




    public synchronized void add(final int notificationID, final String aImageUrl,
            final String aAlertTitle, final String aAlertText, final PendingIntent contentIntent) {
        mTaskQueue.add(new Runnable() {
            @Override
            public void run() {
                mService.add(notificationID, aImageUrl, aAlertTitle, aAlertText, contentIntent);
            }
        });
        notify();

        if (!mBound) {
            bind();
        }
    }

    




    public void update(final int notificationID, final long aProgress, final long aProgressMax,
            final String aAlertText) {
        UpdateRunnable runnable = mUpdatesMap.get(notificationID);

        if (runnable == null) {
            runnable = new UpdateRunnable(notificationID);
            mUpdatesMap.put(notificationID, runnable);
        }

        
        
        if (!runnable.updateProgress(aProgress, aProgressMax, aAlertText)) {
            return;
        }

        synchronized (this) {
            if (mBound) {
                mTaskQueue.add(runnable);
                notify();
            }
        }
    }

    




    public synchronized void remove(final int notificationID) {
        if (!mBound) {
            return;
        }

        mTaskQueue.add(new Runnable() {
            @Override
            public void run() {
                mService.remove(notificationID);
                mUpdatesMap.remove(notificationID);
            }
        });
        notify();
    }

    




    public boolean isProgressStyle(int notificationID) {
        final NotificationService service = mService;
        return service != null && service.isProgressStyle(notificationID);
    }

    private void bind() {
        mBound = true;
        final Intent intent = new Intent(mContext, NotificationService.class);
        mContext.bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
    }

    private void unbind() {
        if (mBound) {
            mBound = false;
            mContext.unbindService(mConnection);
            mUpdatesMap.clear();
        }
    }

    class NotificationServiceConnection implements ServiceConnection, Runnable {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            final NotificationService.NotificationBinder binder =
                    (NotificationService.NotificationBinder) service;
            mService = binder.getService();

            new Thread(this).start();
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            
            
            
            
            
            Log.e(LOGTAG, "Notification service disconnected", new Exception());
        }

        @Override
        public void run() {
            Runnable r;
            try {
                while (true) {
                    
                    
                    synchronized (NotificationServiceClient.this) {
                        r = mTaskQueue.poll();
                        while (r == null) {
                            if (mService.isDone()) {
                                
                                
                                
                                
                                
                                
                                unbind();
                                return;
                            }
                            NotificationServiceClient.this.wait();
                            r = mTaskQueue.poll();
                        }
                    }
                    r.run();
                }
            } catch (InterruptedException e) {
                Log.e(LOGTAG, "Notification task queue processing interrupted", e);
            }
        }
    }
}
