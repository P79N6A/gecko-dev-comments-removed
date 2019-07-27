




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
            Log.i(LOGTAG, e.toString());
        }
        try {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
                                AppConstants.BROWSER_INTENT_CLASS_NAME);
            Bundle b = getIntent().getExtras();
            if (b != null)
                intent.putExtras(b);
            Log.i(LOGTAG, intent.toString());
            startActivity(intent);
        } catch (Exception e) {
            Log.i(LOGTAG, e.toString());
        }
    }
}
