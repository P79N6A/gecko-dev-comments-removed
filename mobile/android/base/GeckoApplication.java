



package org.mozilla.gecko;

import android.app.Application;

import java.util.ArrayList;

public class GeckoApplication extends Application {

    private boolean mInBackground;

    @Override
    public void onCreate() {
        
        try {
            Class.forName("android.os.AsyncTask");
        } catch (ClassNotFoundException e) {}

        super.onCreate();

        GeckoConnectivityReceiver.getInstance().init(getApplicationContext());
        GeckoBatteryManager.getInstance().init(getApplicationContext());
        GeckoBatteryManager.getInstance().start();
        GeckoNetworkManager.getInstance().init(getApplicationContext());
        MemoryMonitor.getInstance().init(getApplicationContext());
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
}
