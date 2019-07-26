




package org.mozilla.gecko.health;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.healthreport.AndroidConfigurationProvider;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ConfigurationProvider;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;
import org.mozilla.gecko.background.healthreport.HealthReportDatabaseStorage;
import org.mozilla.gecko.background.healthreport.HealthReportGenerator;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.ContentProviderClient;
import android.content.Context;
import android.util.Log;









public class BrowserHealthReporter implements GeckoEventListener {
    private static final String LOGTAG = "GeckoHealthRep";

    public static final String EVENT_REQUEST  = "HealthReport:Request";
    public static final String EVENT_RESPONSE = "HealthReport:Response";

    protected final Context context;

    public BrowserHealthReporter() {
        EventDispatcher.getInstance().registerGeckoThreadListener(this, EVENT_REQUEST);

        context = GeckoAppShell.getContext();
        if (context == null) {
            throw new IllegalStateException("Null Gecko context");
        }
    }

    public void uninit() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this, EVENT_REQUEST);
    }

    












    public JSONObject generateReport(long since, long lastPingTime, String profilePath) throws JSONException {
        
        
        
        
        
        
        ContentProviderClient client = EnvironmentBuilder.getContentProviderClient(context);
        if (client == null) {
            throw new IllegalStateException("Could not fetch Health Report content provider.");
        }

        try {
            
            
            HealthReportDatabaseStorage storage = EnvironmentBuilder.getStorage(client, profilePath);
            if (storage == null) {
                throw new IllegalStateException("No storage in Health Reporter.");
            }

            HealthReportGenerator generator = new HealthReportGenerator(storage);
            ConfigurationProvider configProvider = new AndroidConfigurationProvider(context);
            JSONObject report = generator.generateDocument(since, lastPingTime, profilePath, configProvider);
            if (report == null) {
                throw new IllegalStateException("Not enough profile information to generate report.");
            }
            return report;
        } finally {
            client.release();
        }
    }

    







    protected long getLastUploadLocalTime() {
        return context
            .getSharedPreferences(HealthReportConstants.PREFS_BRANCH, 0)
            .getLong(HealthReportConstants.PREF_LAST_UPLOAD_LOCAL_TIME, 0L);
    }

    








    public JSONObject generateReport() throws JSONException {
        GeckoProfile profile = GeckoAppShell.getGeckoInterface().getProfile();
        String profilePath = profile.getDir().getAbsolutePath();

        long since = System.currentTimeMillis() - GlobalConstants.MILLISECONDS_PER_SIX_MONTHS;
        long lastPingTime = Math.max(getLastUploadLocalTime(), HealthReportConstants.EARLIEST_LAST_PING);

        return generateReport(since, lastPingTime, profilePath);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    JSONObject report = null;
                    try {
                        report = generateReport(); 
                    } catch (Exception e) {
                        Log.e(LOGTAG, "Generating report failed; responding with empty report.", e);
                        report = new JSONObject();
                    }

                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(EVENT_RESPONSE, report.toString()));
                }
           });
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }
}

