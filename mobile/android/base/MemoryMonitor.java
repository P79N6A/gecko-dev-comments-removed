




package org.mozilla.gecko;

import android.content.BroadcastReceiver;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Debug;
import android.util.Log;


















class MemoryMonitor extends BroadcastReceiver {
    private static final String LOGTAG = "GeckoMemoryMonitor";
    private static final String ACTION_MEMORY_DUMP = "org.mozilla.gecko.MEMORY_DUMP";
    private static final String ACTION_FORCE_PRESSURE = "org.mozilla.gecko.FORCE_MEMORY_PRESSURE";

    private static final int MEMORY_PRESSURE_NONE = 0;
    private static final int MEMORY_PRESSURE_CLEANUP = 1;
    private static final int MEMORY_PRESSURE_LOW = 2;
    private static final int MEMORY_PRESSURE_MEDIUM = 3;
    private static final int MEMORY_PRESSURE_HIGH = 4;

    private static MemoryMonitor sInstance = new MemoryMonitor();

    static MemoryMonitor getInstance() {
        return sInstance;
    }

    private final PressureDecrementer mPressureDecrementer;
    private int mMemoryPressure;
    private boolean mStoragePressure;

    private MemoryMonitor() {
        mPressureDecrementer = new PressureDecrementer();
        mMemoryPressure = MEMORY_PRESSURE_NONE;
        mStoragePressure = false;
    }

    public void init(Context context) {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_DEVICE_STORAGE_LOW);
        filter.addAction(Intent.ACTION_DEVICE_STORAGE_OK);
        filter.addAction(ACTION_MEMORY_DUMP);
        filter.addAction(ACTION_FORCE_PRESSURE);
        context.getApplicationContext().registerReceiver(this, filter);
    }

    public void onLowMemory() {
        Log.d(LOGTAG, "onLowMemory() notification received");
        increaseMemoryPressure(MEMORY_PRESSURE_HIGH);
    }

    public void onTrimMemory(int level) {
        Log.d(LOGTAG, "onTrimMemory() notification received with level " + level);
        if (Build.VERSION.SDK_INT < 14) {
            
            return;
        }

        if (level >= ComponentCallbacks2.TRIM_MEMORY_COMPLETE) {
            increaseMemoryPressure(MEMORY_PRESSURE_HIGH);
        } else if (level >= ComponentCallbacks2.TRIM_MEMORY_MODERATE) {
            increaseMemoryPressure(MEMORY_PRESSURE_MEDIUM);
        } else if (level >= ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN) {
            
            increaseMemoryPressure(MEMORY_PRESSURE_CLEANUP);
        } else {
            if (Build.VERSION.SDK_INT < 16) {
                
                
                increaseMemoryPressure(MEMORY_PRESSURE_LOW);
            } else if (level >= ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL) {
                increaseMemoryPressure(MEMORY_PRESSURE_HIGH);
            } else if (level >= ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW) {
                increaseMemoryPressure(MEMORY_PRESSURE_MEDIUM);
            } else {
                increaseMemoryPressure(MEMORY_PRESSURE_LOW);
            }
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_DEVICE_STORAGE_LOW.equals(intent.getAction())) {
            Log.d(LOGTAG, "Device storage is low");
            mStoragePressure = true;
            
            
        } else if (Intent.ACTION_DEVICE_STORAGE_OK.equals(intent.getAction())) {
            Log.d(LOGTAG, "Device storage is ok");
            mStoragePressure = false;
        } else if (ACTION_MEMORY_DUMP.equals(intent.getAction())) {
            String label = intent.getStringExtra("label");
            if (label == null) {
                label = "default";
            }
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Memory:Dump", label));
        } else if (ACTION_FORCE_PRESSURE.equals(intent.getAction())) {
            increaseMemoryPressure(MEMORY_PRESSURE_HIGH);
        }
    }

    private void increaseMemoryPressure(int level) {
        int oldLevel;
        synchronized (this) {
            
            if (mMemoryPressure > level) {
                return;
            }
            oldLevel = mMemoryPressure;
            mMemoryPressure = level;
        }

        
        
        
        
        
        mPressureDecrementer.start();

        if (oldLevel == level) {
            
            
            
            return;
        }

        
        if (level >= MEMORY_PRESSURE_MEDIUM) {
            if (GeckoApp.checkLaunchState(GeckoApp.LaunchState.GeckoRunning)) {
                GeckoAppShell.onLowMemory();
            }
            ScreenshotHandler.disableScreenshot(false);
            GeckoAppShell.geckoEventSync();
        }
    }

    private boolean decreaseMemoryPressure() {
        int newLevel;
        synchronized (this) {
            if (mMemoryPressure <= 0) {
                return false;
            }

            newLevel = --mMemoryPressure;
        }
        Log.d(LOGTAG, "Decreased memory pressure to " + newLevel);

        if (newLevel == MEMORY_PRESSURE_NONE) {
            ScreenshotHandler.enableScreenshot(false);
        }

        return true;
    }

    class PressureDecrementer implements Runnable {
        private static final int DECREMENT_DELAY = 5 * 60 * 1000; 

        private boolean mPosted;

        synchronized void start() {
            if (mPosted) {
                
                GeckoAppShell.getHandler().removeCallbacks(this);
            }
            GeckoAppShell.getHandler().postDelayed(this, DECREMENT_DELAY);
            mPosted = true;
        }

        @Override
        public synchronized void run() {
            if (!decreaseMemoryPressure()) {
                
                mPosted = false;
                return;
            }

            
            GeckoAppShell.getHandler().postDelayed(this, DECREMENT_DELAY);
        }
    }
}
