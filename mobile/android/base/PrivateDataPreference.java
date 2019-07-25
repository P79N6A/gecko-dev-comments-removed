




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;

import org.json.JSONObject;
import org.json.JSONException;

import java.util.Map;

import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.util.AttributeSet;
import android.util.Log;

class PrivateDataPreference extends MultiChoicePreference {
    private static final String LOGTAG = "GeckoPrivateDataPreference";
    private static final String PREF_KEY_PREFIX = "private.data.";

    private Context mContext;

    public PrivateDataPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        if (!positiveResult)
            return;

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

            
            if (key.equals("history") && value) {
                GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        BrowserDB.clearHistory(mContext.getContentResolver());
                        GeckoApp.mAppContext.mFavicons.clearFavicons();
                        GeckoApp.mAppContext.handleClearHistory();
                    }
                });
            }
        }

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Sanitize:ClearData", json.toString()));
    }
}
