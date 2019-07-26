



package org.mozilla.gecko;

import android.app.Application;
import org.mozilla.gecko.mozglue.GeckoLoader;

public class GeckoApplication extends Application {

    private boolean mInited;
    private boolean mInBackground;
    private boolean mPausedGecko;

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

    protected void onActivityPause(GeckoActivityStatus activity) {
        mInBackground = true;

        if ((activity.isFinishing() == false) &&
            (activity.isGeckoActivityOpened() == false)) {
            
            
            
            
            
            GeckoAppShell.sendEventToGecko(GeckoEvent.createAppBackgroundingEvent());
            mPausedGecko = true;
        }
        GeckoConnectivityReceiver.getInstance().stop();
        GeckoNetworkManager.getInstance().stop();
    }

    protected void onActivityResume(GeckoActivityStatus activity) {
        if (mPausedGecko) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createAppForegroundingEvent());
            mPausedGecko = false;
        }
        GeckoConnectivityReceiver.getInstance().start();
        GeckoNetworkManager.getInstance().start();

        mInBackground = false;
    }

    @Override
    public void onCreate() {
        GeckoLoader.loadMozGlue(getApplicationContext());
        super.onCreate();
    }

    public boolean isApplicationInBackground() {
        return mInBackground;
    }

    public LightweightTheme getLightweightTheme() {
        return mLightweightTheme;
    }
}
