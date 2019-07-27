




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;

class PrivateDataPreference extends MultiChoicePreference {
    private static final String LOGTAG = "GeckoPrivateDataPreference";
    private static final String PREF_KEY_PREFIX = "private.data.";

    public PrivateDataPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        if (!positiveResult)
            return;

        Telemetry.sendUIEvent(TelemetryContract.Event.SANITIZE, TelemetryContract.Method.DIALOG, "settings");

        CharSequence keys[] = getEntryKeys();
        boolean values[] = getValues();
        JSONObject json = new JSONObject();

        for (int i = 0; i < keys.length; i++) {
            
            
            
            
            
            String key = keys[i].toString().substring(PREF_KEY_PREFIX.length());
            boolean value = values[i];
            try {
                json.put(key, value);
            } catch (JSONException e) {
                Log.e(LOGTAG, "JSON error", e);
            }
        }

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Sanitize:ClearData", json.toString()));
    }
}
