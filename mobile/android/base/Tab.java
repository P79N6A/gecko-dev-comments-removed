




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.drawable.Drawable;
import android.provider.Browser;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.InputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class Tab {
    private static final String LOGTAG = "GeckoTab";

    private int mId;
    private String mUrl;
    private String mTitle;
    private Drawable mFavicon;
    private String mFaviconUrl;
    private String mSecurityMode;
    private Drawable mThumbnail;
    private List<HistoryEntry> mHistory;
    private int mHistoryIndex;
    private boolean mLoading;
    private boolean mBookmark;
    private HashMap<String, DoorHanger> mDoorHangers;
    private long mFaviconLoadId;

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
        mTitle = "";
        mFavicon = null;
        mFaviconUrl = null;
        mSecurityMode = "unknown";
        mThumbnail = null;
        mHistory = new ArrayList<HistoryEntry>();
        mHistoryIndex = -1;
        mBookmark = false;
        mDoorHangers = new HashMap<String, DoorHanger>();
        mFaviconLoadId = 0;
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

    public String getFaviconURL() {
        return mFaviconUrl;
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
            mUrl = url;
            Log.i(LOGTAG, "Updated url: " + url + " for tab with id: " + mId);
            updateBookmark();
        }
    }

    public void updateTitle(String title) {
        mTitle = (title == null ? "" : title);

        Log.i(LOGTAG, "Updated title: " + mTitle + " for tab with id: " + mId);

        final HistoryEntry he = getLastHistoryEntry();
        if (he != null) {
            he.mTitle = mTitle;
            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    GlobalHistory.getInstance().update(he.mUri, he.mTitle);
                }
            });
        } else {
            Log.e(LOGTAG, "Requested title update on empty history stack");
        }
    }

    public void setLoading(boolean loading) {
        mLoading = loading;
    }

    private void setBookmark(boolean bookmark) {
        mBookmark = bookmark;
    }

    public void setFaviconLoadId(long faviconLoadId) {
        mFaviconLoadId = faviconLoadId;
    }

    public long getFaviconLoadId() {
        return mFaviconLoadId;
    }

    public HistoryEntry getLastHistoryEntry() {
        if (mHistory.isEmpty())
            return null;
        return mHistory.get(mHistoryIndex);
    }

    public void updateFavicon(Drawable favicon) {
        mFavicon = favicon;
        Log.i(LOGTAG, "Updated favicon for tab with id: " + mId);
    }

    public void updateFaviconURL(String faviconUrl) {
        mFaviconUrl = faviconUrl;
        Log.i(LOGTAG, "Updated favicon URL for tab with id: " + mId);
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

    public boolean doStop() {
        GeckoEvent e = new GeckoEvent("Session:Stop", "");
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

    public void addDoorHanger(String value, DoorHanger dh) {
        mDoorHangers.put(value, dh);
    } 

    public void removeDoorHanger(String value) {
        mDoorHangers.remove(value);
    }

    public void removeAllDoorHangers() {
        mDoorHangers = new HashMap<String, DoorHanger>();
    }

    public void removeTransientDoorHangers() {
        for (String value : mDoorHangers.keySet()) {
            DoorHanger dh = mDoorHangers.get(value);
            if (dh.shouldRemove())
                mDoorHangers.remove(value);
        }   
    }

    public DoorHanger getDoorHanger(String value) {
        if (mDoorHangers == null)
            return null;

        if (mDoorHangers.containsKey(value))
            return mDoorHangers.get(value);

        return null;
    } 

    public HashMap<String, DoorHanger> getDoorHangers() {
        return mDoorHangers;
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
                Log.e(LOGTAG, "Received unexpected back notification");
                return;
            }
            mHistoryIndex--;
        } else if (event.equals("Forward")) {
            if (mHistoryIndex + 1 >= mHistory.size()) {
                Log.e(LOGTAG, "Received unexpected forward notification");
                return;
            }
            mHistoryIndex++;
        } else if (event.equals("Goto")) {
            int index = message.getInt("index");
            if (index < 0 || index >= mHistory.size()) {
                Log.e(LOGTAG, "Received unexpected history-goto notification");
                return;
            }
            mHistoryIndex = index;
        } else if (event.equals("Purge")) {
            mHistory.clear();
            mHistoryIndex = -1;
        }
    }

    private class CheckBookmarkTask extends GeckoAsyncTask<Void, Void, Boolean> {
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

    private class AddBookmarkTask extends GeckoAsyncTask<Void, Void, Void> {
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

    private class RemoveBookmarkTask extends GeckoAsyncTask<Void, Void, Void> {
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
