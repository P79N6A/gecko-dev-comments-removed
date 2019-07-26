




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.RobocopTarget;

import android.os.SystemClock;
import android.util.Log;











@RobocopTarget
public class Telemetry {
    private static final String LOGTAG = "Telemetry";

    public static long uptime() {
        return SystemClock.uptimeMillis();
    }

    public static long realtime() {
        return SystemClock.elapsedRealtime();
    }

    
    
    public static void HistogramAdd(String name, int value) {
        GeckoEvent event = GeckoEvent.createTelemetryHistogramAddEvent(name, value);
        GeckoAppShell.sendEventToGecko(event);
    }

    public abstract static class Timer {
        private final long mStartTime;
        private final String mName;

        private volatile boolean mHasFinished = false;
        private volatile long mElapsed = -1;

        protected abstract long now();

        public Timer(String name) {
            mName = name;
            mStartTime = now();
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
            }

            mHasFinished = true;

            final long elapsed = now() - mStartTime;
            if (elapsed < 0) {
                Log.e(LOGTAG, "Current time less than start time -- clock shenanigans?");
                return;
            }

            mElapsed = elapsed;
            if (elapsed > Integer.MAX_VALUE) {
                Log.e(LOGTAG, "Duration of " + elapsed + "ms is too great to add to histogram.");
                return;
            }

            HistogramAdd(mName, (int)(elapsed));
        }
    }

    public static class RealtimeTimer extends Timer {
        public RealtimeTimer(String name) {
            super(name);
        }

        @Override
        protected long now() {
            return Telemetry.realtime();
        }
    }

    public static class UptimeTimer extends Timer {
        public UptimeTimer(String name) {
            super(name);
        }

        @Override
        protected long now() {
            return Telemetry.uptime();
        }
    }

    public static void startUISession(String sessionName) {
        GeckoEvent event = GeckoEvent.createTelemetryUISessionStartEvent(sessionName, realtime());
        GeckoAppShell.sendEventToGecko(event);
    }

    public static void stopUISession(String sessionName, String reason) {
        GeckoEvent event = GeckoEvent.createTelemetryUISessionStopEvent(sessionName, reason, realtime());
        GeckoAppShell.sendEventToGecko(event);
    }

    public static void sendUIEvent(String action, String method, long timestamp, String extras) {
        GeckoEvent event = GeckoEvent.createTelemetryUIEvent(action, method, timestamp, extras);
        GeckoAppShell.sendEventToGecko(event);
    }

    public static void sendUIEvent(String action, String method, long timestamp) {
        sendUIEvent(action, method, timestamp, null);
    }

    public static void sendUIEvent(String action, String method, String extras) {
        sendUIEvent(action, method, realtime(), extras);
    }

    public static void sendUIEvent(String action, String method) {
        sendUIEvent(action, method, realtime(), null);
    }

    public static void sendUIEvent(String action) {
        sendUIEvent(action, null, realtime(), null);
    }
}
