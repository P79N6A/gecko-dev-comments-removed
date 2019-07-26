




package org.mozilla.gecko;

import java.util.LinkedList;
import java.util.concurrent.ConcurrentHashMap;

import android.app.PendingIntent;
import android.text.TextUtils;
import android.util.Log;




public abstract class NotificationClient {
    private static final String LOGTAG = "GeckoNotificationClient";

    private volatile NotificationHandler mHandler;
    private boolean mReady;
    private final LinkedList<Runnable> mTaskQueue = new LinkedList<Runnable>();
    private final ConcurrentHashMap<Integer, UpdateRunnable> mUpdatesMap =
            new ConcurrentHashMap<Integer, UpdateRunnable>();

    




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

            mHandler.update(mNotificationID, progress, progressMax, alertText);
        }
    };

    




    public synchronized void add(final int notificationID, final String aImageUrl,
            final String aAlertTitle, final String aAlertText, final PendingIntent contentIntent) {
        mTaskQueue.add(new Runnable() {
            @Override
            public void run() {
                mHandler.add(notificationID, aImageUrl, aAlertTitle, aAlertText, contentIntent);
            }
        });
        notify();

        if (!mReady) {
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
            if (mReady) {
                mTaskQueue.add(runnable);
                notify();
            }
        }
    }

    




    public synchronized void remove(final int notificationID) {
        if (!mReady) {
            return;
        }

        mTaskQueue.add(new Runnable() {
            @Override
            public void run() {
                mHandler.remove(notificationID);
                mUpdatesMap.remove(notificationID);
            }
        });
        notify();
    }

    




    public boolean isProgressStyle(int notificationID) {
        final NotificationHandler handler = mHandler;
        return handler != null && handler.isProgressStyle(notificationID);
    }

    protected void bind() {
        mReady = true;
    }

    protected void unbind() {
        mReady = false;
        mUpdatesMap.clear();
    }

    protected void connectHandler(NotificationHandler handler) {
        mHandler = handler;
        new Thread(new NotificationRunnable()).start();
    }

    private class NotificationRunnable implements Runnable {
        @Override
        public void run() {
            Runnable r;
            try {
                while (true) {
                    
                    
                    synchronized (NotificationClient.this) {
                        r = mTaskQueue.poll();
                        while (r == null) {
                            if (mHandler.isDone()) {
                                unbind();
                                return;
                            }
                            NotificationClient.this.wait();
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
