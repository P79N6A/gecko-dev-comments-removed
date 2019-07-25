




package org.mozilla.gecko;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class Telemetry {
    private static final String LOGTAG = "Telemetry";

    
    
    public static void HistogramAdd(String name,
                                    int value) {
        try {
            JSONObject jsonData = new JSONObject();

            jsonData.put("name", name);
            jsonData.put("value", value);

            GeckoEvent event =
                GeckoEvent.createBroadcastEvent("Telemetry:Add", jsonData.toString());
            GeckoAppShell.sendEventToGecko(event);

            Log.v(LOGTAG, "Sending telemetry: " + jsonData.toString());
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSON exception: ", e);
        }
    }
}
