




































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
    boolean mRestoreSession;

    GeckoThread (Intent intent, String uri, boolean restoreSession) {
        mIntent = intent;
        mUri = uri;
        mRestoreSession = restoreSession;
    }

    public void run() {
        final GeckoApp app = GeckoApp.mAppContext;
        File cacheFile = GeckoAppShell.getCacheDir(app);
        File libxulFile = new File(cacheFile, "libxul.so");

        if ((!libxulFile.exists() ||
             new File(app.getApplication().getPackageResourcePath()).lastModified() >= libxulFile.lastModified())) {
            File[] libs = cacheFile.listFiles(new FilenameFilter() {
                public boolean accept(File dir, String name) {
                    return name.endsWith(".so");
                }
            });
            if (libs != null) {
                for (int i = 0; i < libs.length; i++) {
                    libs[i].delete();
                }
            }
        }

        
        
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

        
        Log.w(LOGTAG, "RunGecko - URI = " + mUri);
        GeckoAppShell.runGecko(app.getApplication().getPackageResourcePath(),
                               mIntent.getStringExtra("args"),
                               mUri,
                               mRestoreSession);
    }
}
