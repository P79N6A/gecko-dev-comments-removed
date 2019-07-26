




package org.mozilla.gecko.health;

import android.content.SharedPreferences;
import android.util.Log;

import org.mozilla.gecko.GeckoApp;

import org.json.JSONException;
import org.json.JSONObject;

public class SessionInformation {
    private static final String LOG_TAG = "GeckoSessInfo";

    public static final String PREFS_SESSION_START = "sessionStart";

    public final long wallStartTime;    
    public final long realStartTime;    

    private final boolean wasOOM;
    private final boolean wasStopped;

    private volatile long timedGeckoStartup = -1;
    private volatile long timedJavaStartup = -1;

    
    
    public SessionInformation(long wallTime, long realTime) {
        this(wallTime, realTime, false, false);
    }

    
    public SessionInformation(long wallTime, long realTime, boolean wasOOM, boolean wasStopped) {
        this.wallStartTime = wallTime;
        this.realStartTime = realTime;
        this.wasOOM = wasOOM;
        this.wasStopped = wasStopped;
    }

    








    public static SessionInformation fromSharedPrefs(SharedPreferences prefs) {
        boolean wasOOM = prefs.getBoolean(GeckoApp.PREFS_OOM_EXCEPTION, false);
        boolean wasStopped = prefs.getBoolean(GeckoApp.PREFS_WAS_STOPPED, true);
        long wallStartTime = prefs.getLong(PREFS_SESSION_START, 0L);
        long realStartTime = 0L;
        Log.d(LOG_TAG, "Building SessionInformation from prefs: " +
                       wallStartTime + ", " + realStartTime + ", " +
                       wasStopped + ", " + wasOOM);
        return new SessionInformation(wallStartTime, realStartTime, wasOOM, wasStopped);
    }

    



    public static SessionInformation forRuntimeTransition() {
        final boolean wasOOM = false;
        final boolean wasStopped = true;
        final long wallStartTime = System.currentTimeMillis();
        final long realStartTime = android.os.SystemClock.elapsedRealtime();
        Log.v(LOG_TAG, "Recording runtime session transition: " +
                       wallStartTime + ", " + realStartTime);
        return new SessionInformation(wallStartTime, realStartTime, wasOOM, wasStopped);
    }

    public boolean wasKilled() {
        return wasOOM || !wasStopped;
    }

    





    public void recordBegin(SharedPreferences.Editor editor) {
        Log.d(LOG_TAG, "Recording start of session: " + this.wallStartTime);
        editor.putLong(PREFS_SESSION_START, this.wallStartTime);
    }

    



    public void recordCompletion(SharedPreferences.Editor editor) {
        Log.d(LOG_TAG, "Recording session done: " + this.wallStartTime);
        editor.remove(PREFS_SESSION_START);
    }

    


    public JSONObject getCompletionJSON(String reason, long realEndTime) throws JSONException {
        long durationSecs = (realEndTime - this.realStartTime) / 1000;
        JSONObject out = new JSONObject();
        out.put("r", reason);
        out.put("d", durationSecs);
        if (this.timedGeckoStartup > 0) {
            out.put("sg", this.timedGeckoStartup);
        }
        if (this.timedJavaStartup > 0) {
            out.put("sj", this.timedJavaStartup);
        }
        return out;
    }

    public JSONObject getCrashedJSON() throws JSONException {
        JSONObject out = new JSONObject();
        
        
        
        out.put("oom", this.wasOOM ? 1 : 0);
        out.put("stopped", this.wasStopped ? 1 : 0);
        out.put("r", "A");
        return out;
    }

    public void setTimedGeckoStartup(final long duration) {
        timedGeckoStartup = duration;
    }

    public void setTimedJavaStartup(final long duration) {
        timedJavaStartup = duration;
    }
}
