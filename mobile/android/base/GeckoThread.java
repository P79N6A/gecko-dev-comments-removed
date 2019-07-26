




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.content.Intent;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.app.Activity;
import java.io.IOException;


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

    private Intent mIntent;
    private final String mUri;

    GeckoThread(Intent intent, String uri) {
        mIntent = intent;
        mUri = uri;
        setName("Gecko");
        GeckoAppShell.getEventDispatcher().registerEventListener("Gecko:Ready", this);
    }

    private String initGeckoEnvironment() {
        
        
        Locale locale = Locale.getDefault();

        if (locale.toString().equalsIgnoreCase("zh_hk")) {
            locale = Locale.TRADITIONAL_CHINESE;
            Locale.setDefault(locale);
        }

        Context app = GeckoAppShell.getContext();
        String resourcePath = "";
        Resources res  = null;
        String[] pluginDirs = null;
        try {
            pluginDirs = GeckoAppShell.getPluginDirectories();
        } catch (Exception e) {
            Log.w(LOGTAG, "Caught exception getting plugin dirs.", e);
        }
        
        if (app instanceof Activity) {
            Activity activity = (Activity)app;
            resourcePath = activity.getApplication().getPackageResourcePath();
            res = activity.getBaseContext().getResources();
            GeckoLoader.setupGeckoEnvironment(activity, pluginDirs, GeckoProfile.get(app).getFilesDir().getPath());
        }
        GeckoLoader.loadSQLiteLibs(app, resourcePath);
        GeckoLoader.loadNSSLibs(app, resourcePath);
        GeckoLoader.loadGeckoLibs(app, resourcePath);
        GeckoJavaSampler.setLibsLoaded();

        Locale.setDefault(locale);

        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());

        return resourcePath;
    }

    private String getTypeFromAction(String action) {
        if (action != null && action.startsWith(GeckoApp.ACTION_WEBAPP_PREFIX)) {
            return "-webapp";
        }
        if (GeckoApp.ACTION_BOOKMARK.equals(action)) {
            return "-bookmark";
        }
        return null;
    }

    private String addCustomProfileArg(String args) {
        String profile = "";
        if (GeckoAppShell.getGeckoInterface() != null) {
            if (GeckoAppShell.getGeckoInterface().getProfile().inGuestMode()) {
                try {
                    profile = " -profile " + GeckoAppShell.getGeckoInterface().getProfile().getDir().getCanonicalPath();
                } catch (IOException ioe) { Log.e(LOGTAG, "error getting guest profile path", ioe); }
            } else if (GeckoApp.sIsUsingCustomProfile) {
                profile = " -P " + GeckoAppShell.getGeckoInterface().getProfile().getName();
            }
        }
        return (args != null ? args : "") + profile;
    }

    @Override
    public void run() {
        Looper.prepare();
        ThreadUtils.sGeckoThread = this;
        ThreadUtils.sGeckoHandler = new Handler();
        ThreadUtils.sGeckoQueue = Looper.myQueue();

        String path = initGeckoEnvironment();

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - runGecko");

        String args = addCustomProfileArg(mIntent.getStringExtra("args"));
        String type = getTypeFromAction(mIntent.getAction());
        mIntent = null;

        
        Log.i(LOGTAG, "RunGecko - args = " + args);
        GeckoAppShell.runGecko(path, args, mUri, type);
    }

    private static Object sLock = new Object();

    @Override
    public void handleMessage(String event, JSONObject message) {
        if ("Gecko:Ready".equals(event)) {
            GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
            setLaunchState(LaunchState.GeckoRunning);
            GeckoAppShell.sendPendingEventsToGecko();
        }
    }

    public static boolean checkLaunchState(LaunchState checkState) {
        synchronized (sLock) {
            return sLaunchState == checkState;
        }
    }

    static void setLaunchState(LaunchState setState) {
        synchronized (sLock) {
            sLaunchState = setState;
        }
    }

    



    static boolean checkAndSetLaunchState(LaunchState checkState, LaunchState setState) {
        synchronized (sLock) {
            if (sLaunchState != checkState)
                return false;
            sLaunchState = setState;
            return true;
        }
    }
}
