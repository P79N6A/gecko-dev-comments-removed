




package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.Locales;
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

        Intent intent = getIntent();

        
        
        if (!AppConstants.MOZ_ANDROID_TAB_QUEUE) {
            loadNormally(intent);
            finish();
        }

        
        final String dataString = intent.getDataString();
        if (TextUtils.isEmpty(dataString)) {
            abortDueToNoURL(dataString);
            return;
        }

        
        final String pageUrl = new WebURLFinder(dataString).bestWebURL();
        if (TextUtils.isEmpty(pageUrl)) {
            abortDueToNoURL(dataString);
            return;
        }

        showToast(intent);
    }

    private void showToast(Intent intent) {
        intent.setClass(getApplicationContext(), TabQueueService.class);
        startService(intent);
        finish();
    }

    


    private void loadNormally(Intent intent) {
        intent.setClass(getApplicationContext(), BrowserApp.class);
        startActivity(intent);
        finish();
    }

    



    private void abortDueToNoURL(String dataString) {
        
        Log.w(LOGTAG, "Unable to process tab queue insertion. No URL found! - passed data string: " + dataString);
        finish();
    }
}
