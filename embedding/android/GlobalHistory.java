




































package org.mozilla.gecko;

import java.util.HashSet;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Set;
import java.lang.ref.SoftReference;

import android.database.Cursor;
import android.os.Handler;
import android.provider.Browser;
import android.util.Log;

class GlobalHistory {
    private static GlobalHistory sInstance = new GlobalHistory();

    static GlobalHistory getInstance() {
        return sInstance;
    }

    
    
    
    private static final long BATCHING_DELAY_MS = 100;
    private static final String LOG_NAME = "GlobalHistory";

    private final Handler mHandler;                     
    private final Queue<String> mPendingUris;           
    private SoftReference<Set<String>> mVisitedCache;   
    private final Runnable mNotifierRunnable;           
    private boolean mProcessing; 

    private GlobalHistory() {
        mHandler = GeckoAppShell.getHandler();
        mPendingUris = new LinkedList<String>();
        mVisitedCache = new SoftReference<Set<String>>(null);
        mNotifierRunnable = new Runnable() {
            public void run() {
                Set<String> visitedSet = mVisitedCache.get();
                if (visitedSet == null) {
                    
                    Log.w(LOG_NAME, "Rebuilding visited link set...");
                    visitedSet = new HashSet<String>();
                    Cursor c = GeckoApp.mAppContext.getContentResolver().query(Browser.BOOKMARKS_URI,
                                    new String[] { Browser.BookmarkColumns.URL },
                                    Browser.BookmarkColumns.BOOKMARK + "=0 AND " +
                                    Browser.BookmarkColumns.VISITS + ">0",
                                    null,
                                    null);
                    if (c.moveToFirst()) {
                        do {
                            visitedSet.add(c.getString(0));
                        } while (c.moveToNext());
                    }
                    mVisitedCache = new SoftReference<Set<String>>(visitedSet);
                }

                
                
                while (true) {
                    String uri = mPendingUris.poll();
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

    public void add(String uri) {
        Browser.updateVisitedHistory(GeckoApp.mAppContext.getContentResolver(), uri, true);
        Set<String> visitedSet = mVisitedCache.get();
        if (visitedSet != null) {
            visitedSet.add(uri);
        }
        GeckoAppShell.notifyUriVisited(uri);
    }

    public void checkUriVisited(final String uri) {
        mHandler.post(new Runnable() {
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
