



package org.mozilla.gecko;

import android.app.Activity;

import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.TimeUnit;

public final class RobocopUtils {
    private static final int MAX_WAIT_MS = 20000;

    private RobocopUtils() {}

    public static void runOnUiThreadSync(Activity activity, final Runnable runnable) {
        final SynchronousQueue syncQueue = new SynchronousQueue();
        activity.runOnUiThread(
            new Runnable() {
                public void run() {
                    runnable.run();
                    try {
                        syncQueue.put(new Object());
                    } catch (InterruptedException e) {
                        FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR, e);
                    }
                }
            });
        try {
            
            if (syncQueue.poll(MAX_WAIT_MS, TimeUnit.MILLISECONDS) == null) {
                FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR,
                                       "time-out waiting for UI thread");
                FennecNativeDriver.logAllStackTraces(FennecNativeDriver.LogLevel.ERROR);
            }
        } catch (InterruptedException e) {
            FennecNativeDriver.log(FennecNativeDriver.LogLevel.ERROR, e);
        }
    }
}
