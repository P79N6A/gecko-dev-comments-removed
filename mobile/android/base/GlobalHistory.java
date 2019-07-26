




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;

import java.lang.ref.SoftReference;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Set;

class GlobalHistory {
    private static final String LOGTAG = "GeckoGlobalHistory";

    private static GlobalHistory sInstance = new GlobalHistory();

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
        mHandler = GeckoAppShell.getHandler();
        mPendingUris = new LinkedList<String>();
        mVisitedCache = new SoftReference<Set<String>>(null);
        mNotifierRunnable = new Runnable() {
            public void run() {
                Set<String> visitedSet = mVisitedCache.get();
                if (visitedSet == null) {
                    
                    Log.w(LOGTAG, "Rebuilding visited link set...");
                    visitedSet = new HashSet<String>();
                    Cursor c = BrowserDB.getAllVisitedHistory(GeckoApp.mAppContext.getContentResolver());
                    if (c.moveToFirst()) {
                        do {
                            visitedSet.add(c.getString(0));
                        } while (c.moveToNext());
                    }
                    mVisitedCache = new SoftReference<Set<String>>(visitedSet);
                    c.close();
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

    public void addToGeckoOnly(String uri) {
        Set<String> visitedSet = mVisitedCache.get();
        if (visitedSet != null) {
            visitedSet.add(uri);
        }
        GeckoAppShell.notifyUriVisited(uri);
    }

    
    
    private boolean canAddURI(String uri) {
        if (uri == null || uri.length() == 0)
            return false;

        
        if (uri.startsWith("http:") || uri.startsWith("https:"))
            return true;

        String scheme = Uri.parse(uri).getScheme();
        if (scheme == null)
            return false;

        
        if (scheme.equals("about") ||
            scheme.equals("imap") ||
            scheme.equals("news") ||
            scheme.equals("mailbox") ||
            scheme.equals("moz-anno") ||
            scheme.equals("view-source") ||
            scheme.equals("chrome") ||
            scheme.equals("resource") ||
            scheme.equals("data") ||
            scheme.equals("wyciwyg") ||
            scheme.equals("javascript"))
            return false;

        return true;
    }

    public void add(String uri) {
        if (!canAddURI(uri))
            return;

        BrowserDB.updateVisitedHistory(GeckoApp.mAppContext.getContentResolver(), uri);
        addToGeckoOnly(uri);
    }

    public void update(String uri, String title) {
        if (!canAddURI(uri))
            return;

        BrowserDB.updateHistoryTitle(GeckoApp.mAppContext.getContentResolver(), uri, title);
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
