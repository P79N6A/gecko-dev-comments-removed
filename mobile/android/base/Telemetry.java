




package org.mozilla.gecko;

import android.os.SystemClock;
import android.util.Log;

public class Telemetry {
    private static final String LOGTAG = "Telemetry";

    
    
    public static void HistogramAdd(String name,
                                    int value) {
        GeckoEvent event =
            GeckoEvent.createTelemetryHistogramAddEvent(name, value);
        GeckoAppShell.sendEventToGecko(event);
    }

    public static class Timer {
        private long mStartTime;
        private String mName;
        private boolean mHasFinished;
        private volatile long mElapsed = -1;

        public Timer(String name) {
            mName = name;
            mStartTime = SystemClock.uptimeMillis();
            mHasFinished = false;
        }

        public void cancel() {
            mHasFinished = true;
        }

        public long getElapsed() {
          return mElapsed;
        }

        public void stop() {
            
            if (mHasFinished) {
                return;
            } else {
                mHasFinished = true;
            }

            final long elapsed = SystemClock.uptimeMillis() - mStartTime;
            mElapsed = elapsed;
            if (elapsed < Integer.MAX_VALUE) {
                HistogramAdd(mName, (int)(elapsed));
            } else {
                Log.e(LOGTAG, "Duration of " + elapsed + " ms is too long to add to histogram.");
            }
        }
    }
}
