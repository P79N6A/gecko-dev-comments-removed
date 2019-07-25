




































package org.mozilla.gecko;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.content.res.Configuration;
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

    GeckoThread (Intent intent) {
        mIntent = intent;
    }

    public void run() {
        final GeckoApp app = GeckoApp.mAppContext;
        Intent intent = mIntent;
        File cacheFile = GeckoAppShell.getCacheDir();
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
        GeckoAppShell.loadGeckoLibs(
            app.getApplication().getPackageResourcePath());
        Locale.setDefault(locale);
        Resources res = app.getBaseContext().getResources();
        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());

        Log.w(LOGTAG, "zerdatime " + new Date().getTime() + " - runGecko");

        
        try {
            String uri = intent.getDataString();
            String title = uri;
            if (!app.mUserDefinedProfile &&
                (uri == null || uri.length() == 0)) {
                SharedPreferences prefs = app.getSharedPreferences("GeckoApp", app.MODE_PRIVATE);
                uri = prefs.getString("last-uri", "");
                title = prefs.getString("last-title", uri);
            }

            final String awesomeTitle = title;
            app.mMainHandler.post(new Runnable() {
                public void run() {
                    app.mBrowserToolbar.setTitle(awesomeTitle);
                }
            });

            Log.w(LOGTAG, "RunGecko - URI = " + uri);

            GeckoAppShell.runGecko(app.getApplication().getPackageResourcePath(),
                                   intent.getStringExtra("args"),
                                   uri);
        } catch (Exception e) {
            Log.e(LOGTAG, "top level exception", e);
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            e.printStackTrace(pw);
            pw.flush();
            GeckoAppShell.reportJavaCrash(sw.toString());
        }
    }
}
