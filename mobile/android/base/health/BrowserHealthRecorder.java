




package org.mozilla.gecko.health;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Scanner;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.background.healthreport.AndroidConfigurationProvider;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder;
import org.mozilla.gecko.background.healthreport.EnvironmentBuilder.ConfigurationProvider;
import org.mozilla.gecko.background.healthreport.HealthReportDatabaseStorage;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.Field;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.MeasurementFields;
import org.mozilla.gecko.background.healthreport.ProfileInformationCache;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.distribution.Distribution.DistributionDescriptor;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.ContentProviderClient;
import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

















public class BrowserHealthRecorder implements HealthRecorder, GeckoEventListener {
    private static final String LOG_TAG = "GeckoHealthRec";
    private static final String PREF_ACCEPT_LANG = "intl.accept_languages";
    private static final String PREF_BLOCKLIST_ENABLED = "extensions.blocklist.enabled";
    private static final String EVENT_SNAPSHOT = "HealthReport:Snapshot";
    private static final String EVENT_ADDONS_CHANGE = "Addons:Change";
    private static final String EVENT_ADDONS_UNINSTALLING = "Addons:Uninstalling";
    private static final String EVENT_PREF_CHANGE = "Pref:Change";

    
    
    
    public static final String EVENT_KEYWORD_SEARCH = "Search:Keyword";

    
    public static final String EVENT_SEARCH = "Search:Event";

    public enum State {
        NOT_INITIALIZED,
        INITIALIZING,
        INITIALIZED,
        INITIALIZATION_FAILED,
        CLOSED
    }

    protected volatile State state = State.NOT_INITIALIZED;

    private final AtomicBoolean orphanChecked = new AtomicBoolean(false);
    private volatile int env = -1;

    final EventDispatcher dispatcher;
    final ProfileInformationCache profileCache;
    private ContentProviderClient client;
    private volatile HealthReportDatabaseStorage storage;
    private final ConfigurationProvider configProvider;
    private final SharedPreferences prefs;

    
    
    private final SessionInformation previousSession;
    private volatile SessionInformation session;

    public void setCurrentSession(SessionInformation session) {
        this.session = session;
    }

    public void recordGeckoStartupTime(long duration) {
        if (this.session == null) {
            return;
        }
        this.session.setTimedGeckoStartup(duration);
    }
    public void recordJavaStartupTime(long duration) {
        if (this.session == null) {
            return;
        }
        this.session.setTimedJavaStartup(duration);
    }

    




    public BrowserHealthRecorder(final Context context,
                                 final SharedPreferences appPrefs,
                                 final String profilePath,
                                 final EventDispatcher dispatcher,
                                 final String osLocale,
                                 final String appLocale,
                                 SessionInformation previousSession) {
        Log.d(LOG_TAG, "Initializing. Dispatcher is " + dispatcher);
        this.dispatcher = dispatcher;
        this.previousSession = previousSession;

        this.client = EnvironmentBuilder.getContentProviderClient(context);
        if (this.client == null) {
            throw new IllegalStateException("Could not fetch Health Report content provider.");
        }

        this.storage = EnvironmentBuilder.getStorage(this.client, profilePath);
        if (this.storage == null) {
            
            
            
            
            this.client.release();
            this.client = null;
        }

        
        
        
        this.profileCache = new ProfileInformationCache(profilePath);
        try {
            this.initialize(context, profilePath, osLocale, appLocale);
        } catch (Exception e) {
            Log.e(LOG_TAG, "Exception initializing.", e);
        }

        this.configProvider = new AndroidConfigurationProvider(context);

        this.prefs = appPrefs;
    }

    public boolean isEnabled() {
        return true;
    }

    



    public synchronized void close() {
        switch (this.state) {
            case CLOSED:
                Log.w(LOG_TAG, "Ignoring attempt to double-close closed BrowserHealthRecorder.");
                return;
            case INITIALIZED:
                Log.i(LOG_TAG, "Closing Health Report client.");
                break;
            default:
                Log.i(LOG_TAG, "Closing incompletely initialized BrowserHealthRecorder.");
        }

        this.state = State.CLOSED;
        this.unregisterEventListeners();

        
        this.storage = null;
        if (this.client != null) {
            this.client.release();
            this.client = null;
        }
    }

    private void unregisterEventListeners() {
        if (state != State.INITIALIZED) {
            return;
        }
        dispatcher.unregisterGeckoThreadListener(this,
            EVENT_SNAPSHOT,
            EVENT_ADDONS_CHANGE,
            EVENT_ADDONS_UNINSTALLING,
            EVENT_PREF_CHANGE,
            EVENT_KEYWORD_SEARCH,
            EVENT_SEARCH);
    }

    public void onAppLocaleChanged(String to) {
        Log.d(LOG_TAG, "Setting health recorder app locale to " + to);
        this.profileCache.beginInitialization();
        this.profileCache.setAppLocale(to);
    }

    public void onAddonChanged(String id, JSONObject json) {
        this.profileCache.beginInitialization();
        try {
            this.profileCache.updateJSONForAddon(id, json);
        } catch (IllegalStateException e) {
            Log.w(LOG_TAG, "Attempted to update add-on cache prior to full init.", e);
        }
    }

    public void onAddonUninstalling(String id) {
        this.profileCache.beginInitialization();
        try {
            this.profileCache.removeAddon(id);
        } catch (IllegalStateException e) {
            Log.w(LOG_TAG, "Attempted to update add-on cache prior to full init.", e);
        }
    }

    










    public synchronized void onEnvironmentChanged() {
        onEnvironmentChanged(true, "E");
    }

    




    public synchronized void onEnvironmentChanged(final boolean startNewSession, final String sessionEndReason) {
        final int previousEnv = this.env;
        this.env = -1;
        try {
            profileCache.completeInitialization();
        } catch (java.io.IOException e) {
            Log.e(LOG_TAG, "Error completing profile cache initialization.", e);
            this.state = State.INITIALIZATION_FAILED;
            return;
        }

        final int updatedEnv = ensureEnvironment();

        if (updatedEnv == -1 ||
            updatedEnv == previousEnv) {
            Log.v(LOG_TAG, "Environment didn't change.");
            return;
        }
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                try {
                    onEnvironmentTransition(previousEnv, updatedEnv, startNewSession, sessionEndReason);
                } catch (Exception e) {
                    Log.w(LOG_TAG, "Could not record environment transition.", e);
                }
            }
        });
    }

    protected synchronized int ensureEnvironment() {
        if (!(state == State.INITIALIZING ||
              state == State.INITIALIZED)) {
            throw new IllegalStateException("Not initialized.");
        }

        if (this.env != -1) {
            return this.env;
        }
        if (this.storage == null) {
            
            return -1;
        }
        return this.env = EnvironmentBuilder.registerCurrentEnvironment(this.storage,
                                                                        this.profileCache,
                                                                        this.configProvider);
    }

    private static final String getTimesPath(final String profilePath) {
        return profilePath + File.separator + "times.json";
    }

    




    protected static long getProfileInitTimeFromFile(final String profilePath) {
        final File times = new File(getTimesPath(profilePath));
        Log.d(LOG_TAG, "Looking for " + times.getAbsolutePath());
        if (!times.exists()) {
            return -1;
        }

        Log.d(LOG_TAG, "Using times.json for profile creation time.");
        Scanner scanner = null;
        try {
            scanner = new Scanner(times, "UTF-8");
            final String contents = scanner.useDelimiter("\\A").next();
            return new JSONObject(contents).getLong("created");
        } catch (Exception e) {
            
            
            Log.w(LOG_TAG, "Failed to read times.json.", e);
        } finally {
            if (scanner != null) {
                scanner.close();
            }
        }
        return -1;
    }

    


    protected static long getPackageInstallTime(final Context context) {
        try {
            return context.getPackageManager().getPackageInfo(AppConstants.ANDROID_PACKAGE_NAME, 0).firstInstallTime;
        } catch (android.content.pm.PackageManager.NameNotFoundException e) {
            Log.e(LOG_TAG, "Unable to fetch our own package info. This should never occur.", e);
        }
        return -1;
    }

    private static long getProfileInitTimeHeuristic(final Context context, final String profilePath) {
        
        
        final long time = getPackageInstallTime(context);
        if (time != -1) {
            return time;
        }

        
        
        Log.d(LOG_TAG, "Using profile directory modified time as proxy for profile creation time.");
        return new File(profilePath).lastModified();
    }

    private static long getAndPersistProfileInitTime(final Context context, final String profilePath) {
        
        long time = getProfileInitTimeFromFile(profilePath);
        if (time > 0) {
            Log.d(LOG_TAG, "Incorporating environment: times.json profile creation = " + time);
            return time;
        }

        
        time = getProfileInitTimeHeuristic(context, profilePath);

        if (time > 0) {
            
            try {
                FileOutputStream stream = new FileOutputStream(getTimesPath(profilePath));
                OutputStreamWriter writer = new OutputStreamWriter(stream, Charset.forName("UTF-8"));
                try {
                    writer.append("{\"created\": " + time + "}\n");
                } finally {
                    writer.close();
                }
            } catch (Exception e) {
                
                Log.w(LOG_TAG, "Couldn't write times.json.", e);
            }
        }

        Log.d(LOG_TAG, "Incorporating environment: profile creation = " + time);
        return time;
    }

    private void onPrefMessage(final String pref, final JSONObject message) {
        Log.d(LOG_TAG, "Incorporating environment: " + pref);
        if (PREF_ACCEPT_LANG.equals(pref)) {
            
            try {
                this.profileCache.beginInitialization();
                this.profileCache.setAcceptLangUserSet(message.getBoolean("isUserSet"));
            } catch (JSONException ex) {
                Log.w(LOG_TAG, "Unexpected JSONException fetching isUserSet for " + pref);
            }
            return;
        }

        
        try {
            boolean value = message.getBoolean("value");

            if (AppConstants.TELEMETRY_PREF_NAME.equals(pref)) {
                this.profileCache.beginInitialization();
                this.profileCache.setTelemetryEnabled(value);
                return;
            }

            if (PREF_BLOCKLIST_ENABLED.equals(pref)) {
                this.profileCache.beginInitialization();
                this.profileCache.setBlocklistEnabled(value);
                return;
            }
        } catch (JSONException ex) {
            Log.w(LOG_TAG, "Unexpected JSONException fetching boolean value for " + pref);
            return;
        }
        Log.w(LOG_TAG, "Unexpected pref: " + pref);
    }

    


    private void initializeStorage() {
        Log.d(LOG_TAG, "Done initializing profile cache. Beginning storage init.");

        final BrowserHealthRecorder self = this;
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                synchronized (self) {
                    if (state != State.INITIALIZING) {
                        Log.w(LOG_TAG, "Unexpected state during init: " + state);
                        return;
                    }

                    
                    if (storage == null) {
                        Log.w(LOG_TAG, "Storage is null during init; shutting down?");
                        if (state == State.INITIALIZING) {
                            state = State.INITIALIZATION_FAILED;
                        }
                        return;
                    }

                    try {
                        storage.beginInitialization();
                    } catch (Exception e) {
                        Log.e(LOG_TAG, "Failed to init storage.", e);
                        state = State.INITIALIZATION_FAILED;
                        return;
                    }

                    try {
                        
                        dispatcher.registerGeckoThreadListener(self,
                            EVENT_ADDONS_UNINSTALLING,
                            EVENT_ADDONS_CHANGE,
                            EVENT_PREF_CHANGE);

                        
                        initializeSessionsProvider();
                        initializeSearchProvider();

                        Log.d(LOG_TAG, "Ensuring environment.");
                        ensureEnvironment();

                        Log.d(LOG_TAG, "Finishing init.");
                        storage.finishInitialization();
                        state = State.INITIALIZED;
                    } catch (Exception e) {
                        state = State.INITIALIZATION_FAILED;
                        storage.abortInitialization();
                        Log.e(LOG_TAG, "Initialization failed.", e);
                        return;
                    }

                    
                    checkForOrphanSessions();
                }
            }
        });
    }

    


    private synchronized void initialize(final Context context,
                                         final String profilePath,
                                         final String osLocale,
                                         final String appLocale)
        throws java.io.IOException {

        Log.d(LOG_TAG, "Initializing profile cache.");
        this.state = State.INITIALIZING;

        
        if (this.profileCache.restoreUnlessInitialized()) {
            this.profileCache.updateLocales(osLocale, appLocale);
            this.profileCache.completeInitialization();

            Log.d(LOG_TAG, "Successfully restored state. Initializing storage.");
            initializeStorage();
            return;
        }

        
        this.profileCache.beginInitialization();
        this.profileCache.setProfileCreationTime(getAndPersistProfileInitTime(context, profilePath));
        this.profileCache.setOSLocale(osLocale);
        this.profileCache.setAppLocale(appLocale);

        
        
        final Distribution distribution = Distribution.getInstance(context);
        distribution.addOnDistributionReadyCallback(new Runnable() {
            @Override
            public void run() {
                Log.d(LOG_TAG, "Running post-distribution task: health recorder.");
                final DistributionDescriptor desc = distribution.getDescriptor();
                if (desc != null && desc.valid) {
                    profileCache.setDistributionString(desc.id, desc.version);
                }
                Log.d(LOG_TAG, "Requesting all add-ons and FHR prefs from Gecko.");
                dispatcher.registerGeckoThreadListener(BrowserHealthRecorder.this, EVENT_SNAPSHOT);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HealthReport:RequestSnapshot", null));
            }
        });
    }

    



    protected void onEnvironmentTransition(int prev, int env, boolean startNewSession, String sessionEndReason) {
        if (this.state != State.INITIALIZED) {
            Log.d(LOG_TAG, "Not initialized: not recording env transition (" + prev + " => " + env + ").");
            return;
        }

        final SharedPreferences.Editor editor = this.prefs.edit();

        recordSessionEnd(sessionEndReason, editor, prev);

        if (!startNewSession) {
            editor.commit();
            return;
        }

        final SessionInformation newSession = SessionInformation.forRuntimeTransition();
        setCurrentSession(newSession);
        newSession.recordBegin(editor);
        editor.commit();
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (EVENT_SNAPSHOT.equals(event)) {
                Log.d(LOG_TAG, "Got all add-ons and prefs.");
                try {
                    JSONObject json = message.getJSONObject("json");
                    JSONObject addons = json.getJSONObject("addons");
                    Log.i(LOG_TAG, "Persisting " + addons.length() + " add-ons.");
                    profileCache.setJSONForAddons(addons);

                    JSONObject prefs = json.getJSONObject("prefs");
                    Log.i(LOG_TAG, "Persisting prefs.");
                    Iterator<?> keys = prefs.keys();
                    while (keys.hasNext()) {
                        String pref = (String) keys.next();
                        this.onPrefMessage(pref, prefs.getJSONObject(pref));
                    }

                    profileCache.completeInitialization();
                } catch (java.io.IOException e) {
                    Log.e(LOG_TAG, "Error completing profile cache initialization.", e);
                    state = State.INITIALIZATION_FAILED;
                    return;
                }

                if (state == State.INITIALIZING) {
                    initializeStorage();
                } else {
                    this.onEnvironmentChanged();
                }

                return;
            }

            if (EVENT_ADDONS_UNINSTALLING.equals(event)) {
                this.onAddonUninstalling(message.getString("id"));
                this.onEnvironmentChanged();
                return;
            }

            if (EVENT_ADDONS_CHANGE.equals(event)) {
                this.onAddonChanged(message.getString("id"), message.getJSONObject("json"));
                this.onEnvironmentChanged();
                return;
            }

            if (EVENT_PREF_CHANGE.equals(event)) {
                final String pref = message.getString("pref");
                Log.d(LOG_TAG, "Pref changed: " + pref);
                this.onPrefMessage(pref, message);
                this.onEnvironmentChanged();
                return;
            }

            
            if (EVENT_KEYWORD_SEARCH.equals(event)) {
                
                
                
                recordSearch(message.getString("identifier"), "bartext");
                return;
            }
            if (EVENT_SEARCH.equals(event)) {
                if (!message.has("location")) {
                    Log.d(LOG_TAG, "Ignoring search without location.");
                    return;
                }
                recordSearch(message.optString("identifier", null), message.getString("location"));
                return;
            }
        } catch (Exception e) {
            Log.e(LOG_TAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    



    public static final String MEASUREMENT_NAME_SEARCH_COUNTS = "org.mozilla.searches.counts";
    public static final int MEASUREMENT_VERSION_SEARCH_COUNTS = 5;

    public static final Set<String> SEARCH_LOCATIONS = Collections.unmodifiableSet(new HashSet<String>(Arrays.asList(new String[] {
        "barkeyword",
        "barsuggest",
        "bartext",
    })));

    void initializeSearchProvider() {
        this.storage.ensureMeasurementInitialized(
            MEASUREMENT_NAME_SEARCH_COUNTS,
            MEASUREMENT_VERSION_SEARCH_COUNTS,
            new MeasurementFields() {
                @Override
                public Iterable<FieldSpec> getFields() {
                    ArrayList<FieldSpec> out = new ArrayList<FieldSpec>(SEARCH_LOCATIONS.size());
                    for (String location : SEARCH_LOCATIONS) {
                        
                        
                        
                        
                        
                        out.add(new FieldSpec(location, Field.TYPE_COUNTED_STRING_DISCRETE));
                    }
                    return out;
                }
        });

        
        
        
        this.dispatcher.registerGeckoThreadListener(this,
            EVENT_KEYWORD_SEARCH,
            EVENT_SEARCH);
    }

    





    public void recordSearch(final String engineID, final String location) {
        if (this.state != State.INITIALIZED) {
            Log.d(LOG_TAG, "Not initialized: not recording search. (" + this.state + ")");
            return;
        }

        final int env = this.env;

        if (env == -1) {
            Log.d(LOG_TAG, "No environment: not recording search.");
            return;
        }

        if (location == null) {
            throw new IllegalArgumentException("location must be provided for search.");
        }

        
        
        
        if (!SEARCH_LOCATIONS.contains(location)) {
            throw new IllegalArgumentException("Unexpected location: " + location);
        }

        final int day = storage.getDay();
        final String key = (engineID == null) ? "other" : engineID;

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final HealthReportDatabaseStorage storage = BrowserHealthRecorder.this.storage;
                if (storage == null) {
                    Log.d(LOG_TAG, "No storage: not recording search. Shutting down?");
                    return;
                }

                Log.d(LOG_TAG, "Recording search: " + key + ", " + location +
                               " (" + day + ", " + env + ").");
                final int searchField = storage.getField(MEASUREMENT_NAME_SEARCH_COUNTS,
                                                         MEASUREMENT_VERSION_SEARCH_COUNTS,
                                                         location)
                                               .getID();
                storage.recordDailyDiscrete(env, day, searchField, key);
            }
        });
    }

    


















































    public static final String MEASUREMENT_NAME_SESSIONS = "org.mozilla.appSessions";
    public static final int MEASUREMENT_VERSION_SESSIONS = 4;

    void initializeSessionsProvider() {
        this.storage.ensureMeasurementInitialized(
            MEASUREMENT_NAME_SESSIONS,
            MEASUREMENT_VERSION_SESSIONS,
            new MeasurementFields() {
                @Override
                public Iterable<FieldSpec> getFields() {
                    List<FieldSpec> out = Arrays.asList(
                        new FieldSpec("normal", Field.TYPE_JSON_DISCRETE),
                        new FieldSpec("abnormal", Field.TYPE_JSON_DISCRETE));
                    return out;
                }
        });
    }

    


    private void recordSessionEntry(String field, SessionInformation session, final int environment, JSONObject value) {
        final HealthReportDatabaseStorage storage = this.storage;
        if (storage == null) {
            Log.d(LOG_TAG, "No storage: not recording session entry. Shutting down?");
            return;
        }

        try {
            final int sessionField = storage.getField(MEASUREMENT_NAME_SESSIONS,
                                                      MEASUREMENT_VERSION_SESSIONS,
                                                      field)
                                            .getID();
            final int day = storage.getDay(session.wallStartTime);
            storage.recordDailyDiscrete(environment, day, sessionField, value);
            Log.v(LOG_TAG, "Recorded session entry for env " + environment + ", current is " + env);
        } catch (Exception e) {
            Log.w(LOG_TAG, "Unable to record session completion.", e);
        }
    }

    public void checkForOrphanSessions() {
        if (!this.orphanChecked.compareAndSet(false, true)) {
            Log.w(LOG_TAG, "Attempting to check for orphan sessions more than once.");
            return;
        }

        Log.d(LOG_TAG, "Checking for orphan session.");
        if (this.previousSession == null) {
            return;
        }
        if (this.previousSession.wallStartTime == 0) {
            return;
        }

        if (state != State.INITIALIZED) {
            
            Log.e(LOG_TAG, "Attempted to record bad session end without initialized recorder.");
            return;
        }

        try {
            recordSessionEntry("abnormal", this.previousSession, this.env,
                               this.previousSession.getCrashedJSON());
        } catch (Exception e) {
            Log.w(LOG_TAG, "Unable to generate session JSON.", e);

            
        }
    }

    public void recordSessionEnd(String reason, SharedPreferences.Editor editor) {
        recordSessionEnd(reason, editor, env);
    }

    





    public void recordSessionEnd(String reason, SharedPreferences.Editor editor, final int environment) {
        Log.d(LOG_TAG, "Recording session end: " + reason);
        if (state != State.INITIALIZED) {
            
            Log.e(LOG_TAG, "Attempted to record session end without initialized recorder.");
            return;
        }

        final SessionInformation session = this.session;
        this.session = null;        

        if (session == null) {
            Log.w(LOG_TAG, "Unable to record session end: no session. Already ended?");
            return;
        }

        if (session.wallStartTime <= 0) {
            Log.e(LOG_TAG, "Session start " + session.wallStartTime + " isn't valid! Can't record end.");
            return;
        }

        long realEndTime = android.os.SystemClock.elapsedRealtime();
        try {
            JSONObject json = session.getCompletionJSON(reason, realEndTime);
            recordSessionEntry("normal", session, environment, json);
        } catch (JSONException e) {
            Log.w(LOG_TAG, "Unable to generate session JSON.", e);

            
            
        }

        
        
        session.recordCompletion(editor);
    }
}
