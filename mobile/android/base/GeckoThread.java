




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

    static GeckoThread sGeckoThread;
    static String sUri;
    static String sArgs;
    static String sAction;

    static void setStartupArgs(String uri, String args, String action) {
        if (uri != null)
            sUri = uri;
        if (args != null)
            sArgs = args;
        if (action != null)
            sAction = action;
    }

    static GeckoThread getInstance() {
        if (sGeckoThread == null) {
            sGeckoThread = new GeckoThread();
        }
        return sGeckoThread;
    }

    private GeckoThread() {
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
        String profile = GeckoAppShell.getGeckoInterface() == null || GeckoApp.sIsUsingCustomProfile ? "" : (" -P " + GeckoAppShell.getGeckoInterface().getProfile().getName());
        return (args != null ? args : "") + profile;
    }

    @Override
    public void run() {
        Looper.prepare();
        ThreadUtils.setGeckoThread(this, new Handler());

        String path = initGeckoEnvironment();

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - runGecko");

        String args = addCustomProfileArg(sArgs);
        String type = getTypeFromAction(sAction != null ? sAction :
                                        sUri != null ? Intent.ACTION_VIEW : Intent.ACTION_MAIN);

        
        Log.i(LOGTAG, "RunGecko - args = " + args);
        GeckoAppShell.runGecko(path, args, sUri, type);
    }

    static Object sLock = new Object();

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
