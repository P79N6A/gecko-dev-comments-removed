




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.util.Log;

import java.io.InputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.mozilla.gecko.db.BrowserDB;

import org.apache.http.HttpEntity;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.entity.BufferedHttpEntity;
import org.apache.http.impl.client.DefaultHttpClient;

public class Favicons {
    private static final String LOGTAG = "GeckoFavicons";

    public static final long NOT_LOADING = 0;

    private Context mContext;
    private DatabaseHelper mDbHelper;

    private Map<Long,LoadFaviconTask> mLoadTasks;
    private long mNextFaviconLoadId;

    public interface OnFaviconLoadedListener {
        public void onFaviconLoaded(String url, Drawable favicon);
    }

    private class DatabaseHelper extends SQLiteOpenHelper {
        private static final String DATABASE_NAME = "favicon_urls.db";
        private static final String TABLE_NAME = "favicon_urls";
        private static final int DATABASE_VERSION = 1;

        private static final String COLUMN_ID = "_id";
        private static final String COLUMN_FAVICON_URL = "favicon_url";
        private static final String COLUMN_PAGE_URL = "page_url";

        DatabaseHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
            Log.d(LOGTAG, "Creating DatabaseHelper");
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            Log.d(LOGTAG, "Creating database for favicon URLs");

            db.execSQL("CREATE TABLE " + TABLE_NAME + " (" +
                       COLUMN_ID + " INTEGER PRIMARY KEY," +
                       COLUMN_FAVICON_URL + " TEXT NOT NULL," +
                       COLUMN_PAGE_URL + " TEXT UNIQUE NOT NULL" +
                       ");");
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.w(LOGTAG, "Upgrading favicon URLs database from version " +
                  oldVersion + " to " + newVersion + ", which will destroy all old data");

            
            db.execSQL("DROP TABLE IF EXISTS " + TABLE_NAME);

            
            onCreate(db);
        }

        public String getFaviconUrlForPageUrl(String pageUrl) {
            Log.d(LOGTAG, "Calling getFaviconUrlForPageUrl() for " + pageUrl);

            SQLiteDatabase db = mDbHelper.getReadableDatabase();

            SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
            qb.setTables(TABLE_NAME);

            Cursor c = qb.query(
                db,
                new String[] { COLUMN_FAVICON_URL },
                COLUMN_PAGE_URL + " = ?",
                new String[] { pageUrl },
                null, null, null
            );

            if (!c.moveToFirst()) {
                c.close();
                return null;
            }

            String url = c.getString(c.getColumnIndexOrThrow(COLUMN_FAVICON_URL));
            c.close();
            return url;
        }

        public void setFaviconUrlForPageUrl(String pageUrl, String faviconUrl) {
            Log.d(LOGTAG, "Calling setFaviconUrlForPageUrl() for " + pageUrl);

            SQLiteDatabase db = mDbHelper.getWritableDatabase();

            ContentValues values = new ContentValues();
            values.put(COLUMN_FAVICON_URL, faviconUrl);
            values.put(COLUMN_PAGE_URL, pageUrl);

            db.replace(TABLE_NAME, null, values);
        }


        public void clearFavicons() {
            SQLiteDatabase db = mDbHelper.getWritableDatabase();
            db.delete(TABLE_NAME, null, null);
        }
    }

    public Favicons(Context context) {
        Log.d(LOGTAG, "Creating Favicons instance");

        mContext = context;
        mDbHelper = new DatabaseHelper(context);

        mLoadTasks = new HashMap<Long,LoadFaviconTask>();
        mNextFaviconLoadId = 0;
    }

    public long loadFavicon(String pageUrl, String faviconUrl,
            OnFaviconLoadedListener listener) {

        
        if (pageUrl == null || pageUrl.length() == 0) {
            if (listener != null)
                listener.onFaviconLoaded(null, null);
        }

        LoadFaviconTask task = new LoadFaviconTask(pageUrl, faviconUrl, listener);

        long taskId = task.getId();
        mLoadTasks.put(taskId, task);

        task.execute();

        Log.d(LOGTAG, "Calling loadFavicon() with URL = " + pageUrl +
                        " and favicon URL = " + faviconUrl +
                        " (" + taskId + ")");

        return taskId;
    }

    public boolean cancelFaviconLoad(long taskId) {
        Log.d(LOGTAG, "Requesting cancelation of favicon load (" + taskId + ")");

        if (!mLoadTasks.containsKey(taskId))
            return false;

        Log.d(LOGTAG, "Cancelling favicon load (" + taskId + ")");

        LoadFaviconTask task = mLoadTasks.get(taskId);
        return task.cancel(false);
    }

    public void clearFavicons() {
        mDbHelper.clearFavicons();
    }

    public void close() {
        Log.d(LOGTAG, "Closing Favicons database");
        mDbHelper.close();

        
        Set<Long> taskIds = mLoadTasks.keySet();
        Iterator<Long> iter = taskIds.iterator();
        while (iter.hasNext()) {
            long taskId = iter.next();
            cancelFaviconLoad(taskId);
        }
    }

    private class LoadFaviconTask extends AsyncTask<Void, Void, BitmapDrawable> {
        private long mId;
        private String mPageUrl;
        private String mFaviconUrl;
        private OnFaviconLoadedListener mListener;

        public LoadFaviconTask(String pageUrl, String faviconUrl, OnFaviconLoadedListener listener) {
            synchronized(this) {
                mId = ++mNextFaviconLoadId;
            }

            mPageUrl = pageUrl;
            mFaviconUrl = faviconUrl;
            mListener = listener;

            Log.d(LOGTAG, "Creating LoadFaviconTask with URL = " + pageUrl +
                          " and favicon URL = " + faviconUrl);
        }

        
        private BitmapDrawable loadFaviconFromDb() {
            Log.d(LOGTAG, "Loading favicon from DB for URL = " + mPageUrl);

            ContentResolver resolver = mContext.getContentResolver();
            BitmapDrawable favicon = BrowserDB.getFaviconForUrl(resolver, mPageUrl);

            if (favicon != null)
                Log.d(LOGTAG, "Loaded favicon from DB successfully for URL = " + mPageUrl);

            return favicon;
        }

        
        private void saveFaviconToDb(BitmapDrawable favicon) {
            
            
            synchronized(mDbHelper) {
                Log.d(LOGTAG, "Saving favicon on browser database for URL = " + mPageUrl);
                ContentResolver resolver = mContext.getContentResolver();
                BrowserDB.updateFaviconForUrl(resolver, mPageUrl, favicon);

                Log.d(LOGTAG, "Saving favicon URL for URL = " + mPageUrl);
                mDbHelper.setFaviconUrlForPageUrl(mPageUrl, mFaviconUrl);
            }
        }

        
        private BitmapDrawable downloadFavicon(URL faviconUrl) {
            Log.d(LOGTAG, "Downloading favicon for URL = " + mPageUrl +
                          " with favicon URL = " + mFaviconUrl);

            if (mFaviconUrl.startsWith("jar:jar:")) {
                return GeckoJarReader.getBitmapDrawable(mFaviconUrl);
            }

            
            
            BitmapDrawable image = null;
            InputStream contentStream = null;
            try {
                HttpGet request = new HttpGet(faviconUrl.toURI());
                HttpEntity entity = new DefaultHttpClient().execute(request).getEntity();
                BufferedHttpEntity bufferedEntity = new BufferedHttpEntity(entity);
                contentStream = bufferedEntity.getContent();
                image = (BitmapDrawable) Drawable.createFromStream(contentStream, "src");
            } catch (IOException e) {
                
            } catch (Exception e) {
                Log.e(LOGTAG, "Error reading favicon", e);
            } finally {
                try {
                    if (contentStream != null)
                        contentStream.close();
                } catch (IOException e) {
                    Log.d(LOGTAG, "error closing favicon stream");
                }
            }

            return image;
        }

        @Override
        protected BitmapDrawable doInBackground(Void... unused) {
            BitmapDrawable image = null;

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
                Log.d(LOGTAG, "The provided favicon URL is not valid: " + e);
                return null;
            }

            Log.d(LOGTAG, "Favicon URL is now: " + mFaviconUrl);

            if (isCancelled())
                return null;

            String storedFaviconUrl = mDbHelper.getFaviconUrlForPageUrl(mPageUrl);
            if (storedFaviconUrl != null && storedFaviconUrl.equals(mFaviconUrl)) {
                image = loadFaviconFromDb();

                if (isCancelled())
                    return null;

                
                
                if (image == null) {
                    image = downloadFavicon(faviconUrl);
                }
            } else {
                image = downloadFavicon(faviconUrl);
            }

            if (image != null) {
                Log.d(LOGTAG, "Downloaded favicon successfully for URL = " + mPageUrl);
                saveFaviconToDb(image);
            }

            return image;
        }

        @Override
        protected void onPostExecute(final BitmapDrawable image) {
            Log.d(LOGTAG, "LoadFaviconTask finished for URL = " + mPageUrl +
                          " (" + mId + ")");

            mLoadTasks.remove(mId);

            if (mListener != null) {
                
                GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                    public void run() {
                        mListener.onFaviconLoaded(mPageUrl, image);
                    }
                });
            }
        }

        @Override
        protected void onCancelled() {
            Log.d(LOGTAG, "LoadFaviconTask cancelled for URL = " + mPageUrl +
                          " (" + mId + ")");

            mLoadTasks.remove(mId);

            
            
        }

        public long getId() {
            return mId;
        }
    }
}
