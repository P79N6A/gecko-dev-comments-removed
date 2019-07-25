




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.provider.Browser;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.InputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class Tab {

    private static final String LOG_NAME = "Tab";
    private int mId;
    private String mUrl;
    private String mTitle;
    private Drawable mFavicon;
    private String mSecurityMode;
    private Drawable mThumbnail;
    private List<HistoryEntry> mHistory;
    private int mHistoryIndex;
    private boolean mLoading;
    private boolean mBookmark;
    private DownloadFaviconTask mFaviconDownloader;

    static class HistoryEntry {
        public final String mUri;
        public String mTitle;

        public HistoryEntry(String uri, String title) {
            mUri = uri;
            mTitle = title;
        }
    }

    public Tab() {
        this(-1, "");
    }

    public Tab(int id, String url) {
        mId = id;
        mUrl = url;
        mTitle = new String();
        mFavicon = null;
        mSecurityMode = "unknown";
        mThumbnail = null;
        mHistory = new ArrayList<HistoryEntry>();
        mHistoryIndex = -1;
        mBookmark = false;
    }

    public int getId() {
        return mId;
    }

    public String getURL() {
        return mUrl;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getDisplayTitle() {
        if (mTitle != null && mTitle.length() > 0) {
            return mTitle;
        }

        return mUrl;
    }

    public Drawable getFavicon() {
        return mFavicon;
    }

    public String getSecurityMode() {
        return mSecurityMode;
    }

    public boolean isLoading() {
        return mLoading;
    }

    public boolean isBookmark() {
        return mBookmark;
    }

    public void updateURL(String url) {
        if (url != null && url.length() > 0) {
            mUrl = new String(url);
            Log.i(LOG_NAME, "Updated url: " + url + " for tab with id: " + mId);
            updateBookmark();
        }
    }

    public void updateTitle(String title) {
        if (title != null && title.length() > 0) {
            mTitle = new String(title);
        } else {
            mTitle = "";
        }

        Log.i(LOG_NAME, "Updated title: " + mTitle + " for tab with id: " + mId);

        final HistoryEntry he = getLastHistoryEntry();
        if (he != null) {
            he.mTitle = mTitle;
            GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        GlobalHistory.getInstance().update(he.mUri, he.mTitle);
                    }
                });
        } else {
            Log.e(LOG_NAME, "Requested title update on empty history stack");
        }
    }

    public void setLoading(boolean loading) {
        mLoading = loading;
    }

    private void setBookmark(boolean bookmark) {
        mBookmark = bookmark;
    }

    public HistoryEntry getLastHistoryEntry() {
        if (mHistory.isEmpty())
            return null;
        return mHistory.get(mHistoryIndex);
    }

    public void updateFavicon(Drawable favicon) {
        mFavicon = favicon;
        Log.i(LOG_NAME, "Updated favicon for tab with id: " + mId);
    }
 
    public void updateSecurityMode(String mode) {
        mSecurityMode = mode;
    }

    private void updateBookmark() {
        new CheckBookmarkTask().execute();
    }

    public void addBookmark() {
        new AddBookmarkTask().execute();
    }

    public void removeBookmark() {
        new RemoveBookmarkTask().execute();
    }

    public boolean doReload() {
        if (mHistory.isEmpty())
            return false;
        GeckoEvent e = new GeckoEvent("Session:Reload", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    public boolean doBack() {
        if (mHistoryIndex < 1) {
            return false;
        }
        GeckoEvent e = new GeckoEvent("Session:Back", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    public boolean canDoForward() {
        return (mHistoryIndex + 1 < mHistory.size());
    }

    public boolean doForward() {
        if (mHistoryIndex + 1 >= mHistory.size()) {
            return false;
        }
        GeckoEvent e = new GeckoEvent("Session:Forward", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    void handleSessionHistoryMessage(String event, JSONObject message) throws JSONException {
        if (event.equals("New")) {
            final String uri = message.getString("uri");
            mHistoryIndex++;
            while (mHistory.size() > mHistoryIndex) {
                mHistory.remove(mHistoryIndex);
            }
            HistoryEntry he = new HistoryEntry(uri, null);
            mHistory.add(he);
            GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        GlobalHistory.getInstance().add(uri);
                    }
                });
        } else if (event.equals("Back")) {
            if (mHistoryIndex - 1 < 0) {
                Log.e(LOG_NAME, "Received unexpected back notification");
                return;
            }
            mHistoryIndex--;
        } else if (event.equals("Forward")) {
            if (mHistoryIndex + 1 >= mHistory.size()) {
                Log.e(LOG_NAME, "Received unexpected forward notification");
                return;
            }
            mHistoryIndex++;
        } else if (event.equals("Goto")) {
            int index = message.getInt("index");
            if (index < 0 || index >= mHistory.size()) {
                Log.e(LOG_NAME, "Received unexpected history-goto notification");
                return;
            }
            mHistoryIndex = index;
        } else if (event.equals("Purge")) {
            mHistory.clear();
            mHistoryIndex = -1;
        }
    }

    void downloadFavicon(String url) {
        if (url == null) {
            try {
                URL urlObj = new URL(mUrl);
                url = urlObj.getProtocol() + "://" + urlObj.getAuthority() + "/favicon.ico";
            } catch (MalformedURLException e) {
                
                return;
            }
        }

        try {
            URL urlObj = new URL(url);
            
            
            if (mFaviconDownloader != null) {
                mFaviconDownloader.cancel(false);
                Log.d(LOG_NAME, "Cancelled old favicon downloader");
            }

            mFaviconDownloader = new DownloadFaviconTask();
            mFaviconDownloader.execute(urlObj);
        } catch (MalformedURLException e) {
        }
    }

    private class DownloadFaviconTask extends AsyncTask<URL, Void, Drawable> {
        protected Drawable doInBackground(URL... args) {
            Drawable image = null;
            try {
                URL url = args[0];
                InputStream is = (InputStream) url.getContent();
                image = Drawable.createFromStream(is, "src");
            } catch (IOException e) {
                Log.d(LOG_NAME, "Error loading favicon: " + e);
            }

            return image;
        }

        protected void onPostExecute(Drawable image) {
            if (image == null)
                return;

            updateFavicon(image);
            GeckoApp.mAppContext.faviconUpdated(Tab.this);
        }
    }

    private class CheckBookmarkTask extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... unused) {
            ContentResolver resolver = Tabs.getInstance().getContentResolver();
            Cursor cursor = resolver.query(Browser.BOOKMARKS_URI,
                                           null,
                                           Browser.BookmarkColumns.URL + " = ? and " + Browser.BookmarkColumns.BOOKMARK + " = ?",
                                           new String[] { getURL(), "1" },
                                           Browser.BookmarkColumns.URL);
            int count = cursor.getCount();
            cursor.close();
            if (count == 1)
                return true;
            else
                return false;
        }

        @Override
        protected void onPostExecute(Boolean isBookmark) {
            setBookmark(isBookmark.booleanValue());
        }
    }

    private class AddBookmarkTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... unused) {
            ContentResolver resolver = Tabs.getInstance().getContentResolver();
            Cursor cursor = resolver.query(Browser.BOOKMARKS_URI,
                                           null,
                                           Browser.BookmarkColumns.URL + " = ?",
                                           new String[] { getURL() },
                                           Browser.BookmarkColumns.URL);

            ContentValues values = new ContentValues();
            values.put(Browser.BookmarkColumns.BOOKMARK, "1");
            values.put(Browser.BookmarkColumns.TITLE, getTitle());

            if (cursor.getCount() == 1) {
                
                resolver.update(Browser.BOOKMARKS_URI,
                                values,
                                Browser.BookmarkColumns.URL + " = ?",
                                new String[] { getURL() });
            } else {
                
                values.put(Browser.BookmarkColumns.URL, mUrl);
                resolver.insert(Browser.BOOKMARKS_URI,
                                values);
            }

            cursor.close();

            return null;
        }

        @Override
        protected void onPostExecute(Void unused) {
            setBookmark(true);
        }
    }

    private class RemoveBookmarkTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... unused) {
            ContentResolver resolver = Tabs.getInstance().getContentResolver();
            ContentValues values = new ContentValues();
            values.put(Browser.BookmarkColumns.BOOKMARK, "0");
            resolver.update(Browser.BOOKMARKS_URI,
                            values,
                            Browser.BookmarkColumns.URL + " = ?",
                            new String[] { getURL() });
            return null;
        }

        @Override
        protected void onPostExecute(Void unused) {
            setBookmark(false);
        }
    }
} 
