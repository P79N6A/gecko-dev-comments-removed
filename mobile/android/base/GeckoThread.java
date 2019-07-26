




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.GfxInfoThread;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.SystemClock;
import android.util.Log;

import java.util.Locale;

public class GeckoThread extends Thread implements GeckoEventListener {
    private static final String LOGTAG = "GeckoThread";

    public enum LaunchState {
        Launching,
        WaitForDebugger,
        Launched,
        GeckoRunning,
        GeckoExiting
    };

    private static LaunchState sLaunchState = LaunchState.Launching;

    private final Intent mIntent;
    private final String mUri;

    GeckoThread(Intent intent, String uri) {
        mIntent = intent;
        mUri = uri;
        setName("Gecko");
        GeckoAppShell.getEventDispatcher().registerEventListener("Gecko:Ready", this);
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

        String profile = GeckoApp.sIsUsingCustomProfile ? "" : (" -P " + app.getProfile().getName());
        args = (args != null ? args : "") + profile;

        
        Log.i(LOGTAG, "RunGecko - args = " + args);
        GeckoAppShell.runGecko(app.getApplication().getPackageResourcePath(),
                               args,
                               mUri,
                               type);
    }

    public void handleMessage(String event, JSONObject message) {
        if ("Gecko:Ready".equals(event)) {
            GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
            setLaunchState(LaunchState.GeckoRunning);
            GeckoAppShell.sendPendingEventsToGecko();
        }
    }

    public static boolean checkLaunchState(LaunchState checkState) {
        synchronized (sLaunchState) {
            return sLaunchState == checkState;
        }
    }

    static void setLaunchState(LaunchState setState) {
        synchronized (sLaunchState) {
            sLaunchState = setState;
        }
    }

    



    static boolean checkAndSetLaunchState(LaunchState checkState, LaunchState setState) {
        synchronized (sLaunchState) {
            if (sLaunchState != checkState)
                return false;
            sLaunchState = setState;
            return true;
        }
    }
}
