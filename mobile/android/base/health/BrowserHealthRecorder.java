




package org.mozilla.gecko.health;

import java.util.ArrayList;

import android.content.Context;
import android.content.ContentProviderClient;
import android.util.Log;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.PrefsHelper.PrefHandler;

import org.mozilla.gecko.background.healthreport.EnvironmentBuilder;
import org.mozilla.gecko.background.healthreport.HealthReportDatabaseStorage;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.Field;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.MeasurementFields;
import org.mozilla.gecko.background.healthreport.HealthReportStorage.MeasurementFields.FieldSpec;
import org.mozilla.gecko.background.healthreport.ProfileInformationCache;

import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.util.Scanner;


















public class BrowserHealthRecorder implements GeckoEventListener {
    private static final String LOG_TAG = "GeckoHealthRec";
    private static final String PREF_BLOCKLIST_ENABLED = "extensions.blocklist.enabled";
    private static final String EVENT_ADDONS_ALL = "Addons:All";
    private static final String EVENT_ADDONS_CHANGE = "Addons:Change";
    private static final String EVENT_PREF_CHANGE = "Pref:Change";

    public enum State {
        NOT_INITIALIZED,
        INITIALIZING,
        INITIALIZED,
        INITIALIZATION_FAILED,
        CLOSED
    }

    protected volatile State state = State.NOT_INITIALIZED;

    private volatile int env = -1;
    private volatile HealthReportDatabaseStorage storage;
    private final ProfileInformationCache profileCache;
    private ContentProviderClient client;
    private final EventDispatcher dispatcher;

    



    private void setHealthEnvironment(final int env) {
        this.env = env;
    }

    


    public BrowserHealthRecorder(final Context context, final String profilePath, final EventDispatcher dispatcher) {
        Log.d(LOG_TAG, "Initializing. Dispatcher is " + dispatcher);
        this.dispatcher = dispatcher;
        this.client = EnvironmentBuilder.getContentProviderClient(context);
        if (this.client == null) {
            throw new IllegalStateException("Could not fetch Health Report content provider.");
        }

        this.storage = EnvironmentBuilder.getStorage(this.client, profilePath);
        if (this.storage == null) {
            throw new IllegalStateException("No storage in health recorder!");
        }

        this.profileCache = new ProfileInformationCache(profilePath);
        try {
            this.initialize(context, profilePath);
        } catch (Exception e) {
            Log.e(LOG_TAG, "Exception initializing.", e);
        }

        
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
        this.dispatcher.unregisterEventListener(EVENT_ADDONS_ALL, this);
        this.dispatcher.unregisterEventListener(EVENT_ADDONS_CHANGE, this);
        this.dispatcher.unregisterEventListener(EVENT_PREF_CHANGE, this);
    }

    public void onBlocklistPrefChanged(boolean to) {
        this.profileCache.beginInitialization();
        this.profileCache.setBlocklistEnabled(to);
    }

    public void onTelemetryPrefChanged(boolean to) {
        this.profileCache.beginInitialization();
        this.profileCache.setTelemetryEnabled(to);
    }

    public void onAddonChanged(String id, JSONObject json) {
        this.profileCache.beginInitialization();
        try {
            this.profileCache.updateJSONForAddon(id, json);
        } catch (IllegalStateException e) {
            Log.w(LOG_TAG, "Attempted to update add-on cache prior to full init.", e);
        }
    }

    









    public synchronized void onEnvironmentChanged() {
        this.env = -1;
        try {
            profileCache.completeInitialization();
        } catch (java.io.IOException e) {
            Log.e(LOG_TAG, "Error completing profile cache initialization.", e);
            this.state = State.INITIALIZATION_FAILED;
            return;
        }
        ensureEnvironment();
    }

    protected synchronized int ensureEnvironment() {
        if (!(state == State.INITIALIZING ||
              state == State.INITIALIZED)) {
            throw new IllegalStateException("Not initialized.");
        }

        if (this.env != -1) {
            return this.env;
        }
        return this.env = EnvironmentBuilder.registerCurrentEnvironment(this.storage,
                                                                        this.profileCache);
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
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.GINGERBREAD) {
            return -1;
        }

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

    private void handlePrefValue(final String pref, final boolean value) {
        Log.d(LOG_TAG, "Incorporating environment: " + pref + " = " + value);
        if (AppConstants.TELEMETRY_PREF_NAME.equals(pref)) {
            profileCache.setTelemetryEnabled(value);
            return;
        }
        if (PREF_BLOCKLIST_ENABLED.equals(pref)) {
            profileCache.setBlocklistEnabled(value);
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
                        
                        dispatcher.registerEventListener(EVENT_ADDONS_CHANGE, self);
                        dispatcher.registerEventListener(EVENT_PREF_CHANGE, self);

                        

                        Log.d(LOG_TAG, "Ensuring environment.");
                        ensureEnvironment();

                        Log.d(LOG_TAG, "Finishing init.");
                        storage.finishInitialization();
                        state = State.INITIALIZED;
                    } catch (Exception e) {
                        state = State.INITIALIZATION_FAILED;
                        storage.abortInitialization();
                        Log.e(LOG_TAG, "Initialization failed.", e);
                    }
                }
            }
        });
    }

    


    private synchronized void initialize(final Context context,
                                         final String profilePath)
        throws java.io.IOException {

        Log.d(LOG_TAG, "Initializing profile cache.");
        this.state = State.INITIALIZING;

        
        if (this.profileCache.restoreUnlessInitialized()) {
            Log.i(LOG_TAG, "Successfully restored state. Initializing storage.");
            initializeStorage();
            return;
        }

        
        this.profileCache.beginInitialization();
        this.profileCache.setProfileCreationTime(getAndPersistProfileInitTime(context, profilePath));

        final BrowserHealthRecorder self = this;

        PrefHandler handler = new PrefsHelper.PrefHandlerBase() {
            @Override
            public void prefValue(String pref, boolean value) {
                handlePrefValue(pref, value);
            }

            @Override
            public void finish() {
                Log.d(LOG_TAG, "Requesting all add-ons from Gecko.");
                dispatcher.registerEventListener(EVENT_ADDONS_ALL, self);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Addons:FetchAll", null));
                
            }
        };

        
        PrefsHelper.getPrefs(new String[] {
                                 AppConstants.TELEMETRY_PREF_NAME,
                                 PREF_BLOCKLIST_ENABLED
                             },
                             handler);
        Log.d(LOG_TAG, "Done initializing profile cache. Beginning storage init.");
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (EVENT_ADDONS_ALL.equals(event)) {
                Log.d(LOG_TAG, "Got all add-ons.");
                try {
                    JSONObject addons = message.getJSONObject("json");
                    Log.d(LOG_TAG, "Persisting " + addons.length() + " add-ons.");
                    profileCache.setJSONForAddons(addons);
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
            if (EVENT_ADDONS_CHANGE.equals(event)) {
                Log.d(LOG_TAG, "Add-on changed: " + message.getString("id"));
                this.onAddonChanged(message.getString("id"), message.getJSONObject("json"));
                this.onEnvironmentChanged();
                return;
            }
            if (EVENT_PREF_CHANGE.equals(event)) {
                final String pref = message.getString("pref");
                Log.d(LOG_TAG, "Pref changed: " + pref);
                handlePrefValue(pref, message.getBoolean("value"));
                this.onEnvironmentChanged();
                return;
            }
        } catch (Exception e) {
            Log.e(LOG_TAG, "Exception handling message \"" + event + "\":", e);
        }
    }
}

