



package org.mozilla.gecko.util;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Bundle;
import android.util.Log;

import java.util.HashSet;
import java.util.Set;
import java.util.UUID;

public final class JSONUtils {
    private static final String LOGTAG = "GeckoJSONUtils";

    private JSONUtils() {}

    public static UUID getUUID(String name, JSONObject json) {
        String uuid = json.optString(name, null);
        return (uuid != null) ? UUID.fromString(uuid) : null;
    }

    public static void putUUID(String name, UUID uuid, JSONObject json) {
        String uuidString = uuid.toString();
        try {
            json.put(name, uuidString);
        } catch (JSONException e) {
            throw new IllegalArgumentException(name + "=" + uuidString, e);
        }
    }

    public static JSONObject bundleToJSON(Bundle bundle) {
        if (bundle == null || bundle.isEmpty()) {
            return null;
        }

        JSONObject json = new JSONObject();
        for (String key : bundle.keySet()) {
            try {
                json.put(key, bundle.get(key));
            } catch (JSONException e) {
                Log.w(LOGTAG, "Error building JSON response.", e);
            }
        }

        return json;
    }

    
    public static Set<String> parseStringSet(JSONArray json) {
        final Set<String> ret = new HashSet<String>();

        for (int i = 0; i < json.length(); i++) {
            try {
                ret.add(json.getString(i));
            } catch(JSONException ex) {
                Log.i(LOGTAG, "Error parsing json", ex);
            }
        }

        return ret;
    }

}
