




package org.mozilla.gecko.util;

import android.content.SharedPreferences;
import android.os.Build;
import android.util.Log;

import java.util.HashSet;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONException;


public class PrefUtils {
    private static final String LOGTAG = "GeckoPrefUtils";

    
    public static Set<String> getStringSet(final SharedPreferences prefs,
                                           final String key,
                                           final Set<String> defaultVal) {
        if (!prefs.contains(key)) {
            return defaultVal;
        }

        if (Build.VERSION.SDK_INT < 11) {
            return getFromJSON(prefs, key);
        }

        
        try {
            return prefs.getStringSet(key, new HashSet<String>());
        } catch(ClassCastException ex) {
            
            final Set<String> val = getFromJSON(prefs, key);
            SharedPreferences.Editor edit = prefs.edit();
            putStringSet(edit, key, val).apply();
            return val;
        }
    }

    private static Set<String> getFromJSON(SharedPreferences prefs, String key) {
        try {
            final String val = prefs.getString(key, "[]");
            return JSONUtils.parseStringSet(new JSONArray(val));
        } catch(JSONException ex) {
            Log.i(LOGTAG, "Unable to parse JSON", ex);
        }

        return new HashSet<String>();
    }

    
    
    
    public static SharedPreferences.Editor putStringSet(final SharedPreferences.Editor edit,
                                    final String key,
                                    final Set<String> vals) {
        if (Build.VERSION.SDK_INT < 11) {
            final JSONArray json = new JSONArray(vals);
            edit.putString(key, json.toString()).apply();
        } else {
            edit.putStringSet(key, vals).apply();
        }

        return edit;
    }
}
