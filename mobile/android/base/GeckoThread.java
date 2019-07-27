




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;

import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicReference;

public class GeckoThread extends Thread implements GeckoEventListener {
    private static final String LOGTAG = "GeckoThread";

    @RobocopTarget
    public enum LaunchState {
        Launching,
        WaitForDebugger,
        Launched,
        GeckoRunning,
        GeckoExiting,
        GeckoExited
    }

    private static final AtomicReference<LaunchState> sLaunchState =
                                            new AtomicReference<LaunchState>(LaunchState.Launching);

    private static GeckoThread sGeckoThread;

    private final String mArgs;
    private final String mAction;
    private final String mUri;

    public static boolean ensureInit() {
        ThreadUtils.assertOnUiThread();
        if (isCreated())
            return false;
        sGeckoThread = new GeckoThread(sArgs, sAction, sUri);
        ThreadUtils.sGeckoThread = sGeckoThread;
        return true;
    }

    public static String sArgs;
    public static String sAction;
    public static String sUri;

    public static void setArgs(String args) {
        sArgs = args;
    }

    public static void setAction(String action) {
        sAction = action;
    }

    public static void setUri(String uri) {
        sUri = uri;
    }

    GeckoThread(String args, String action, String uri) {
        mArgs = args;
        mAction = action;
        mUri = uri;
        setName("Gecko");
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "Gecko:Ready");
    }

    public static boolean isCreated() {
        return sGeckoThread != null;
    }

    public static void createAndStart() {
        if (ensureInit())
            sGeckoThread.start();
    }

    private String initGeckoEnvironment() {
        
        
        Locale locale = Locale.getDefault();

        if (locale.toString().equalsIgnoreCase("zh_hk")) {
            locale = Locale.TRADITIONAL_CHINESE;
            Locale.setDefault(locale);
        }

        Context context = GeckoAppShell.getContext();
        String resourcePath = "";
        Resources res  = null;
        String[] pluginDirs = null;
        try {
            pluginDirs = GeckoAppShell.getPluginDirectories();
        } catch (Exception e) {
            Log.w(LOGTAG, "Caught exception getting plugin dirs.", e);
        }

        resourcePath = context.getPackageResourcePath();
        res = context.getResources();
        GeckoLoader.setupGeckoEnvironment(context, pluginDirs, context.getFilesDir().getPath());

        GeckoLoader.loadSQLiteLibs(context, resourcePath);
        GeckoLoader.loadNSSLibs(context, resourcePath);
        GeckoLoader.loadGeckoLibs(context, resourcePath);
        GeckoJavaSampler.setLibsLoaded();

        Locale.setDefault(locale);

        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, null);

        return resourcePath;
    }

    private String getTypeFromAction(String action) {
        if (action != null && action.startsWith(GeckoApp.ACTION_WEBAPP_PREFIX)) {
            return "-webapp";
        }
        if (GeckoApp.ACTION_HOMESCREEN_SHORTCUT.equals(action)) {
            return "-bookmark";
        }
        return null;
    }

    private String addCustomProfileArg(String args) {
        String profileArg = "";
        String guestArg = "";
        if (GeckoAppShell.getGeckoInterface() != null) {
            final GeckoProfile profile = GeckoAppShell.getGeckoInterface().getProfile();

            if (profile.inGuestMode()) {
                try {
                    profileArg = " -profile " + profile.getDir().getCanonicalPath();
                } catch (final IOException ioe) {
                    Log.e(LOGTAG, "error getting guest profile path", ioe);
                }

                if (args == null || !args.contains(BrowserApp.GUEST_BROWSING_ARG)) {
                    guestArg = " " + BrowserApp.GUEST_BROWSING_ARG;
                }
            } else if (!GeckoProfile.sIsUsingCustomProfile) {
                
                
                profileArg = " -P " + profile.forceCreate().getName();
            }
        }

        return (args != null ? args : "") + profileArg + guestArg;
    }

    @Override
    public void run() {
        Looper.prepare();
        ThreadUtils.sGeckoHandler = new Handler();
        ThreadUtils.sGeckoQueue = Looper.myQueue();

        String path = initGeckoEnvironment();

        
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override public void run() {
                GeckoAppShell.registerJavaUiThread();
            }
        });

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - runGecko");

        String args = addCustomProfileArg(mArgs);
        String type = getTypeFromAction(mAction);

        if (!AppConstants.MOZILLA_OFFICIAL) {
            Log.i(LOGTAG, "RunGecko - args = " + args);
        }
        
        GeckoAppShell.runGecko(path, args, mUri, type);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        if ("Gecko:Ready".equals(event)) {
            EventDispatcher.getInstance().unregisterGeckoThreadListener(this, event);
            setLaunchState(LaunchState.GeckoRunning);
            GeckoAppShell.sendPendingEventsToGecko();
        }
    }

    @RobocopTarget
    public static boolean checkLaunchState(LaunchState checkState) {
        return sLaunchState.get() == checkState;
    }

    static void setLaunchState(LaunchState setState) {
        sLaunchState.set(setState);
    }

    



    static boolean checkAndSetLaunchState(LaunchState checkState, LaunchState setState) {
        return sLaunchState.compareAndSet(checkState, setState);
    }
}
