




package org.mozilla.gecko.distribution;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.mozglue.RobocopTarget;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;
import android.text.TextUtils;
import android.util.Log;

public class ReferrerReceiver extends BroadcastReceiver {
    private static final String LOGTAG = "GeckoReferrerReceiver";

    private static final String ACTION_INSTALL_REFERRER = "com.android.vending.INSTALL_REFERRER";

    
    @RobocopTarget
    public static final String ACTION_REFERRER_RECEIVED = "org.mozilla.fennec.REFERRER_RECEIVED";

    


    private static final String MOZILLA_UTM_SOURCE = "mozilla";

    


    private static final String DISTRIBUTION_UTM_CAMPAIGN = "distribution";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.v(LOGTAG, "Received intent " + intent);
        if (!ACTION_INSTALL_REFERRER.equals(intent.getAction())) {
            
            return;
        }

        
        ReferrerDescriptor referrer = new ReferrerDescriptor(intent.getStringExtra("referrer"));

        if (!TextUtils.equals(referrer.source, MOZILLA_UTM_SOURCE)) {
            if (AppConstants.MOZ_INSTALL_TRACKING) {
                
                try {
                    AppConstants.getAdjustHelper().onReceive(context, intent);
                } catch (Exception e) {
                    Log.e(LOGTAG, "Got exception in Adjust's onReceive; ignoring referrer intent.", e);
                }
            }
            return;
        }

        if (TextUtils.equals(referrer.campaign, DISTRIBUTION_UTM_CAMPAIGN)) {
            Distribution.onReceivedReferrer(context, referrer);
        } else {
            Log.d(LOGTAG, "Not downloading distribution: non-matching campaign.");
            
            
            propagateMozillaCampaign(referrer);
        }

        
        final Intent receivedIntent = new Intent(ACTION_REFERRER_RECEIVED);
        LocalBroadcastManager.getInstance(context).sendBroadcast(receivedIntent);
    }


    private void propagateMozillaCampaign(ReferrerDescriptor referrer) {
        if (referrer.campaign == null) {
            return;
        }

        try {
            final JSONObject data = new JSONObject();
            data.put("id", "playstore");
            data.put("version", referrer.campaign);
            String payload = data.toString();

            
            final GeckoEvent event = GeckoEvent.createBroadcastEvent("Campaign:Set", payload);
            GeckoAppShell.sendEventToGecko(event);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error propagating campaign identifier.", e);
        }
    }
}
