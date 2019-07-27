




package org.mozilla.gecko;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class Restarter extends Activity {
    private static final String LOGTAG = "GeckoRestarter";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.i(LOGTAG, "Trying to restart " + AppConstants.MOZ_APP_NAME);
        try {
            int countdown = 40;
            while (GeckoAppShell.checkForGeckoProcs() &&  --countdown > 0) {
                
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ie) {}
            }

            if (countdown <= 0) {
                
                GeckoAppShell.killAnyZombies();
                countdown = 10;
                
                while (GeckoAppShell.checkForGeckoProcs() &&  --countdown > 0) {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException ie) {}
                }
            }
        } catch (Exception e) {
            Log.i(LOGTAG, "Error killing gecko", e);
        }

        try {
            final Intent originalIntent = getIntent();
            Intent intent = null;
            if (originalIntent.hasExtra(Intent.EXTRA_INTENT)) {
                intent = (Intent) originalIntent.getParcelableExtra(Intent.EXTRA_INTENT);
                originalIntent.removeExtra(Intent.EXTRA_INTENT);
            }

            if (intent == null) {
                intent = new Intent(Intent.ACTION_MAIN);
            }

            intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                                AppConstants.BROWSER_INTENT_CLASS_NAME);

            Bundle b = originalIntent.getExtras();
            if (b != null) {
                intent.putExtras(b);
            }

            Log.i(LOGTAG, intent.toString());
            startActivity(intent);
        } catch (Exception e) {
            Log.i(LOGTAG, "Error restarting", e);
        }
    }
}
