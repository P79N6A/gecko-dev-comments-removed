



































package org.mozilla.gecko;

import android.os.*;
import android.app.*;
import android.app.ActivityManager.*;
import android.util.*;















public class MemoryWatcher extends Handler
{
    private static final long MEMORY_WATCHER_INTERVAL = 2000;
    private static final long MEMORY_WATCHER_INTERVAL_DELAY_FACTOR = 5;
    private static final long MEMORY_WATCHER_CRITICAL_RESPONSE_THRESHOLD = 200; 

    private Handler mMemoryWatcherHandler;
    private ActivityManager mActivityManager;
    private MemoryInfo mMemoryInfo;
    private boolean mMemoryWatcherKeepGoing;
    private boolean mMemoryWatcherEnabled = false;

    public MemoryWatcher(GeckoApp app) {
        if (android.os.Build.MODEL.equals("Nexus S") == false)
            return;
        mMemoryWatcherEnabled = true;

        mMemoryWatcherKeepGoing = true;
        mMemoryInfo = new MemoryInfo();
        mActivityManager = (ActivityManager) app.getSystemService("activity");
    }


    @Override
    public void handleMessage(Message msg) {
        long startTime = System.currentTimeMillis();
        mActivityManager.getMemoryInfo(mMemoryInfo);
        long took = System.currentTimeMillis() - startTime;

        







        
        
        long nextInterval = MEMORY_WATCHER_INTERVAL;

        
        
        
        if (took > MEMORY_WATCHER_CRITICAL_RESPONSE_THRESHOLD) {
            GeckoAppShell.onCriticalOOM();
            nextInterval *= MEMORY_WATCHER_INTERVAL_DELAY_FACTOR;
        }
        else if (mMemoryInfo.lowMemory) {
            GeckoAppShell.onLowMemory();
            nextInterval *= MEMORY_WATCHER_INTERVAL_DELAY_FACTOR;
        }

        if (mMemoryWatcherKeepGoing == true)
            this.sendEmptyMessageDelayed(0, nextInterval);
    }

    public void StartMemoryWatcher() {
        if (mMemoryWatcherEnabled == false)
            return;
        mMemoryWatcherKeepGoing = true;
        sendEmptyMessageDelayed(0, MEMORY_WATCHER_INTERVAL);
    }

    public void StopMemoryWatcher() {
        if (mMemoryWatcherEnabled == false)
            return;
        mMemoryWatcherKeepGoing = false;
        removeMessages(0);
    }
}
