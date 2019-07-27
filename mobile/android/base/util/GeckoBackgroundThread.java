



package org.mozilla.gecko.util;

import android.os.Handler;
import android.os.Looper;

import java.util.concurrent.SynchronousQueue;

final class GeckoBackgroundThread extends Thread {
    private static final String LOOPER_NAME = "GeckoBackgroundThread";

    
    private static Handler handler;
    private static Thread thread;

    
    
    private Runnable initialRunnable;

    
    private GeckoBackgroundThread(final Runnable initialRunnable) {
        this.initialRunnable = initialRunnable;
    }

    @Override
    public void run() {
        setName(LOOPER_NAME);
        Looper.prepare();

        synchronized (GeckoBackgroundThread.class) {
            handler = new Handler();
            GeckoBackgroundThread.class.notify();
        }

        if (initialRunnable != null) {
            initialRunnable.run();
            initialRunnable = null;
        }

        Looper.loop();
    }

    private static void startThread(final Runnable initialRunnable) {
        thread = new GeckoBackgroundThread(initialRunnable);
        ThreadUtils.setBackgroundThread(thread);

        thread.setDaemon(true);
        thread.start();
    }

    
     static synchronized Handler getHandler() {
        if (thread == null) {
            startThread(null);
        }

        while (handler == null) {
            try {
                GeckoBackgroundThread.class.wait();
            } catch (final InterruptedException e) {
            }
        }
        return handler;
    }

     static synchronized void post(final Runnable runnable) {
        if (thread == null) {
            startThread(runnable);
            return;
        }
        getHandler().post(runnable);
    }
}
