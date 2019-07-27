




package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.sync.setup.activities.WebURLFinder;

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

        ContextUtils.SafeIntent intent = new ContextUtils.SafeIntent(getIntent());

        
        
        if (!AppConstants.MOZ_ANDROID_TAB_QUEUE || !AppConstants.NIGHTLY_BUILD) {
            loadNormally(intent.getUnsafe());
            return;
        }

        
        final String dataString = intent.getDataString();
        if (TextUtils.isEmpty(dataString)) {
            abortDueToNoURL(dataString);
            return;
        }

        boolean shouldShowOpenInBackgroundToast = GeckoSharedPrefs.forApp(this).getBoolean(GeckoPreferences.PREFS_TAB_QUEUE, false);

        if (shouldShowOpenInBackgroundToast) {
            showToast(intent.getUnsafe());
        } else {
            loadNormally(intent.getUnsafe());
        }
    }

    private void showToast(Intent intent) {
        intent.setClass(getApplicationContext(), TabQueueService.class);
        startService(intent);
        finish();
    }

    


    private void loadNormally(Intent intent) {
        intent.setClassName(getApplicationContext(), AppConstants.BROWSER_INTENT_CLASS_NAME);
        startActivity(intent);
        finish();
    }

    



    private void abortDueToNoURL(String dataString) {
        
        Log.w(LOGTAG, "Unable to process tab queue insertion. No URL found! - passed data string: " + dataString);
        finish();
    }
}
