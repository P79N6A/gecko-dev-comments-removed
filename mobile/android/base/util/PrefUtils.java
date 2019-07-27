




package org.mozilla.gecko.util;

import java.util.HashSet;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONException;
import org.mozilla.gecko.AppConstants.Versions;

import android.content.SharedPreferences;
import android.util.Log;


public class PrefUtils {
    private static final String LOGTAG = "GeckoPrefUtils";

    
    public static Set<String> getStringSet(final SharedPreferences prefs,
                                           final String key,
                                           final Set<String> defaultVal) {
        if (!prefs.contains(key)) {
            return defaultVal;
        }

        if (Versions.preHC) {
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

    










    public static SharedPreferences.Editor putStringSet(final SharedPreferences.Editor editor,
                                    final String key,
                                    final Set<String> vals) {
        if (Versions.preHC) {
            final JSONArray json = new JSONArray(vals);
            editor.putString(key, json.toString());
        } else {
            editor.putStringSet(key, vals);
        }

        return editor;
    }
}
