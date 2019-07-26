




package org.mozilla.gecko.favicons;

import android.graphics.BitmapFactory;
import android.text.TextUtils;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.cache.FaviconCache;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.support.v4.util.LruCache;
import android.util.Log;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

public class Favicons {
    private static final String LOGTAG = "GeckoFavicons";

    
    public static final int FAVICON_CACHE_SIZE_BYTES = 512 * 1024;

    
    public static final int PAGE_URL_MAPPINGS_TO_STORE = 128;

    public static final int NOT_LOADING = 0;
    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;

    protected static Context sContext;

    
    public static Bitmap sDefaultFavicon;

    
    public static int sDefaultFaviconSize;

    private static final Map<Integer, LoadFaviconTask> sLoadTasks = Collections.synchronizedMap(new HashMap<Integer, LoadFaviconTask>());

    
    
    private static final LruCache<String, String> sPageURLMappings = new LruCache<String, String>(PAGE_URL_MAPPINGS_TO_STORE);

    public static String getFaviconURLForPageURLFromCache(String pageURL) {
        return sPageURLMappings.get(pageURL);
    }

    



    public static void putFaviconURLForPageURLInCache(String pageURL, String faviconURL) {
        sPageURLMappings.put(pageURL, faviconURL);
    }

    private static FaviconCache sFaviconsCache;
    static void dispatchResult(final String pageUrl, final String faviconURL, final Bitmap image,
            final OnFaviconLoadedListener listener) {
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (listener != null) {
                    listener.onFaviconLoaded(pageUrl, faviconURL, image);
                }
            }
        });
    }

    













    public static int getFaviconForSize(String pageURL, String faviconURL, int targetSize, int flags, OnFaviconLoadedListener listener) {
        
        String cacheURL = faviconURL;
        if (cacheURL == null)  {
            cacheURL = guessDefaultFaviconURL(pageURL);
        }

        
        if (cacheURL == null) {
            dispatchResult(pageURL, null, sDefaultFavicon, listener);
            return NOT_LOADING;
        }

        Bitmap cachedIcon = getSizedFaviconFromCache(cacheURL, targetSize);
        if (cachedIcon != null) {
            dispatchResult(pageURL, cacheURL, cachedIcon, listener);
            return NOT_LOADING;
        }

        
        if (sFaviconsCache.isFailedFavicon(cacheURL)) {
            dispatchResult(pageURL, cacheURL, sDefaultFavicon, listener);
            return NOT_LOADING;
        }

        
        return loadUncachedFavicon(pageURL, faviconURL, flags, targetSize, listener);
    }

    









    public static Bitmap getSizedFaviconFromCache(String faviconURL, int targetSize) {
        return sFaviconsCache.getFaviconForDimensions(faviconURL, targetSize);
    }

    













    public static int getSizedFaviconForPageFromLocal(final String pageURL, final int targetSize, final OnFaviconLoadedListener callback) {
        
        
        String targetURL = sPageURLMappings.get(pageURL);
        if (targetURL != null) {
            
            if (sFaviconsCache.isFailedFavicon(targetURL)) {
                dispatchResult(pageURL, targetURL, null, callback);
                return NOT_LOADING;
            }

            
            Bitmap result = getSizedFaviconFromCache(targetURL, targetSize);
            if (result != null) {
                
                dispatchResult(pageURL, targetURL, result, callback);
                return NOT_LOADING;
            }
        }

        
        LoadFaviconTask task = new LoadFaviconTask(ThreadUtils.getBackgroundHandler(), pageURL, targetURL, 0, callback, targetSize, true);
        int taskId = task.getId();
        sLoadTasks.put(taskId, task);
        task.execute();
        return taskId;
    }

    public static int getSizedFaviconForPageFromLocal(final String pageURL, final OnFaviconLoadedListener callback) {
        return getSizedFaviconForPageFromLocal(pageURL, sDefaultFaviconSize, callback);
    }
    







    public static String getFaviconUrlForPageUrl(String pageURL) {
        
        
        String targetURL;
        Tab theTab = Tabs.getInstance().getTabForUrl(pageURL);
        if (theTab != null) {
            targetURL = theTab.getFaviconURL();
            if (targetURL != null) {
                return targetURL;
            }
        }

        targetURL = BrowserDB.getFaviconUrlForHistoryUrl(sContext.getContentResolver(), pageURL);
        if (targetURL == null) {
            
            targetURL = guessDefaultFaviconURL(pageURL);
        }
        return targetURL;
    }

    

















    private static int loadUncachedFavicon(String pageUrl, String faviconUrl, int flags, int targetSize, OnFaviconLoadedListener listener) {
        
        if (TextUtils.isEmpty(pageUrl)) {
            dispatchResult(null, null, null, listener);
            return NOT_LOADING;
        }

        LoadFaviconTask task = new LoadFaviconTask(ThreadUtils.getBackgroundHandler(), pageUrl, faviconUrl, flags, listener, targetSize, false);

        int taskId = task.getId();
        sLoadTasks.put(taskId, task);

        task.execute();

        return taskId;
    }

    public static void putFaviconInMemCache(String pageUrl, Bitmap image) {
        sFaviconsCache.putSingleFavicon(pageUrl, image);
    }

    public static void putFaviconsInMemCache(String pageUrl, Iterator<Bitmap> images) {
        sFaviconsCache.putFavicons(pageUrl, images);
    }

    public static void clearMemCache() {
        sFaviconsCache.evictAll();
        sPageURLMappings.evictAll();
    }

    public static void putFaviconInFailedCache(String faviconURL) {
        sFaviconsCache.putFailed(faviconURL);
    }

    public static boolean cancelFaviconLoad(int taskId) {
        if (taskId == NOT_LOADING) {
            return false;
        }

        boolean cancelled;
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
            sLoadTasks.clear();
        }

        LoadFaviconTask.closeHTTPClient();
    }

    





    public static int getFaviconColor(String url) {
        return sFaviconsCache.getDominantColor(url);
    }

    






    public static void attachToContext(Context context) throws Exception {
        sContext = context;

        
        sDefaultFavicon = BitmapFactory.decodeResource(context.getResources(), R.drawable.favicon);
        if (sDefaultFavicon == null) {
            throw new Exception("Null default favicon was returned from the resources system!");
        }

        sDefaultFaviconSize = context.getResources().getDimensionPixelSize(R.dimen.favicon_bg);
        sFaviconsCache = new FaviconCache(FAVICON_CACHE_SIZE_BYTES, context.getResources().getDimensionPixelSize(R.dimen.favicon_largest_interesting_size));
    }

    





    public static String guessDefaultFaviconURL(String pageURL) {
        
        
        
        
        if (pageURL.startsWith("about:") || pageURL.startsWith("jar:")) {
            return pageURL;
        }

        try {
            
            URI u = new URI(pageURL);
            return new URI(u.getScheme(),
                           u.getAuthority(),
                           "/favicon.ico", null,
                           null).toString();
        } catch (URISyntaxException e) {
            return null;
        }
    }

    public static void removeLoadTask(long taskId) {
        sLoadTasks.remove(taskId);
    }

    




    static boolean isFailedFavicon(String faviconURL) {
        return sFaviconsCache.isFailedFavicon(faviconURL);
    }

    








    public static void getLargestFaviconForPage(String url, OnFaviconLoadedListener onFaviconLoadedListener) {
        loadUncachedFavicon(url, null, 0, -1, onFaviconLoadedListener);
    }
}
