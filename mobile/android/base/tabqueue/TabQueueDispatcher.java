




package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.preferences.GeckoPreferences;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;







public class TabQueueDispatcher extends Locales.LocaleAwareActivity {
    private static final String LOGTAG = "Gecko" + TabQueueDispatcher.class.getSimpleName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        GeckoAppShell.ensureCrashHandling();

        
        
        
        Intent intent = getIntent();
        int flags = intent.getFlags() & ~Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS;
        intent.setFlags(flags);

        ContextUtils.SafeIntent safeIntent = new ContextUtils.SafeIntent(intent);

        
        
        if (!AppConstants.MOZ_ANDROID_TAB_QUEUE || !AppConstants.NIGHTLY_BUILD) {
            loadNormally(safeIntent.getUnsafe());
            return;
        }

        
        final String dataString = safeIntent.getDataString();
        if (TextUtils.isEmpty(dataString)) {
            abortDueToNoURL(dataString);
            return;
        }

        boolean shouldShowOpenInBackgroundToast = GeckoSharedPrefs.forApp(this).getBoolean(GeckoPreferences.PREFS_TAB_QUEUE, false);

        if (shouldShowOpenInBackgroundToast) {
            showToast(safeIntent.getUnsafe());
        } else {
            loadNormally(safeIntent.getUnsafe());
        }
    }

    private void showToast(Intent intent) {
        intent.setClass(getApplicationContext(), TabQueueService.class);
        startService(intent);
        finish();
    }

    


    private void loadNormally(Intent intent) {
        intent.setClassName(getApplicationContext(), AppConstants.MOZ_ANDROID_BROWSER_INTENT_CLASS);
        startActivity(intent);
        finish();
    }

    



    private void abortDueToNoURL(String dataString) {
        
        Log.w(LOGTAG, "Unable to process tab queue insertion. No URL found! - passed data string: " + dataString);
        finish();
    }
}
