



package org.mozilla.gecko;

import android.app.Application;

public class GeckoApplication extends Application {

    private boolean mInited;
    private boolean mInBackground;

    private LightweightTheme mLightweightTheme;

    protected void initialize() {
        if (mInited)
            return;

        
        try {
            Class.forName("android.os.AsyncTask");
        } catch (ClassNotFoundException e) {}

        mLightweightTheme = new LightweightTheme(this);

        GeckoConnectivityReceiver.getInstance().init(getApplicationContext());
        GeckoBatteryManager.getInstance().init(getApplicationContext());
        GeckoBatteryManager.getInstance().start();
        GeckoNetworkManager.getInstance().init(getApplicationContext());
        MemoryMonitor.getInstance().init(getApplicationContext());
        mInited = true;
    }

    protected void onActivityPause(GeckoActivity activity) {
        mInBackground = true;

        GeckoAppShell.sendEventToGecko(GeckoEvent.createPauseEvent(true));
        GeckoConnectivityReceiver.getInstance().stop();
        GeckoNetworkManager.getInstance().stop();
    }

    protected void onActivityResume(GeckoActivity activity) {
        if (GeckoApp.checkLaunchState(GeckoApp.LaunchState.GeckoRunning))
            GeckoAppShell.sendEventToGecko(GeckoEvent.createResumeEvent(true));
        GeckoConnectivityReceiver.getInstance().start();
        GeckoNetworkManager.getInstance().start();

        mInBackground = false;
    }

    public boolean isApplicationInBackground() {
        return mInBackground;
    }

    public LightweightTheme getLightweightTheme() {
        return mLightweightTheme;
    }
}
