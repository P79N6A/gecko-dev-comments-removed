




package org.mozilla.gecko.favicons;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.support.v4.util.LruCache;
import android.util.Log;

import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

public class Favicons {
    private static final String LOGTAG = "GeckoFavicons";

    public static final int NOT_LOADING = 0;
    public static final int FAILED_EXPIRY_NEVER = -1;
    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;

    private static int sFaviconSmallSize = -1;
    private static int sFaviconLargeSize = -1;

    protected static Context sContext;

    private static final Map<Integer, LoadFaviconTask> sLoadTasks = Collections.synchronizedMap(new HashMap<Integer, LoadFaviconTask>());
    private static final LruCache<String, Bitmap> sFaviconCache = new LruCache<String, Bitmap>(1024 * 1024) {
        @Override
        protected int sizeOf(String url, Bitmap image) {
            return image.getRowBytes() * image.getHeight();
        }
    };

    public static Bitmap sDefaultFavicon;

    
    
    private static final LruCache<String, Long> sFailedCache = new LruCache<String, Long>(64);

    
    
    private static final LruCache<String, Integer> sColorCache = new LruCache<String, Integer>(256);
    static void dispatchResult(final String pageUrl, final Bitmap image,
            final OnFaviconLoadedListener listener) {
        if (pageUrl != null && image != null)
            putFaviconInMemCache(pageUrl, image);

        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (listener != null)
                    listener.onFaviconLoaded(pageUrl, image);
            }
        });
    }

    public static String getFaviconUrlForPageUrl(String pageUrl) {
        return BrowserDB.getFaviconUrlForHistoryUrl(sContext.getContentResolver(), pageUrl);
    }

    public static int loadFavicon(String pageUrl, String faviconUrl, int flags,
            OnFaviconLoadedListener listener) {

        
        if (pageUrl == null || pageUrl.length() == 0) {
            dispatchResult(null, null, listener);
            return -1;
        }

        
        if (isFailedFavicon(pageUrl)) {
            dispatchResult(pageUrl, null, listener);
            return -1;
        }

        
        Bitmap image = getFaviconFromMemCache(pageUrl);
        if (image != null) {
            dispatchResult(pageUrl, image, listener);
            return -1;
        }

        LoadFaviconTask task = new LoadFaviconTask(ThreadUtils.getBackgroundHandler(), pageUrl, faviconUrl, flags, listener);

        int taskId = task.getId();
        sLoadTasks.put(taskId, task);

        task.execute();

        return taskId;
    }

    public static Bitmap getFaviconFromMemCache(String pageUrl) {
        
        
        if (pageUrl == null) {
            return null;
        }

        return sFaviconCache.get(pageUrl);
    }

    public static void putFaviconInMemCache(String pageUrl, Bitmap image) {
        sFaviconCache.put(pageUrl, image);
    }

    public static void clearMemCache() {
        sFaviconCache.evictAll();
    }

    public static boolean isFailedFavicon(String pageUrl) {
        Long fetchTime = sFailedCache.get(pageUrl);
        if (fetchTime == null)
            return false;
        
        return true;
    }

    public static void putFaviconInFailedCache(String pageUrl, long fetchTime) {
        sFailedCache.put(pageUrl, fetchTime);
    }

    public static void clearFailedCache() {
        sFailedCache.evictAll();
    }

    public static boolean cancelFaviconLoad(int taskId) {
        Log.d(LOGTAG, "Requesting cancelation of favicon load (" + taskId + ")");

        boolean cancelled = false;
        synchronized (sLoadTasks) {
            if (!sLoadTasks.containsKey(taskId))
                return false;

            Log.d(LOGTAG, "Cancelling favicon load (" + taskId + ")");

            LoadFaviconTask task = sLoadTasks.get(taskId);
            cancelled = task.cancel(false);
        }
        return cancelled;
    }

    public static void close() {
        Log.d(LOGTAG, "Closing Favicons database");

        
        synchronized (sLoadTasks) {
            Set<Integer> taskIds = sLoadTasks.keySet();
            Iterator<Integer> iter = taskIds.iterator();
            while (iter.hasNext()) {
                int taskId = iter.next();
                cancelFaviconLoad(taskId);
            }
        }

        LoadFaviconTask.closeHTTPClient();
    }

    public static boolean isLargeFavicon(Bitmap image) {
        return image.getWidth() > sFaviconSmallSize || image.getHeight() > sFaviconSmallSize;
    }

    public static Bitmap scaleImage(Bitmap image) {
        
        
        if (isLargeFavicon(image)) {
            image = Bitmap.createScaledBitmap(image, sFaviconLargeSize, sFaviconLargeSize, false);
        } else {
            image = Bitmap.createScaledBitmap(image, sFaviconSmallSize, sFaviconSmallSize, false);
        }
        return image;
    }

    public static int getFaviconColor(Bitmap image, String key) {
        Integer color = sColorCache.get(key);
        if (color != null) {
            return color;
        }

        color = BitmapUtils.getDominantColor(image);
        sColorCache.put(key, color);
        return color;
    }

    public static void attachToContext(Context context) {
        sContext = context;
        if (sFaviconSmallSize < 0) {
            sFaviconSmallSize = Math.round(sContext.getResources().getDimension(R.dimen.favicon_size_small));
        }
        if (sFaviconLargeSize < 0) {
            sFaviconLargeSize = Math.round(sContext.getResources().getDimension(R.dimen.favicon_size_large));
        }

        
        sDefaultFavicon = BitmapUtils.decodeResource(sContext, R.drawable.favicon);
    }
    public static void removeLoadTask(long taskId) {
        sLoadTasks.remove(taskId);
    }
}
