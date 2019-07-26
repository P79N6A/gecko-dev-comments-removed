




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.util.GeckoJarReader;

import org.apache.http.HttpEntity;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.entity.BufferedHttpEntity;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.http.AndroidHttpClient;
import android.os.AsyncTask;
import android.support.v4.util.LruCache;
import android.util.Log;

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

    private Context mContext;

    private Map<Long,LoadFaviconTask> mLoadTasks;
    private long mNextFaviconLoadId;
    private LruCache<String, Bitmap> mFaviconsCache;
    private static final String USER_AGENT = GeckoApp.mAppContext.getDefaultUAString();
    private AndroidHttpClient mHttpClient;

    public interface OnFaviconLoadedListener {
        public void onFaviconLoaded(String url, Bitmap favicon);
    }

    public Favicons() {
        Log.d(LOGTAG, "Creating Favicons instance");

        mLoadTasks = Collections.synchronizedMap(new HashMap<Long,LoadFaviconTask>());
        mNextFaviconLoadId = 0;

        
        mFaviconsCache = new LruCache<String, Bitmap>(1024 * 1024) {
            @Override
            protected int sizeOf(String url, Bitmap image) {
                return image.getRowBytes() * image.getHeight();
            }
        };
    }

    private synchronized AndroidHttpClient getHttpClient() {
        if (mHttpClient != null)
            return mHttpClient;

        mHttpClient = AndroidHttpClient.newInstance(USER_AGENT);
        return mHttpClient;
    }

    private void dispatchResult(final String pageUrl, final Bitmap image,
            final OnFaviconLoadedListener listener) {
        if (pageUrl != null && image != null)
            putFaviconInMemCache(pageUrl, image);

        
        GeckoAppShell.getMainHandler().post(new Runnable() {
            public void run() {
                if (listener != null)
                    listener.onFaviconLoaded(pageUrl, image);
            }
        });
    }

    public String getFaviconUrlForPageUrl(String pageUrl) {
        return BrowserDB.getFaviconUrlForHistoryUrl(mContext.getContentResolver(), pageUrl);
    }

    public long loadFavicon(String pageUrl, String faviconUrl, boolean persist,
            OnFaviconLoadedListener listener) {

        
        if (pageUrl == null || pageUrl.length() == 0) {
            dispatchResult(null, null, listener);
            return -1;
        }

        
        Bitmap image = getFaviconFromMemCache(pageUrl);
        if (image != null) {
            dispatchResult(pageUrl, image, listener);
            return -1;
        }

        LoadFaviconTask task = new LoadFaviconTask(pageUrl, faviconUrl, persist, listener);

        long taskId = task.getId();
        mLoadTasks.put(taskId, task);

        task.execute();

        return taskId;
    }

    public Bitmap getFaviconFromMemCache(String pageUrl) {
        return mFaviconsCache.get(pageUrl);
    }

    public void putFaviconInMemCache(String pageUrl, Bitmap image) {
        mFaviconsCache.put(pageUrl, image);
    }

    public void clearMemCache() {
        mFaviconsCache.evictAll();
    }

    public boolean cancelFaviconLoad(long taskId) {
        Log.d(LOGTAG, "Requesting cancelation of favicon load (" + taskId + ")");

        boolean cancelled = false;
        synchronized (mLoadTasks) {
            if (!mLoadTasks.containsKey(taskId))
                return false;

            Log.d(LOGTAG, "Cancelling favicon load (" + taskId + ")");

            LoadFaviconTask task = mLoadTasks.get(taskId);
            cancelled = task.cancel(false);
        }
        return cancelled;
    }

    public void close() {
        Log.d(LOGTAG, "Closing Favicons database");

        
        synchronized (mLoadTasks) {
            Set<Long> taskIds = mLoadTasks.keySet();
            Iterator<Long> iter = taskIds.iterator();
            while (iter.hasNext()) {
                long taskId = iter.next();
                cancelFaviconLoad(taskId);
            }
        }
        if (mHttpClient != null)
            mHttpClient.close();
    }

    private static class FaviconsInstanceHolder {
        private static final Favicons INSTANCE = new Favicons();
    }

    public static Favicons getInstance() {
       return Favicons.FaviconsInstanceHolder.INSTANCE;
    }

    public void attachToContext(Context context) {
        mContext = context;
    }

    private class LoadFaviconTask extends AsyncTask<Void, Void, Bitmap> {
        private long mId;
        private String mPageUrl;
        private String mFaviconUrl;
        private OnFaviconLoadedListener mListener;
        private boolean mPersist;

        public LoadFaviconTask(String pageUrl, String faviconUrl, boolean persist,
                OnFaviconLoadedListener listener) {
            synchronized(this) {
                mId = ++mNextFaviconLoadId;
            }

            mPageUrl = pageUrl;
            mFaviconUrl = faviconUrl;
            mListener = listener;
            mPersist = persist;
        }

        
        private Bitmap loadFaviconFromDb() {
            ContentResolver resolver = mContext.getContentResolver();
            return BrowserDB.getFaviconForUrl(resolver, mPageUrl);
        }

        
        private void saveFaviconToDb(Bitmap favicon) {
            if (!mPersist) {
                return;
            }

            
            
            synchronized(Favicons.this) {
                ContentResolver resolver = mContext.getContentResolver();
                BrowserDB.updateFaviconForUrl(resolver, mPageUrl, favicon, mFaviconUrl);
            }
        }

        
        private Bitmap downloadFavicon(URL faviconUrl) {
            if (mFaviconUrl.startsWith("jar:jar:")) {
                BitmapDrawable d = GeckoJarReader.getBitmapDrawable(mContext.getResources(), mFaviconUrl);
                return d.getBitmap();
            }

            URI uri;
            try {
                uri = faviconUrl.toURI();
            } catch (URISyntaxException e) {
                Log.d(LOGTAG, "Could not get URI for favicon");
                return null;
            }

            
            String scheme = uri.getScheme();
            if (!"http".equals(scheme) && !"https".equals(scheme))
                return null;

            
            
            Bitmap image = null;
            try {
                HttpGet request = new HttpGet(faviconUrl.toURI());
                HttpEntity entity = getHttpClient().execute(request).getEntity();
                BufferedHttpEntity bufferedEntity = new BufferedHttpEntity(entity);
                InputStream contentStream = bufferedEntity.getContent();
                image = BitmapFactory.decodeStream(contentStream);
            } catch (Exception e) {
                Log.e(LOGTAG, "Error reading favicon", e);
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
                
                if (mFaviconUrl == null || mFaviconUrl.length() == 0) {
                    
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
                if (image != null)
                    return image;
            }

            if (isCancelled())
                return null;

            image = downloadFavicon(faviconUrl);

            if (image != null && image.getWidth() > 0 && image.getHeight() > 0) {
                saveFaviconToDb(image);
            } else {
                image = null;
            }

            return image;
        }

        @Override
        protected void onPostExecute(final Bitmap image) {
            mLoadTasks.remove(mId);
            dispatchResult(mPageUrl, image, mListener);
        }

        @Override
        protected void onCancelled() {
            mLoadTasks.remove(mId);

            
            
        }

        public long getId() {
            return mId;
        }
    }
}
