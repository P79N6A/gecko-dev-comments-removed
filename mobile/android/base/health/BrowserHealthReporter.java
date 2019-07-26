




package org.mozilla.gecko.health;

import android.content.ContentProviderClient;
import android.content.Context;
import android.util.Log;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;

import org.mozilla.gecko.background.healthreport.EnvironmentBuilder;
import org.mozilla.gecko.background.healthreport.HealthReportDatabaseStorage;
import org.mozilla.gecko.background.healthreport.HealthReportGenerator;

import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONException;
import org.json.JSONObject;









public class BrowserHealthReporter implements GeckoEventListener {
    private static final String LOGTAG = "GeckoHealthRep";

    public static final long MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;
    public static final long MILLISECONDS_PER_SIX_MONTHS = 180 * MILLISECONDS_PER_DAY;

    public static final String EVENT_REQUEST  = "HealthReport:Request";
    public static final String EVENT_RESPONSE = "HealthReport:Response";

    public BrowserHealthReporter() {
        GeckoAppShell.registerEventListener(EVENT_REQUEST, this);

        final Context context = GeckoAppShell.getContext();
        if (context == null) {
            throw new IllegalStateException("Null Gecko context");
        }
    }

    public void uninit() {
        GeckoAppShell.unregisterEventListener(EVENT_REQUEST, this);
    }

    









    public JSONObject generateReport(long since, long lastPingTime, String profilePath) throws JSONException {
        final Context context = GeckoAppShell.getContext();
        if (context == null) {
            Log.e(LOGTAG, "Null Gecko context; returning null report.", new RuntimeException());
            return null;
        }

        
        
        
        
        
        
        ContentProviderClient client = EnvironmentBuilder.getContentProviderClient(context);
        if (client == null) {
            throw new IllegalStateException("Could not fetch Health Report content provider.");
        }

        try {
            
            
            HealthReportDatabaseStorage storage = EnvironmentBuilder.getStorage(client, profilePath);
            if (storage == null) {
                Log.e(LOGTAG, "No storage in health reporter; returning null report.", new RuntimeException());
                return null;
            }

            HealthReportGenerator generator = new HealthReportGenerator(storage);
            return generator.generateDocument(since, lastPingTime, profilePath);
        } finally {
            client.release();
        }
    }

    




    public JSONObject generateReport() throws JSONException {
        GeckoProfile profile = GeckoAppShell.getGeckoInterface().getProfile();
        String profilePath = profile.getDir().getAbsolutePath();

        long since = System.currentTimeMillis() - MILLISECONDS_PER_SIX_MONTHS;
         
        long lastPingTime = since;
        return generateReport(since, lastPingTime, profilePath);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    JSONObject report = new JSONObject();
                    try {
                        report = generateReport();
                    } catch (Exception e) {
                        Log.e(LOGTAG, "Generating report failed; responding with null.", e);
                    }

                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(EVENT_RESPONSE, report.toString()));
                }
           });
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }
}

