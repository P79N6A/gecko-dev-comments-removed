



package org.mozilla.gecko;

import java.util.concurrent.atomic.AtomicBoolean;

import android.app.Activity;

public final class RobocopUtils {
    private static final int MAX_WAIT_MS = 20000;

    private RobocopUtils() {}

    public static void runOnUiThreadSync(Activity activity, final Runnable runnable) {
        final AtomicBoolean sentinel = new AtomicBoolean(false);

        
        activity.runOnUiThread(
            new Runnable() {
                @Override
                public void run() {
                    runnable.run();

                    synchronized (sentinel) {
                        sentinel.set(true);
                        sentinel.notifyAll();
                    }
                }
            }
        );


        
        
        long startTimestamp = System.currentTimeMillis();

        synchronized (sentinel) {
            while (!sentinel.get()) {
                try {
                    sentinel.wait(MAX_WAIT_MS);
                } catch (InterruptedException e) {
                    FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR, e);
                }

                
                if (System.currentTimeMillis() - startTimestamp >= MAX_WAIT_MS) {
                    FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR,
                                                  "time-out waiting for UI thread");
                    FennecNativeDriver.logAllStackTraces(FennecNativeDriver.LogLevel.ERROR);

                    return;
                }
            }
        }
    }
}
