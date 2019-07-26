




package org.mozilla.gecko;

import java.lang.ref.SoftReference;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Set;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.util.ThreadUtils;

import android.database.Cursor;
import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;

class GlobalHistory {
    private static final String LOGTAG = "GeckoGlobalHistory";

    private static final String TELEMETRY_HISTOGRAM_ADD = "FENNEC_GLOBALHISTORY_ADD_MS";
    private static final String TELEMETRY_HISTOGRAM_UPDATE = "FENNEC_GLOBALHISTORY_UPDATE_MS";
    private static final String TELEMETRY_HISTOGRAM_BUILD_VISITED_LINK = "FENNEC_GLOBALHISTORY_VISITED_BUILD_MS";

    private static final GlobalHistory sInstance = new GlobalHistory();

    static GlobalHistory getInstance() {
        return sInstance;
    }

    
    
    
    private static final long BATCHING_DELAY_MS = 100;

    private final Handler mHandler;                     
    private final Queue<String> mPendingUris;           
    private SoftReference<Set<String>> mVisitedCache;   
    private final Runnable mNotifierRunnable;           
    private boolean mProcessing; 

    private GlobalHistory() {
        mHandler = ThreadUtils.getBackgroundHandler();
        mPendingUris = new LinkedList<String>();
        mVisitedCache = new SoftReference<Set<String>>(null);
        mNotifierRunnable = new Runnable() {
            @Override
            public void run() {
                Set<String> visitedSet = mVisitedCache.get();
                if (visitedSet == null) {
                    
                    Log.w(LOGTAG, "Rebuilding visited link set...");
                    final long start = SystemClock.uptimeMillis();
                    final Cursor c = BrowserDB.getAllVisitedHistory(GeckoAppShell.getContext().getContentResolver());
                    if (c == null) {
                        return;
                    }

                    try {
                        visitedSet = new HashSet<String>();
                        if (c.moveToFirst()) {
                            do {
                                visitedSet.add(c.getString(0));
                            } while (c.moveToNext());
                        }
                        mVisitedCache = new SoftReference<Set<String>>(visitedSet);
                        final long end = SystemClock.uptimeMillis();
                        final long took = end - start;
                        Telemetry.HistogramAdd(TELEMETRY_HISTOGRAM_BUILD_VISITED_LINK, (int) Math.min(took, Integer.MAX_VALUE));
                    } finally {
                        c.close();
                    }
                }

                
                
                while (true) {
                    final String uri = mPendingUris.poll();
                    if (uri == null) {
                        break;
                    }

                    if (visitedSet.contains(uri)) {
                        GeckoAppShell.notifyUriVisited(uri);
                    }
                }
                mProcessing = false;
            }
        };
    }

    public void addToGeckoOnly(String uri) {
        Set<String> visitedSet = mVisitedCache.get();
        if (visitedSet != null) {
            visitedSet.add(uri);
        }
        GeckoAppShell.notifyUriVisited(uri);
    }

    public void add(String uri) {
        final long start = SystemClock.uptimeMillis();
        BrowserDB.updateVisitedHistory(GeckoAppShell.getContext().getContentResolver(), uri);
        final long end = SystemClock.uptimeMillis();
        final long took = end - start;
        Telemetry.HistogramAdd(TELEMETRY_HISTOGRAM_ADD, (int) Math.min(took, Integer.MAX_VALUE));
        addToGeckoOnly(uri);
    }

    @SuppressWarnings("static-method")
    public void update(String uri, String title) {
        final long start = SystemClock.uptimeMillis();
        BrowserDB.updateHistoryTitle(GeckoAppShell.getContext().getContentResolver(), uri, title);
        final long end = SystemClock.uptimeMillis();
        final long took = end - start;
        Telemetry.HistogramAdd(TELEMETRY_HISTOGRAM_UPDATE, (int) Math.min(took, Integer.MAX_VALUE));
    }

    public void checkUriVisited(final String uri) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                
                
                mPendingUris.add(uri);
                if (mProcessing) {
                    
                    
                    return;
                }
                mProcessing = true;
                mHandler.postDelayed(mNotifierRunnable, BATCHING_DELAY_MS);
            }
        });
    }
}
