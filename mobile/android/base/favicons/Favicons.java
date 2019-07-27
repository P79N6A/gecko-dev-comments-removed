




package org.mozilla.gecko.favicons;

import android.graphics.drawable.Drawable;
import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.cache.FaviconCache;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.NonEvictingLruCache;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

public class Favicons {
    private static final String LOGTAG = "GeckoFavicons";

    
    private static final String BUILT_IN_FAVICON_URL = "about:favicon";

    
    private static final String BUILT_IN_SEARCH_URL = "about:search";

    
    public static final int FAVICON_CACHE_SIZE_BYTES = 512 * 1024;

    
    public static final int NUM_PAGE_URL_MAPPINGS_TO_STORE = 128;

    public static final int NOT_LOADING  = 0;
    public static final int LOADED       = 1;

    
    public static Bitmap defaultFavicon;

    
    public static int defaultFaviconSize;

    
    public static int largestFaviconSize;

    
    public static int browserToolbarFaviconSize;

    
    public static final AtomicBoolean isInitialized = new AtomicBoolean(false);

    
    public static final ExecutorService longRunningExecutor = Executors.newSingleThreadExecutor();

    private static final SparseArray<LoadFaviconTask> loadTasks = new SparseArray<>();

    
    
    private static final NonEvictingLruCache<String, String> pageURLMappings = new NonEvictingLruCache<>(NUM_PAGE_URL_MAPPINGS_TO_STORE);

    
    private static final HashSet<String> sDecodableMimeTypes = new HashSet<>();

    
    
    private static final HashSet<String> sContainerMimeTypes = new HashSet<>();
    static {
        
        
        
        sContainerMimeTypes.add("image/vnd.microsoft.icon");
        sContainerMimeTypes.add("image/ico");
        sContainerMimeTypes.add("image/icon");
        sContainerMimeTypes.add("image/x-icon");
        sContainerMimeTypes.add("text/ico");
        sContainerMimeTypes.add("application/ico");

        
        sDecodableMimeTypes.addAll(sContainerMimeTypes);

        
        sDecodableMimeTypes.add("image/png");
        sDecodableMimeTypes.add("application/png");
        sDecodableMimeTypes.add("application/x-png");

        
        sDecodableMimeTypes.add("image/gif");

        
        sDecodableMimeTypes.add("image/jpeg");
        sDecodableMimeTypes.add("image/jpg");
        sDecodableMimeTypes.add("image/pipeg");
        sDecodableMimeTypes.add("image/vnd.swiftview-jpeg");
        sDecodableMimeTypes.add("application/jpg");
        sDecodableMimeTypes.add("application/x-jpg");

        
        sDecodableMimeTypes.add("application/bmp");
        sDecodableMimeTypes.add("application/x-bmp");
        sDecodableMimeTypes.add("application/x-win-bitmap");
        sDecodableMimeTypes.add("image/bmp");
        sDecodableMimeTypes.add("image/x-bmp");
        sDecodableMimeTypes.add("image/x-bitmap");
        sDecodableMimeTypes.add("image/x-xbitmap");
        sDecodableMimeTypes.add("image/x-win-bitmap");
        sDecodableMimeTypes.add("image/x-windows-bitmap");
        sDecodableMimeTypes.add("image/x-ms-bitmap");
        sDecodableMimeTypes.add("image/x-ms-bmp");
        sDecodableMimeTypes.add("image/ms-bmp");
    }

    public static String getFaviconURLForPageURLFromCache(String pageURL) {
        return pageURLMappings.get(pageURL);
    }

    



    public static void putFaviconURLForPageURLInCache(String pageURL, String faviconURL) {
        pageURLMappings.put(pageURL, faviconURL);
    }

    private static FaviconCache faviconsCache;

    




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
        final String faviconURL = pageURLMappings.get(pageURL);
        if (faviconURL == null) {
            return null;
        }
        return getSizedFaviconFromCache(faviconURL, targetSize);
    }

    














    public static int getSizedFavicon(Context context, String pageURL, String faviconURL, int targetSize, int flags, OnFaviconLoadedListener listener) {
        
        String cacheURL = faviconURL;
        if (cacheURL == null) {
            cacheURL = pageURLMappings.get(pageURL);
        }

        
        if (cacheURL == null)  {
            cacheURL = guessDefaultFaviconURL(pageURL);
        }

        
        if (cacheURL == null) {
            return dispatchResult(pageURL, null, defaultFavicon, listener);
        }

        Bitmap cachedIcon = getSizedFaviconFromCache(cacheURL, targetSize);
        if (cachedIcon != null) {
            return dispatchResult(pageURL, cacheURL, cachedIcon, listener);
        }

        
        if (faviconsCache.isFailedFavicon(cacheURL)) {
            return dispatchResult(pageURL, cacheURL, defaultFavicon, listener);
        }

        
        return loadUncachedFavicon(context, pageURL, faviconURL, flags, targetSize, listener);
    }

    









    public static Bitmap getSizedFaviconFromCache(String faviconURL, int targetSize) {
        return faviconsCache.getFaviconForDimensions(faviconURL, targetSize);
    }

    













    public static int getSizedFaviconForPageFromLocal(Context context, final String pageURL, final int targetSize,
                                                      final OnFaviconLoadedListener callback) {
        
        
        final String targetURL = pageURLMappings.get(pageURL);
        if (targetURL != null) {
            
            if (faviconsCache.isFailedFavicon(targetURL)) {
                return dispatchResult(pageURL, targetURL, null, callback);
            }

            
            final Bitmap result = getSizedFaviconFromCache(targetURL, targetSize);
            if (result != null) {
                
                return dispatchResult(pageURL, targetURL, result, callback);
            }
        }

        
        final LoadFaviconTask task =
            new LoadFaviconTask(context, pageURL, targetURL, 0, callback, targetSize, true);
        final int taskId = task.getId();
        synchronized(loadTasks) {
            loadTasks.put(taskId, task);
        }
        task.execute();

        return taskId;
    }

    public static int getSizedFaviconForPageFromLocal(Context context, final String pageURL, final OnFaviconLoadedListener callback) {
        return getSizedFaviconForPageFromLocal(context, pageURL, defaultFaviconSize, callback);
    }

    









    public static String getFaviconURLForPageURL(final BrowserDB db, final ContentResolver cr, final String pageURL) {
        
        
        String targetURL;
        Tab theTab = Tabs.getInstance().getFirstTabForUrl(pageURL);
        if (theTab != null) {
            targetURL = theTab.getFaviconURL();
            if (targetURL != null) {
                return targetURL;
            }
        }

        
        targetURL = db.getFaviconURLFromPageURL(cr, pageURL);
        if (targetURL != null) {
            return targetURL;
        }

        
        return guessDefaultFaviconURL(pageURL);
    }

    

















    private static int loadUncachedFavicon(Context context, String pageURL, String faviconURL, int flags,
                                           int targetSize, OnFaviconLoadedListener listener) {
        
        if (TextUtils.isEmpty(pageURL)) {
            dispatchResult(null, null, null, listener);
            return NOT_LOADING;
        }

        final LoadFaviconTask task =
            new LoadFaviconTask(context, pageURL, faviconURL, flags, listener, targetSize, false);
        final int taskId = task.getId();
        synchronized(loadTasks) {
            loadTasks.put(taskId, task);
        }
        task.execute();

        return taskId;
    }

    public static void putFaviconInMemCache(String pageUrl, Bitmap image) {
        faviconsCache.putSingleFavicon(pageUrl, image);
    }

    







    public static void putFaviconsInMemCache(String pageUrl, Iterator<Bitmap> images, boolean permanently) {
        faviconsCache.putFavicons(pageUrl, images, permanently);
    }

    public static void putFaviconsInMemCache(String pageUrl, Iterator<Bitmap> images) {
        putFaviconsInMemCache(pageUrl, images, false);
    }

    public static void clearMemCache() {
        faviconsCache.evictAll();
        pageURLMappings.evictAll();
    }

    public static void putFaviconInFailedCache(String faviconURL) {
        faviconsCache.putFailed(faviconURL);
    }

    public static boolean cancelFaviconLoad(int taskId) {
        if (taskId == NOT_LOADING) {
            return false;
        }

        synchronized (loadTasks) {
            if (loadTasks.indexOfKey(taskId) < 0) {
                return false;
            }

            Log.v(LOGTAG, "Cancelling favicon load " + taskId + ".");
            LoadFaviconTask task = loadTasks.get(taskId);
            return task.cancel();
        }
    }

    public static void close() {
        Log.d(LOGTAG, "Closing Favicons database");

        
        longRunningExecutor.shutdown();

        
        synchronized (loadTasks) {
            final int count = loadTasks.size();
            for (int i = 0; i < count; i++) {
                cancelFaviconLoad(loadTasks.keyAt(i));
            }
            loadTasks.clear();
        }

        LoadFaviconTask.closeHTTPClient();
    }

    





    public static int getFaviconColor(String url) {
        return faviconsCache.getDominantColor(url);
    }

    






    public static void initializeWithContext(Context context) throws IllegalStateException {
        
        if (!isInitialized.compareAndSet(false, true)) {
            return;
        }

        final Resources res = context.getResources();

        final Drawable defaultFaviconDrawable = res.getDrawable(R.drawable.toolbar_favicon_default);
        if (defaultFaviconDrawable instanceof BitmapDrawable) {
            defaultFavicon = ((BitmapDrawable) defaultFaviconDrawable).getBitmap();
        } else {
            throw new IllegalStateException("toolbar_favicon_default wasn't a bitmap resource!");
        }

        defaultFaviconSize = res.getDimensionPixelSize(R.dimen.favicon_bg);

        
        
        largestFaviconSize = res.getDimensionPixelSize(R.dimen.favicon_largest_interesting_size);

        browserToolbarFaviconSize = res.getDimensionPixelSize(R.dimen.browser_toolbar_favicon_size);

        faviconsCache = new FaviconCache(FAVICON_CACHE_SIZE_BYTES, largestFaviconSize);

        
        for (String url : AboutPages.getDefaultIconPages()) {
            pageURLMappings.putWithoutEviction(url, BUILT_IN_FAVICON_URL);
        }

        
        
        List<Bitmap> toInsert = Arrays.asList(loadBrandingBitmap(context, "favicon64.png"),
                                              loadBrandingBitmap(context, "favicon32.png"));

        putFaviconsInMemCache(BUILT_IN_FAVICON_URL, toInsert.iterator(), true);

        pageURLMappings.putWithoutEviction(AboutPages.HOME, BUILT_IN_SEARCH_URL);
        List<Bitmap> searchIcons = Collections.singletonList(BitmapFactory.decodeResource(res, R.drawable.favicon_search));
        putFaviconsInMemCache(BUILT_IN_SEARCH_URL, searchIcons.iterator(), true);
    }

    



    private static String getBrandingBitmapPath(Context context, String name) {
        return GeckoJarReader.getJarURL(context, "chrome/chrome/content/branding/" + name);
    }

    private static Bitmap loadBrandingBitmap(Context context, String name) {
        Bitmap b = GeckoJarReader.getBitmap(context,
                                            context.getResources(),
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

    




    public static boolean canDecodeType(String imgType) {
        return "".equals(imgType) || sDecodableMimeTypes.contains(imgType);
    }

    





    public static boolean isContainerType(String mimeType) {
        return sDecodableMimeTypes.contains(mimeType);
    }

    public static void removeLoadTask(int taskId) {
        synchronized(loadTasks) {
            loadTasks.delete(taskId);
        }
    }

    




    static boolean isFailedFavicon(String faviconURL) {
        return faviconsCache.isFailedFavicon(faviconURL);
    }

    











    public static void getPreferredSizeFaviconForPage(Context context, String url, OnFaviconLoadedListener onFaviconLoadedListener) {
        int preferredSize = GeckoAppShell.getPreferredIconSize();
        loadUncachedFavicon(context, url, null, 0, preferredSize, onFaviconLoadedListener);
    }
}
