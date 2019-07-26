




package org.mozilla.gecko.favicons;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.cache.FaviconCache;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.NonEvictingLruCache;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import java.io.File;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.Set;

public class Favicons {
    private static final String LOGTAG = "GeckoFavicons";

    
    private static final String BUILT_IN_FAVICON_URL = "about:favicon";

    
    public static final int FAVICON_CACHE_SIZE_BYTES = 512 * 1024;

    
    public static final int NUM_PAGE_URL_MAPPINGS_TO_STORE = 128;

    public static final int NOT_LOADING  = 0;
    public static final int LOADED       = 1;
    public static final int FLAG_PERSIST = 2;
    public static final int FLAG_SCALE   = 4;

    protected static Context sContext;

    
    public static Bitmap sDefaultFavicon;

    
    public static int sDefaultFaviconSize;

    
    public static int sLargestFaviconSize;

    private static final SparseArray<LoadFaviconTask> sLoadTasks = new SparseArray<LoadFaviconTask>();

    
    
    private static final NonEvictingLruCache<String, String> sPageURLMappings = new NonEvictingLruCache<String, String>(NUM_PAGE_URL_MAPPINGS_TO_STORE);

    public static String getFaviconURLForPageURLFromCache(String pageURL) {
        return sPageURLMappings.get(pageURL);
    }

    



    public static void putFaviconURLForPageURLInCache(String pageURL, String faviconURL) {
        sPageURLMappings.put(pageURL, faviconURL);
    }

    private static FaviconCache sFaviconsCache;

    




    static int dispatchResult(final String pageUrl, final String faviconURL, final Bitmap image,
            final OnFaviconLoadedListener listener) {
        if (listener == null) {
            return NOT_LOADING;
        }

        if (ThreadUtils.isOnUiThread()) {
            listener.onFaviconLoaded(pageUrl, faviconURL, image);
            return LOADED;
        }

        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                listener.onFaviconLoaded(pageUrl, faviconURL, image);
            }
        });
        return NOT_LOADING;
    }

    





    public static Bitmap getSizedFaviconForPageFromCache(final String pageURL, int targetSize) {
        final String faviconURL = sPageURLMappings.get(pageURL);
        if (faviconURL == null) {
            return null;
        }
        return getSizedFaviconFromCache(faviconURL, targetSize);
    }

    














    public static int getSizedFavicon(String pageURL, String faviconURL, int targetSize, int flags, OnFaviconLoadedListener listener) {
        
        String cacheURL = faviconURL;
        if (cacheURL == null) {
            cacheURL = sPageURLMappings.get(pageURL);
        }

        
        if (cacheURL == null)  {
            cacheURL = guessDefaultFaviconURL(pageURL);
        }

        
        if (cacheURL == null) {
            return dispatchResult(pageURL, null, sDefaultFavicon, listener);
        }

        Bitmap cachedIcon = getSizedFaviconFromCache(cacheURL, targetSize);
        if (cachedIcon != null) {
            return dispatchResult(pageURL, cacheURL, cachedIcon, listener);
        }

        
        if (sFaviconsCache.isFailedFavicon(cacheURL)) {
            return dispatchResult(pageURL, cacheURL, sDefaultFavicon, listener);
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
                return dispatchResult(pageURL, targetURL, null, callback);
            }

            
            Bitmap result = getSizedFaviconFromCache(targetURL, targetSize);
            if (result != null) {
                
                return dispatchResult(pageURL, targetURL, result, callback);
            }
        }

        
        LoadFaviconTask task = new LoadFaviconTask(ThreadUtils.getBackgroundHandler(), pageURL, targetURL, 0, callback, targetSize, true);
        int taskId = task.getId();
        synchronized(sLoadTasks) {
            sLoadTasks.put(taskId, task);
        }
        task.execute();
        return taskId;
    }

    public static int getSizedFaviconForPageFromLocal(final String pageURL, final OnFaviconLoadedListener callback) {
        return getSizedFaviconForPageFromLocal(pageURL, sDefaultFaviconSize, callback);
    }

    







    public static String getFaviconURLForPageURL(String pageURL) {
        
        
        String targetURL;
        Tab theTab = Tabs.getInstance().getFirstTabForUrl(pageURL);
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
        synchronized(sLoadTasks) {
            sLoadTasks.put(taskId, task);
        }

        task.execute();

        return taskId;
    }

    public static void putFaviconInMemCache(String pageUrl, Bitmap image) {
        sFaviconsCache.putSingleFavicon(pageUrl, image);
    }

    







    public static void putFaviconsInMemCache(String pageUrl, Iterator<Bitmap> images, boolean permanently) {
        sFaviconsCache.putFavicons(pageUrl, images, permanently);
    }

    public static void putFaviconsInMemCache(String pageUrl, Iterator<Bitmap> images) {
        putFaviconsInMemCache(pageUrl, images, false);
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
            if (sLoadTasks.indexOfKey(taskId) < 0)
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
            final int count = sLoadTasks.size();
            for (int i = 0; i < count; i++) {
                cancelFaviconLoad(sLoadTasks.keyAt(i));
            }
            sLoadTasks.clear();
        }

        LoadFaviconTask.closeHTTPClient();
    }

    





    public static int getFaviconColor(String url) {
        return sFaviconsCache.getDominantColor(url);
    }

    






    public static void attachToContext(Context context) throws Exception {
        final Resources res = context.getResources();
        sContext = context;

        
        sDefaultFavicon = BitmapFactory.decodeResource(res, R.drawable.favicon);
        if (sDefaultFavicon == null) {
            throw new Exception("Null default favicon was returned from the resources system!");
        }

        sDefaultFaviconSize = res.getDimensionPixelSize(R.dimen.favicon_bg);

        
        
        sLargestFaviconSize = context.getResources().getDimensionPixelSize(R.dimen.favicon_largest_interesting_size);
        sFaviconsCache = new FaviconCache(FAVICON_CACHE_SIZE_BYTES, sLargestFaviconSize);

        
        for (String url : AboutPages.getDefaultIconPages()) {
            sPageURLMappings.putWithoutEviction(url, BUILT_IN_FAVICON_URL);
        }

        
        
        ArrayList<Bitmap> toInsert = new ArrayList<Bitmap>(2);
        toInsert.add(loadBrandingBitmap(context, "favicon64.png"));
        toInsert.add(loadBrandingBitmap(context, "favicon32.png"));
        putFaviconsInMemCache(BUILT_IN_FAVICON_URL, toInsert.iterator(), true);
    }

    



    private static String getBrandingBitmapPath(Context context, String name) {
        final String apkPath = context.getPackageResourcePath();
        return "jar:jar:" + new File(apkPath).toURI() + "!/" +
               AppConstants.OMNIJAR_NAME + "!/" +
               "chrome/chrome/content/branding/" + name;
    }

    private static Bitmap loadBrandingBitmap(Context context, String name) {
        Bitmap b = GeckoJarReader.getBitmap(context.getResources(),
                                            getBrandingBitmapPath(context, name));
        if (b == null) {
            throw new IllegalStateException("Bitmap " + name + " missing from JAR!");
        }
        return b;
    }

    





    public static String guessDefaultFaviconURL(String pageURL) {
        
        
        
        
        if (AboutPages.isAboutPage(pageURL) || pageURL.startsWith("jar:")) {
            return pageURL;
        }

        try {
            
            URI u = new URI(pageURL);
            return new URI(u.getScheme(),
                           u.getAuthority(),
                           "/favicon.ico", null,
                           null).toString();
        } catch (URISyntaxException e) {
            Log.e(LOGTAG, "URISyntaxException getting default favicon URL", e);
            return null;
        }
    }

    public static void removeLoadTask(int taskId) {
        synchronized(sLoadTasks) {
            sLoadTasks.delete(taskId);
        }
    }

    




    static boolean isFailedFavicon(String faviconURL) {
        return sFaviconsCache.isFailedFavicon(faviconURL);
    }

    








    public static void getLargestFaviconForPage(String url, OnFaviconLoadedListener onFaviconLoadedListener) {
        loadUncachedFavicon(url, null, 0, -1, onFaviconLoadedListener);
    }
}
