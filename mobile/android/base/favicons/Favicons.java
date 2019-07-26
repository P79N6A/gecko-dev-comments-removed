




package org.mozilla.gecko.favicons;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.entity.BufferedHttpEntity;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.http.AndroidHttpClient;
import android.os.Handler;
import android.support.v4.util.LruCache;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

public class Favicons {
    private static final String LOGTAG = "GeckoFavicons";

    public static final long NOT_LOADING = 0;
    public static final long FAILED_EXPIRY_NEVER = -1;
    public static final int FLAG_PERSIST = 1;
    public static final int FLAG_SCALE = 2;

    private static int sFaviconSmallSize = -1;
    private static int sFaviconLargeSize = -1;

    private static Context sContext;

    private static final Map<Long,LoadFaviconTask> sLoadTasks = Collections.synchronizedMap(new HashMap<Long, LoadFaviconTask>());
    private static long sNextFaviconLoadId;
    private static final LruCache<String, Bitmap> sFaviconCache = new LruCache<String, Bitmap>(1024 * 1024) {
        @Override
        protected int sizeOf(String url, Bitmap image) {
            return image.getRowBytes() * image.getHeight();
        }
    };

    
    
    private static final LruCache<String, Long> sFailedCache = new LruCache<String, Long>(64);

    
    
    private static final LruCache<String, Integer> sColorCache = new LruCache<String, Integer>(256);
    private static final String USER_AGENT = GeckoAppShell.getGeckoInterface().getDefaultUAString();
    private static AndroidHttpClient sHttpClient;

    public interface OnFaviconLoadedListener {
        public void onFaviconLoaded(String url, Bitmap favicon);
    }

    private static synchronized AndroidHttpClient getHttpClient() {
        if (sHttpClient != null)
            return sHttpClient;

        sHttpClient = AndroidHttpClient.newInstance(USER_AGENT);
        return sHttpClient;
    }

    private static void dispatchResult(final String pageUrl, final Bitmap image,
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

    public static long loadFavicon(String pageUrl, String faviconUrl, int flags,
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

        long taskId = task.getId();
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

    public static boolean cancelFaviconLoad(long taskId) {
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
            Set<Long> taskIds = sLoadTasks.keySet();
            Iterator<Long> iter = taskIds.iterator();
            while (iter.hasNext()) {
                long taskId = iter.next();
                cancelFaviconLoad(taskId);
            }
        }
        if (sHttpClient != null)
            sHttpClient.close();
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
    }

    private static class LoadFaviconTask extends UiAsyncTask<Void, Void, Bitmap> {
        private long mId;
        private String mPageUrl;
        private String mFaviconUrl;
        private OnFaviconLoadedListener mListener;
        private int mFlags;

        public LoadFaviconTask(Handler backgroundThreadHandler,
                               String pageUrl, String faviconUrl, int flags,
                               OnFaviconLoadedListener listener) {
            super(backgroundThreadHandler);

            synchronized(this) {
                mId = ++sNextFaviconLoadId;
            }

            mPageUrl = pageUrl;
            mFaviconUrl = faviconUrl;
            mListener = listener;
            mFlags = flags;
        }

        
        private Bitmap loadFaviconFromDb() {
            ContentResolver resolver = sContext.getContentResolver();
            return BrowserDB.getFaviconForUrl(resolver, mPageUrl);
        }

        
        private void saveFaviconToDb(final Bitmap favicon) {
            if ((mFlags & FLAG_PERSIST) == 0) {
                return;
            }

            ContentResolver resolver = sContext.getContentResolver();
            BrowserDB.updateFaviconForUrl(resolver, mPageUrl, favicon, mFaviconUrl);
        }

        
        private Bitmap downloadFavicon(URL faviconUrl) {
            if (mFaviconUrl.startsWith("jar:jar:")) {
                return GeckoJarReader.getBitmap(sContext.getResources(), mFaviconUrl);
            }

            URI uri;
            try {
                uri = faviconUrl.toURI();
            } catch (URISyntaxException e) {
                Log.e(LOGTAG, "URISyntaxException getting URI for favicon", e);
                return null;
            }

            
            String scheme = uri.getScheme();
            if (!"http".equals(scheme) && !"https".equals(scheme))
                return null;

            
            
            Bitmap image = null;
            try {
                HttpGet request = new HttpGet(faviconUrl.toURI());
                HttpResponse response = getHttpClient().execute(request);
                if (response == null)
                    return null;
                if (response.getStatusLine() != null) {
                    
                    int status = response.getStatusLine().getStatusCode();
                    if (status >= 400) {
                        putFaviconInFailedCache(mPageUrl, FAILED_EXPIRY_NEVER);
                        return null;
                    }
                }

                HttpEntity entity = response.getEntity();
                if (entity == null)
                    return null;
                if (entity.getContentType() != null) {
                    
                    String contentType = entity.getContentType().getValue();
                    if (!contentType.contains("image"))
                        return null;
                }

                BufferedHttpEntity bufferedEntity = new BufferedHttpEntity(entity);
                InputStream contentStream = bufferedEntity.getContent();
                image = BitmapUtils.decodeStream(contentStream);
                contentStream.close();
            } catch (IOException e) {
                Log.e(LOGTAG, "IOException reading favicon:", e);
            } catch (URISyntaxException e) {
                Log.e(LOGTAG, "URISyntaxException reading favicon:", e);
            }

            return image;
        }

        @Override
        protected Bitmap doInBackground(Void... unused) {
            Bitmap image = null;

            if (isCancelled())
                return null;

            URL faviconUrl = null;

            
            try {
                
                if (mFaviconUrl == null || mFaviconUrl.isEmpty()) {
                    
                    URL pageUrl = null;
                    pageUrl = new URL(mPageUrl);

                    faviconUrl = new URL(pageUrl.getProtocol(), pageUrl.getAuthority(), "/favicon.ico");
                    mFaviconUrl = faviconUrl.toString();
                } else {
                    faviconUrl = new URL(mFaviconUrl);
                }
            } catch (MalformedURLException e) {
                Log.d(LOGTAG, "The provided favicon URL is not valid");
                return null;
            }

            if (isCancelled())
                return null;

            String storedFaviconUrl = getFaviconUrlForPageUrl(mPageUrl);
            if (storedFaviconUrl != null && storedFaviconUrl.equals(mFaviconUrl)) {
                image = loadFaviconFromDb();
                if (image != null && image.getWidth() > 0 && image.getHeight() > 0)
                    return ((mFlags & FLAG_SCALE) != 0) ? scaleImage(image) : image;
            }

            if (isCancelled())
                return null;

            image = downloadFavicon(faviconUrl);

            if (image != null && image.getWidth() > 0 && image.getHeight() > 0) {
                saveFaviconToDb(image);
                image = ((mFlags & FLAG_SCALE) != 0) ? scaleImage(image) : image;
            } else {
                image = null;
            }

            return image;
        }

        @Override
        protected void onPostExecute(final Bitmap image) {
            sLoadTasks.remove(mId);
            dispatchResult(mPageUrl, image, mListener);
        }

        @Override
        protected void onCancelled() {
            sLoadTasks.remove(mId);

            
            
        }

        public long getId() {
            return mId;
        }
    }
}
