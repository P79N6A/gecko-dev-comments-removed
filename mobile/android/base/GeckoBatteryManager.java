




package org.mozilla.gecko;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.Build;
import android.os.SystemClock;
import android.util.Log;

public class GeckoBatteryManager extends BroadcastReceiver {
    private static final String LOGTAG = "GeckoBatteryManager";

    
    
    private final static double  kDefaultLevel         = 1.0;
    private final static boolean kDefaultCharging      = true;
    private final static double  kDefaultRemainingTime = 0.0;
    private final static double  kUnknownRemainingTime = -1.0;

    private static long    sLastLevelChange            = 0;
    private static boolean sNotificationsEnabled       = false;
    private static double  sLevel                      = kDefaultLevel;
    private static boolean sCharging                   = kDefaultCharging;
    private static double  sRemainingTime              = kDefaultRemainingTime;

    private static GeckoBatteryManager sInstance = new GeckoBatteryManager();

    private final IntentFilter mFilter;
    private Context mApplicationContext;
    private boolean mIsEnabled;

    public static GeckoBatteryManager getInstance() {
        return sInstance;
    }

    private GeckoBatteryManager() {
        mFilter = new IntentFilter();
        mFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
    }

    public synchronized void start(final Context context) {
        if (mIsEnabled) {
            Log.w(LOGTAG, "Already started!");
            return;
        }

        mApplicationContext = context.getApplicationContext();
        
        if (mApplicationContext.registerReceiver(this, mFilter) == null) {
            Log.e(LOGTAG, "Registering receiver failed");
        } else {
            mIsEnabled = true;
        }
    }

    public synchronized void stop() {
        if (!mIsEnabled) {
            Log.w(LOGTAG, "Already stopped!");
            return;
        }

        mApplicationContext.unregisterReceiver(this);
        mApplicationContext = null;
        mIsEnabled = false;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
            Log.e(LOGTAG, "Got an unexpected intent!");
            return;
        }

        boolean previousCharging = isCharging();
        double previousLevel = getLevel();

        
        
        
        
        
        
        if (intent.getBooleanExtra(BatteryManager.EXTRA_PRESENT, false) ||
                Build.MODEL.equals("Galaxy Nexus")) {
            int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
            if (plugged == -1) {
                sCharging = kDefaultCharging;
                Log.e(LOGTAG, "Failed to get the plugged status!");
            } else {
                
                
                sCharging = plugged != 0;
            }

            if (sCharging != previousCharging) {
                sRemainingTime = kUnknownRemainingTime;
                
                
                sLastLevelChange = 0;
            }

            
            double current =  (double)intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
            double max = (double)intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
            if (current == -1 || max == -1) {
                Log.e(LOGTAG, "Failed to get battery level!");
                sLevel = kDefaultLevel;
            } else {
                sLevel = current / max;
            }

            if (sLevel == 1.0 && sCharging) {
                sRemainingTime = kDefaultRemainingTime;
            } else if (sLevel != previousLevel) {
                
                if (sLastLevelChange != 0) {
                    
                    long currentTime = SystemClock.elapsedRealtime();
                    long dt = (currentTime - sLastLevelChange) / 1000;
                    double dLevel = sLevel - previousLevel;

                    if (sCharging) {
                        if (dLevel < 0) {
                            Log.w(LOGTAG, "When charging, level should increase!");
                            sRemainingTime = kUnknownRemainingTime;
                        } else {
                            sRemainingTime = Math.round(dt / dLevel * (1.0 - sLevel));
                        }
                    } else {
                        if (dLevel > 0) {
                            Log.w(LOGTAG, "When discharging, level should decrease!");
                            sRemainingTime = kUnknownRemainingTime;
                        } else {
                            sRemainingTime = Math.round(dt / -dLevel * sLevel);
                        }
                    }

                    sLastLevelChange = currentTime;
                } else {
                    
                    sLastLevelChange = SystemClock.elapsedRealtime();
                }
            }
        } else {
            sLevel = kDefaultLevel;
            sCharging = kDefaultCharging;
            sRemainingTime = kDefaultRemainingTime;
        }

        










        if (sNotificationsEnabled &&
                (previousCharging != isCharging() || previousLevel != getLevel())) {
            GeckoAppShell.notifyBatteryChange(getLevel(), isCharging(), getRemainingTime());
        }
    }

    public static boolean isCharging() {
        return sCharging;
    }

    public static double getLevel() {
        return sLevel;
    }

    public static double getRemainingTime() {
        return sRemainingTime;
    }

    public static void enableNotifications() {
        sNotificationsEnabled = true;
    }

    public static void disableNotifications() {
        sNotificationsEnabled = false;
    }

    public static double[] getCurrentInformation() {
        return new double[] { getLevel(), isCharging() ? 1.0 : 0.0, getRemainingTime() };
    }
}
