




package org.mozilla.gecko;

import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

import java.util.Map;
import java.util.HashMap;




public final class SharedPreferencesHelper
             implements GeckoEventListener
{
    public static final String LOGTAG = "GeckoAndSharedPrefs";

    private enum Scope {
        APP("app"),
        PROFILE("profile"),
        GLOBAL("global");

        public final String key;

        private Scope(String key) {
            this.key = key;
        }

        public static Scope forKey(String key) {
            for (Scope scope : values()) {
                if (scope.key.equals(key)) {
                    return scope;
                }
            }

            throw new IllegalStateException("SharedPreferences scope must be valid.");
        }
    }

    protected final Context mContext;

    
    
    protected final Map<String, SharedPreferences.OnSharedPreferenceChangeListener> mListeners;

    public SharedPreferencesHelper(Context context) {
        mContext = context;

        mListeners = new HashMap<String, SharedPreferences.OnSharedPreferenceChangeListener>();

        EventDispatcher dispatcher = EventDispatcher.getInstance();
        if (dispatcher == null) {
            Log.e(LOGTAG, "Gecko event dispatcher must not be null", new RuntimeException());
            return;
        }
        dispatcher.registerGeckoThreadListener(this,
            "SharedPreferences:Set",
            "SharedPreferences:Get",
            "SharedPreferences:Observe");
    }

    public synchronized void uninit() {
        EventDispatcher dispatcher = EventDispatcher.getInstance();
        if (dispatcher == null) {
            Log.e(LOGTAG, "Gecko event dispatcher must not be null", new RuntimeException());
            return;
        }
        dispatcher.unregisterGeckoThreadListener(this,
            "SharedPreferences:Set",
            "SharedPreferences:Get",
            "SharedPreferences:Observe");
    }

    private SharedPreferences getSharedPreferences(JSONObject message) throws JSONException {
        final Scope scope = Scope.forKey(message.getString("scope"));
        switch (scope) {
            case APP:
                return GeckoSharedPrefs.forApp(mContext);
            case PROFILE:
                final String profileName = message.optString("profileName", null);
                if (profileName == null) {
                    return GeckoSharedPrefs.forProfile(mContext);
                } else {
                    return GeckoSharedPrefs.forProfileName(mContext, profileName);
                }
            case GLOBAL:
                final String branch = message.optString("branch", null);
                if (branch == null) {
                    return PreferenceManager.getDefaultSharedPreferences(mContext);
                } else {
                    return mContext.getSharedPreferences(branch, Context.MODE_PRIVATE);
                }
        }

        return null;
    }

    private String getBranch(Scope scope, String profileName, String branch) {
        switch (scope) {
            case APP:
                return GeckoSharedPrefs.APP_PREFS_NAME;
            case PROFILE:
                if (profileName == null) {
                    profileName = GeckoProfile.get(mContext).getName();
                }

                return GeckoSharedPrefs.PROFILE_PREFS_NAME_PREFIX + profileName;
            case GLOBAL:
                return branch;
        }

        return null;
    }

    








    private void handleSet(JSONObject message) throws JSONException {
        SharedPreferences.Editor editor = getSharedPreferences(message).edit();

        JSONArray jsonPrefs = message.getJSONArray("preferences");

        for (int i = 0; i < jsonPrefs.length(); i++) {
            JSONObject pref = jsonPrefs.getJSONObject(i);
            String name = pref.getString("name");
            String type = pref.getString("type");
            if ("bool".equals(type)) {
                editor.putBoolean(name, pref.getBoolean("value"));
            } else if ("int".equals(type)) {
                editor.putInt(name, pref.getInt("value"));
            } else if ("string".equals(type)) {
                editor.putString(name, pref.getString("value"));
            } else {
                Log.w(LOGTAG, "Unknown pref value type [" + type + "] for pref [" + name + "]");
            }
            editor.apply();
        }
    }

    








    private JSONArray handleGet(JSONObject message) throws JSONException {
        SharedPreferences prefs = getSharedPreferences(message);
        JSONArray jsonPrefs = message.getJSONArray("preferences");
        JSONArray jsonValues = new JSONArray();

        for (int i = 0; i < jsonPrefs.length(); i++) {
            JSONObject pref = jsonPrefs.getJSONObject(i);
            String name = pref.getString("name");
            String type = pref.getString("type");
            JSONObject jsonValue = new JSONObject();
            jsonValue.put("name", name);
            jsonValue.put("type", type);
            try {
                if ("bool".equals(type)) {
                    boolean value = prefs.getBoolean(name, false);
                    jsonValue.put("value", value);
                } else if ("int".equals(type)) {
                    int value = prefs.getInt(name, 0);
                    jsonValue.put("value", value);
                } else if ("string".equals(type)) {
                    String value = prefs.getString(name, "");
                    jsonValue.put("value", value);
                } else {
                    Log.w(LOGTAG, "Unknown pref value type [" + type + "] for pref [" + name + "]");
                }
            } catch (ClassCastException e) {
                
                
                Log.w(LOGTAG, "Wrong pref value type [" + type + "] for pref [" + name + "]");
            }
            jsonValues.put(jsonValue);
        }

        return jsonValues;
    }

    private static class ChangeListener
        implements SharedPreferences.OnSharedPreferenceChangeListener {
        public final Scope scope;
        public final String branch;
        public final String profileName;

        public ChangeListener(final Scope scope, final String branch, final String profileName) {
            this.scope = scope;
            this.branch = branch;
            this.profileName = profileName;
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if (Log.isLoggable(LOGTAG, Log.VERBOSE)) {
                Log.v(LOGTAG, "Got onSharedPreferenceChanged");
            }
            try {
                final JSONObject msg = new JSONObject();
                msg.put("scope", this.scope.key);
                msg.put("branch", this.branch);
                msg.put("profileName", this.profileName);
                msg.put("key", key);

                
                
                
                msg.put("value", sharedPreferences.getAll().get(key));

                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SharedPreferences:Changed", msg.toString()));
            } catch (JSONException e) {
                Log.e(LOGTAG, "Got exception creating JSON object", e);
                return;
            }
        }
    }

    







    private void handleObserve(JSONObject message) throws JSONException {
        final SharedPreferences prefs = getSharedPreferences(message);
        final boolean enable = message.getBoolean("enable");

        final Scope scope = Scope.forKey(message.getString("scope"));
        final String profileName = message.optString("profileName", null);
        final String branch = getBranch(scope, profileName, message.optString("branch", null));

        if (branch == null) {
            Log.e(LOGTAG, "No branch specified for SharedPreference:Observe; aborting.");
            return;
        }

        
        
        if (enable && !this.mListeners.containsKey(branch)) {
            SharedPreferences.OnSharedPreferenceChangeListener listener
                = new ChangeListener(scope, branch, profileName);
            this.mListeners.put(branch, listener);
            prefs.registerOnSharedPreferenceChangeListener(listener);
        }
        if (!enable && this.mListeners.containsKey(branch)) {
            SharedPreferences.OnSharedPreferenceChangeListener listener
                = this.mListeners.remove(branch);
            prefs.unregisterOnSharedPreferenceChangeListener(listener);
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        
        
        try {
            if (event.equals("SharedPreferences:Set")) {
                if (Log.isLoggable(LOGTAG, Log.VERBOSE)) {
                    Log.v(LOGTAG, "Got SharedPreferences:Set message.");
                }
                handleSet(message);
            } else if (event.equals("SharedPreferences:Get")) {
                if (Log.isLoggable(LOGTAG, Log.VERBOSE)) {
                    Log.v(LOGTAG, "Got SharedPreferences:Get message.");
                }
                JSONObject obj = new JSONObject();
                obj.put("values", handleGet(message));
                EventDispatcher.sendResponse(message, obj);
            } else if (event.equals("SharedPreferences:Observe")) {
                if (Log.isLoggable(LOGTAG, Log.VERBOSE)) {
                    Log.v(LOGTAG, "Got SharedPreferences:Observe message.");
                }
                handleObserve(message);
            } else {
                Log.e(LOGTAG, "SharedPreferencesHelper got unexpected message " + event);
                return;
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Got exception in handleMessage handling event " + event, e);
            return;
        }
    }
}
