




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONException;
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
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicReference;

public class GeckoThread extends Thread implements GeckoEventListener {
    private static final String LOGTAG = "GeckoThread";

    @RobocopTarget
    public enum LaunchState {
        Launching,
        Launched,
        GeckoRunning,
        GeckoExiting,
        GeckoExited
    }

    private static final AtomicReference<LaunchState> sLaunchState =
                                            new AtomicReference<LaunchState>(LaunchState.Launching);
    private static final Queue<GeckoEvent> PENDING_EVENTS = new ConcurrentLinkedQueue<GeckoEvent>();

    private static GeckoThread sGeckoThread;

    private final String mArgs;
    private final String mAction;
    private final String mUri;
    private final boolean mDebugging;

    GeckoThread(String args, String action, String uri, boolean debugging) {
        mArgs = args;
        mAction = action;
        mUri = uri;
        mDebugging = debugging;

        setName("Gecko");
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "Gecko:Ready");
    }

    public static boolean ensureInit(String args, String action, String uri) {
        return ensureInit(args, action, uri,  false);
    }

    public static boolean ensureInit(String args, String action, String uri, boolean debugging) {
        ThreadUtils.assertOnUiThread();
        if (checkLaunchState(LaunchState.Launching) && sGeckoThread == null) {
            sGeckoThread = new GeckoThread(args, action, uri, debugging);
            return true;
        }
        return false;
    }

    public static boolean launch() {
        ThreadUtils.assertOnUiThread();
        if (checkAndSetLaunchState(LaunchState.Launching, LaunchState.Launched)) {
            sGeckoThread.start();
            return true;
        }
        return false;
    }

    public static boolean isLaunched() {
        return !checkLaunchState(LaunchState.Launching);
    }

    private String initGeckoEnvironment() {
        final Locale locale = Locale.getDefault();

        final Context context = GeckoAppShell.getContext();
        final Resources res = context.getResources();
        if (locale.toString().equalsIgnoreCase("zh_hk")) {
            final Locale mappedLocale = Locale.TRADITIONAL_CHINESE;
            Locale.setDefault(mappedLocale);
            Configuration config = res.getConfiguration();
            config.locale = mappedLocale;
            res.updateConfiguration(config, null);
        }

        String resourcePath = "";
        String[] pluginDirs = null;
        try {
            pluginDirs = GeckoAppShell.getPluginDirectories();
        } catch (Exception e) {
            Log.w(LOGTAG, "Caught exception getting plugin dirs.", e);
        }

        resourcePath = context.getPackageResourcePath();
        GeckoLoader.setupGeckoEnvironment(context, pluginDirs, context.getFilesDir().getPath());

        GeckoLoader.loadSQLiteLibs(context, resourcePath);
        GeckoLoader.loadNSSLibs(context, resourcePath);
        GeckoLoader.loadGeckoLibs(context, resourcePath);
        GeckoJavaSampler.setLibsLoaded();

        return resourcePath;
    }

    private String getTypeFromAction(String action) {
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
        ThreadUtils.sGeckoThread = this;
        ThreadUtils.sGeckoHandler = new Handler();

        if (mDebugging) {
            try {
                Thread.sleep(5 * 1000 );
            } catch (final InterruptedException e) {
            }
        }

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

        
        GeckoThread.setLaunchState(GeckoThread.LaunchState.GeckoExited);

        try {
            final JSONObject msg = new JSONObject();
            msg.put("type", "Gecko:Exited");
            EventDispatcher.getInstance().dispatchEvent(msg, null);
        } catch (final JSONException e) {
            Log.e(LOGTAG, "unable to dispatch event", e);
        }
    }

    public static void addPendingEvent(final GeckoEvent e) {
        synchronized (PENDING_EVENTS) {
            if (checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
                
                GeckoAppShell.notifyGeckoOfEvent(e);
                e.recycle();
            } else {
                
                PENDING_EVENTS.add(e);
            }
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        if ("Gecko:Ready".equals(event)) {
            EventDispatcher.getInstance().unregisterGeckoThreadListener(this, event);

            
            
            synchronized (PENDING_EVENTS) {
                GeckoEvent e;
                while ((e = PENDING_EVENTS.poll()) != null) {
                    GeckoAppShell.notifyGeckoOfEvent(e);
                    e.recycle();
                }
                setLaunchState(LaunchState.GeckoRunning);
            }
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
