




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.GfxInfoThread;

import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.SystemClock;
import android.util.Log;

import java.util.Locale;

public class GeckoThread extends Thread {
    private static final String LOGTAG = "GeckoThread";

    Intent mIntent;
    String mUri;
    int mRestoreMode;

    GeckoThread(Intent intent, String uri, int restoreMode) {
        mIntent = intent;
        mUri = uri;
        mRestoreMode = restoreMode;
        setName("Gecko");
    }

    public void run() {

        
        
        
        
        
        
        GeckoAppShell.sGfxInfoThread = new GfxInfoThread();
        GeckoAppShell.sGfxInfoThread.start();

        final GeckoApp app = GeckoApp.mAppContext;

        
        
        Locale locale = Locale.getDefault();

        String resourcePath = app.getApplication().getPackageResourcePath();
        GeckoAppShell.setupGeckoEnvironment(app);
        GeckoAppShell.loadSQLiteLibs(app, resourcePath);
        GeckoAppShell.loadNSSLibs(app, resourcePath);
        GeckoAppShell.loadGeckoLibs(resourcePath);

        Locale.setDefault(locale);
        Resources res = app.getBaseContext().getResources();
        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - runGecko");

        
        final String action = mIntent.getAction();
        String type = null;

        if (action != null && action.startsWith(GeckoApp.ACTION_WEBAPP_PREFIX))
            type = "-webapp";
        else if (GeckoApp.ACTION_BOOKMARK.equals(action))
            type = "-bookmark";

        String args = mIntent.getStringExtra("args");

        
        if (!(app instanceof BrowserApp)) {
            String profile = app.getDefaultProfileName();
            args = (args != null ? args : "") + "-P " + profile;
        }

        
        Log.i(LOGTAG, "RunGecko - URI = " + mUri + " args = " + args);
        GeckoAppShell.runGecko(app.getApplication().getPackageResourcePath(),
                               args,
                               mUri,
                               type,
                               mRestoreMode);
    }
}
