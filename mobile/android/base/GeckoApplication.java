



package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.LocalBrowserDB;
import org.mozilla.gecko.home.HomePanelsManager;
import org.mozilla.gecko.lwt.LightweightTheme;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.util.Log;

import java.io.File;

public class GeckoApplication extends Application 
    implements ContextGetter {
    private static final String LOG_TAG = "GeckoApplication";

    private static volatile GeckoApplication instance;

    private boolean mInBackground;
    private boolean mPausedGecko;

    private LightweightTheme mLightweightTheme;

    public GeckoApplication() {
        super();
        instance = this;
    }

    public static GeckoApplication get() {
        return instance;
    }

    @Override
    public Context getContext() {
        return this;
    }

    @Override
    public SharedPreferences getSharedPreferences() {
        return GeckoSharedPrefs.forApp(this);
    }

    



    @Override
    public void onConfigurationChanged(Configuration config) {
        Log.d(LOG_TAG, "onConfigurationChanged: " + config.locale +
                       ", background: " + mInBackground);

        
        
        if (mInBackground) {
            super.onConfigurationChanged(config);
            return;
        }

        
        
        try {
            BrowserLocaleManager.getInstance().correctLocale(this, getResources(), config);
        } catch (IllegalStateException ex) {
            
            Log.w(LOG_TAG, "Couldn't correct locale.", ex);
        }

        super.onConfigurationChanged(config);
    }

    public void onActivityPause(GeckoActivityStatus activity) {
        mInBackground = true;

        if ((activity.isFinishing() == false) &&
            (activity.isGeckoActivityOpened() == false)) {
            
            
            
            
            
            GeckoAppShell.sendEventToGecko(GeckoEvent.createAppBackgroundingEvent());
            mPausedGecko = true;

            final BrowserDB db = GeckoProfile.get(this).getDB();
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    db.expireHistory(getContentResolver(), BrowserContract.ExpirePriority.NORMAL);
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

        final Context applicationContext = getApplicationContext();
        GeckoBatteryManager.getInstance().start(applicationContext);
        GeckoConnectivityReceiver.getInstance().start(applicationContext);
        GeckoNetworkManager.getInstance().start(applicationContext);

        mInBackground = false;
    }

    @Override
    public void onCreate() {
        final Context context = getApplicationContext();
        HardwareUtils.init(context);
        Clipboard.init(context);
        FilePicker.init(context);
        GeckoLoader.loadMozGlue(context);
        DownloadsIntegration.init();
        HomePanelsManager.getInstance().init(context);

        
        NotificationHelper.getInstance(context).init();

        
        
        
        
        
        
        
        
        
        GeckoProfile.setBrowserDBFactory(new BrowserDB.Factory() {
            @Override
            public BrowserDB get(String profileName, File profileDir) {
                
                
                
                return new LocalBrowserDB(profileName);
            }
        });

        super.onCreate();

        if (AppConstants.MOZ_INSTALL_TRACKING) {
            AppConstants.getAdjustHelper().onCreate(this, AppConstants.MOZ_INSTALL_TRACKING_ADJUST_SDK_APP_TOKEN);
        }
    }

    public boolean isApplicationInBackground() {
        return mInBackground;
    }

    public LightweightTheme getLightweightTheme() {
        return mLightweightTheme;
    }

    public void prepareLightweightTheme() {
        mLightweightTheme = new LightweightTheme(this);
    }
}
