





































package org.mozilla.gecko;

import java.util.Stack;

import android.content.ContentValues;
import android.content.Intent;
import android.database.sqlite.SQLiteDatabase;
import android.os.AsyncTask;
import android.util.Log;

class SessionHistory
{
    private final GeckoApp mApp;
    private final DatabaseHelper mDbHelper;
    private final Stack<HistoryEntry> mHistory;
    private SQLiteDatabase mDb;

    SessionHistory(GeckoApp app) {
        mApp = app;
        mDbHelper = new DatabaseHelper(app);
        mHistory = new Stack<HistoryEntry>();
    }

    void add(HistoryEntry entry) {
        new HistoryEntryTask().execute(entry);
    }

    void searchRequested(Intent searchIntent) {
        if (!mHistory.empty()) {
            searchIntent.putExtra(AwesomeBar.CURRENT_URL_KEY, mHistory.peek().mUri);
        }
    }

    boolean doReload() {
        if (mHistory.empty())
            return false;
        GeckoEvent e = new GeckoEvent("session-reload", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    boolean doBack() {
        if (mHistory.size() <= 1) {
            return false;
        }
        mHistory.pop();
        GeckoEvent e = new GeckoEvent("session-back", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    void cleanup() {
        if (mDb != null) {
            mDb.close();
        }
    }

    static class HistoryEntry {
        public final String mUri;
        public final String mTitle;

        public HistoryEntry(String uri, String title) {
            mUri = uri;
            mTitle = title;
        }
    }

    private class HistoryEntryTask extends AsyncTask<HistoryEntry, Void, Void> {
        protected Void doInBackground(HistoryEntry... entries) {
            HistoryEntry entry = entries[0];
            Log.d("GeckoApp", "adding uri=" + entry.mUri + ", title=" + entry.mTitle + " to history");
            ContentValues values = new ContentValues();
            values.put("url", entry.mUri);
            values.put("title", entry.mTitle);
            if (mHistory.empty() || !mHistory.peek().mUri.equals(entry.mUri))
                mHistory.push(entry);
            mDb = mDbHelper.getWritableDatabase();
            long id = mDb.insertWithOnConflict("moz_places", null, values, SQLiteDatabase.CONFLICT_REPLACE);
            values = new ContentValues();
            values.put("place_id", id);
            mDb.insertWithOnConflict("moz_historyvisits", null, values, SQLiteDatabase.CONFLICT_REPLACE);
            return null;
        }
    }
    HistoryEntry getHistoryEntryAt(int index) {
        if (index < mHistory.size())
            return mHistory.get(index);
        return null;
    }
}
