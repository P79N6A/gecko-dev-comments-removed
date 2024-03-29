



package org.mozilla.mozstumbler.service.mainthread;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.mozilla.mozstumbler.service.AppGlobals;
import org.mozilla.mozstumbler.service.stumblerthread.StumblerService;

















public class PassiveServiceReceiver extends BroadcastReceiver {
    
    
    static final String LOG_TAG = "PassiveStumbler";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null) {
            return;
        }

        
        
        
        AppGlobals.isDebug = Log.isLoggable(LOG_TAG, Log.DEBUG);

        final String action = intent.getAction();
        final boolean isIntentFromHostApp = (action != null) && action.contains(".STUMBLER_PREF");
        if (!isIntentFromHostApp) {
            Log.d(LOG_TAG, "Stumbler: received intent external to host app");
            Intent startServiceIntent = new Intent(context, StumblerService.class);
            startServiceIntent.putExtra(StumblerService.ACTION_NOT_FROM_HOST_APP, true);
            context.startService(startServiceIntent);
            return;
        }

        StumblerService.sFirefoxStumblingEnabled.set(intent.getBooleanExtra("enabled", false));

        if (!StumblerService.sFirefoxStumblingEnabled.get()) {
            
            context.stopService(new Intent(context, StumblerService.class));
            
            context.sendBroadcast(new Intent(AppGlobals.ACTION_TEST_SETTING_DISABLED));
            return;
        }

        
        context.sendBroadcast(new Intent(AppGlobals.ACTION_TEST_SETTING_ENABLED));

        Log.d(LOG_TAG, "Stumbler: Sending passive start message | isDebug:" + AppGlobals.isDebug);

        final Intent startServiceIntent = new Intent(context, StumblerService.class);
        startServiceIntent.putExtra(StumblerService.ACTION_START_PASSIVE, true);
        final String mozApiKey = intent.getStringExtra("moz_mozilla_api_key");
        startServiceIntent.putExtra(StumblerService.ACTION_EXTRA_MOZ_API_KEY, mozApiKey);
        final String userAgent = intent.getStringExtra("user_agent");
        startServiceIntent.putExtra(StumblerService.ACTION_EXTRA_USER_AGENT, userAgent);
        context.startService(startServiceIntent);
    }
}

