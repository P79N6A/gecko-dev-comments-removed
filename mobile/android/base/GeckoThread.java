




package org.mozilla.gecko;

import android.content.Intent;
import android.content.res.Resources;
import android.content.res.Configuration;
import android.os.SystemClock;
import android.util.Log;
import android.widget.AbsoluteLayout;

import java.io.File;
import java.io.FilenameFilter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Date;
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
        String type = GeckoApp.ACTION_WEBAPP.equals(action) ? "-webapp" :
                      GeckoApp.ACTION_BOOKMARK.equals(action) ? "-bookmark" :
                      null;

        
        Log.i(LOGTAG, "RunGecko - URI = " + mUri);
        GeckoAppShell.runGecko(app.getApplication().getPackageResourcePath(),
                               mIntent.getStringExtra("args"),
                               mUri,
                               type,
                               mRestoreMode);
    }
}
