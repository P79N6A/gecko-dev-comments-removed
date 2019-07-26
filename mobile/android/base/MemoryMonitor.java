




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserContract;

import android.content.BroadcastReceiver;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
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
        if (increaseMemoryPressure(MEMORY_PRESSURE_HIGH)) {
            
            
            GeckoAppShell.geckoEventSync();
        }
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
            
            
            
            increaseMemoryPressure(MEMORY_PRESSURE_LOW);
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_DEVICE_STORAGE_LOW.equals(intent.getAction())) {
            Log.d(LOGTAG, "Device storage is low");
            mStoragePressure = true;
            GeckoAppShell.getHandler().post(new StorageReducer(context));
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

    private boolean increaseMemoryPressure(int level) {
        int oldLevel;
        synchronized (this) {
            
            if (mMemoryPressure > level) {
                return false;
            }
            oldLevel = mMemoryPressure;
            mMemoryPressure = level;
        }

        
        
        
        
        
        mPressureDecrementer.start();

        if (oldLevel == level) {
            
            
            
            return false;
        }

        
        if (level >= MEMORY_PRESSURE_MEDIUM) {
            if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
                GeckoAppShell.onLowMemory();
            }

            Favicons.getInstance().clearMemCache();
        }
        return true;
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

    class StorageReducer implements Runnable {
        private final Context mContext;
        public StorageReducer(final Context context) {
            this.mContext = context;
        }

        @Override
        public void run() {
            
            if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
                GeckoAppShell.getHandler().postDelayed(this, 10000);
                return;
            }

            if (!mStoragePressure) {
                
                return;
            }

            BrowserDB.expireHistory(mContext.getContentResolver(),
                                    BrowserContract.ExpirePriority.AGGRESSIVE);
            BrowserDB.removeThumbnails(mContext.getContentResolver());
            
        }
    }
}
