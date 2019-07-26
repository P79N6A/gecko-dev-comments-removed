




package org.mozilla.gecko;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import java.net.URLDecoder;
import java.util.HashMap;

public class ReferrerReceiver
    extends BroadcastReceiver
{
    private static final String LOGTAG = "GeckoReferrerReceiver";

    public static final String ACTION_INSTALL_REFERRER = "com.android.vending.INSTALL_REFERRER";
    public static final String UTM_SOURCE = "mozilla";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (ACTION_INSTALL_REFERRER.equals(intent.getAction())) {
            String referrer = intent.getStringExtra("referrer");
            if (referrer == null)
                return;

            HashMap<String, String> values = new HashMap<String, String>();
            try {
                String referrers[] = referrer.split("&");
                for (String referrerValue : referrers) {
                    String keyValue[] = referrerValue.split("=");
                    values.put(URLDecoder.decode(keyValue[0]), URLDecoder.decode(keyValue[1]));
                }
            } catch (Exception e) {
            }

            String source = values.get("utm_source");
            String campaign = values.get("utm_campaign");

            if (source != null && UTM_SOURCE.equals(source) && campaign != null) {
                try {
                    JSONObject data = new JSONObject();
                    data.put("id", "playstore");
                    data.put("version", campaign);

                    
                    GeckoEvent event = GeckoEvent.createBroadcastEvent("Distribution:Set", data.toString());
                    GeckoAppShell.sendEventToGecko(event);
                } catch (JSONException e) {
                    Log.e(LOGTAG, "Error setting distribution", e);
                }
            }
        }
    }
}
