



package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Application;
import android.content.res.Configuration;
import android.util.Log;

public class GeckoApplication extends Application {

    private boolean mInited;
    private boolean mInBackground;
    private boolean mPausedGecko;

    private LightweightTheme mLightweightTheme;

    



    @Override
    public void onConfigurationChanged(Configuration config) {
        Log.d("GeckoApplication", "onConfigurationChanged: " + config.locale +
                                  ", background: " + mInBackground);

        
        
        if (mInBackground) {
            super.onConfigurationChanged(config);
            return;
        }

        
        
        LocaleManager.correctLocale(getResources(), config);
        super.onConfigurationChanged(config);
    }

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

    public void onActivityPause(GeckoActivityStatus activity) {
        mInBackground = true;

        if ((activity.isFinishing() == false) &&
            (activity.isGeckoActivityOpened() == false)) {
            
            
            
            
            
            GeckoAppShell.sendEventToGecko(GeckoEvent.createAppBackgroundingEvent());
            mPausedGecko = true;

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    BrowserDB.expireHistory(getContentResolver(),
                                            BrowserContract.ExpirePriority.NORMAL);
                }
            });
        }
        GeckoConnectivityReceiver.getInstance().stop();
        GeckoNetworkManager.getInstance().stop();
    }

    public void onActivityResume(GeckoActivityStatus activity) {
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
        HardwareUtils.init(getApplicationContext());
        Clipboard.init(getApplicationContext());
        GeckoLoader.loadMozGlue();
        super.onCreate();
    }

    public boolean isApplicationInBackground() {
        return mInBackground;
    }

    public LightweightTheme getLightweightTheme() {
        return mLightweightTheme;
    }
}
