




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.provider.Browser;
import android.util.Log;

import java.util.Stack;

public class Tab {

    private static final String LOG_NAME = "Tab";
    private int mId;
    private String mUrl;
    private String mTitle;
    private Drawable mFavicon;
    private Drawable mThumbnail;
    private Stack<HistoryEntry> mHistory;
    private boolean mLoading;
    private boolean mBookmark;

    static class HistoryEntry {
        public final String mUri;
        public final String mTitle;

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
        mThumbnail = null;
        mHistory = new Stack<HistoryEntry>();
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

    public Drawable getFavicon() {
        return mFavicon;
    }

    public boolean isLoading() {
        return mLoading;
    }

    public boolean isBookmark() {
        return mBookmark;
    }

    public Stack<HistoryEntry> getHistory() {
        return mHistory;
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
            Log.i(LOG_NAME, "Updated title: " + title + " for tab with id: " + mId);
        }
    }

    public void setLoading(boolean loading) {
        mLoading = loading;
    }

    private void setBookmark(boolean bookmark) {
        mBookmark = bookmark;
    }

    public void addHistory(HistoryEntry entry) {
        if (mHistory.empty() || !mHistory.peek().mUri.equals(entry.mUri)) {
            mHistory.push(entry);
            new HistoryEntryTask().execute(entry);
        }
    }

    public HistoryEntry getLastHistoryEntry() {
        if (mHistory.empty())
            return null;
        return mHistory.peek();
    }

    public void updateFavicon(Drawable favicon) {
        mFavicon = favicon;
        Log.i(LOG_NAME, "Updated favicon for tab with id: " + mId);
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
        if (mHistory.empty())
            return false;
        GeckoEvent e = new GeckoEvent("session-reload", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    public boolean doBack() {
        if (mHistory.size() <= 1) {
            return false;
        }
        mHistory.pop();
        GeckoEvent e = new GeckoEvent("session-back", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    private class HistoryEntryTask extends AsyncTask<HistoryEntry, Void, Void> {
        protected Void doInBackground(HistoryEntry... entries) {
            HistoryEntry entry = entries[0];
            GlobalHistory.getInstance().add(entry.mUri);
            return null;
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
            if (cursor.getCount() == 1)
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
