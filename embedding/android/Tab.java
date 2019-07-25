




































package org.mozilla.gecko;

import java.util.*;

import android.content.*;
import android.database.sqlite.SQLiteDatabase;
import android.os.AsyncTask;
import android.graphics.drawable.*;
import android.util.Log;
import android.provider.Browser;

public class Tab {

    private static final String LOG_FILE_NAME = "Tab";
    private int id;
    private String url, title;
    private Drawable favicon, thumbnail;
    private Stack<HistoryEntry> history;
    private boolean loading;

    static class HistoryEntry {
        public final String mUri;
        public final String mTitle;

        public HistoryEntry(String uri, String title) {
            mUri = uri;
            mTitle = title;
        }
    }

    public Tab() {
        this.id = -1;
        this.url = new String();
        this.title = new String();
        this.favicon = null;
        this.thumbnail = null;
        this.history = new Stack<HistoryEntry>();
    }

    public Tab(int id, String url) {
        this.id = id;
        this.url = new String(url);
        this.title = new String();
        this.favicon = null;
        this.thumbnail = null;
        this.history = new Stack<HistoryEntry>();
    }

    public int getId() {
        return id;
    }

    public String getURL() {
        return url;
    }

    public String getTitle() {
        return title;
    }

    public boolean isLoading() {
        return loading;
    }

    public Stack<HistoryEntry> getHistory() {
        return history;
    }

    public void updateURL(String url) {

        if(url != null && url.length() > 0) {
            this.url = new String(url);
            Log.i(LOG_FILE_NAME, "Updated url: " + url + " for tab with id: " + this.id);
        }
    }

    public void updateTitle(String title) {
        if(title != null && title.length() > 0) {
            this.title = new String(title);
            Log.i(LOG_FILE_NAME, "Updated title: " + title + " for tab with id: " + this.id);
        }
    }

    public void setLoading(boolean loading) {
        this.loading = loading;
    }

    public void addHistory(HistoryEntry entry) {
       if (history.empty() || !history.peek().mUri.equals(entry.mUri)) {
           history.push(entry);
           new HistoryEntryTask().execute(entry);
       }
    }

    public HistoryEntry getLastHistoryEntry() {
       if (history.empty())
           return null;
       return history.peek();
    }

    public void updateFavicon(Drawable favicon) {
        if (favicon != null) {
            this.favicon = favicon;
            Log.i(LOG_FILE_NAME, "Updated favicon for tab with id: " + this.id);
        }
    }

    public boolean doReload() {
        if (history.empty())
            return false;
        GeckoEvent e = new GeckoEvent("session-reload", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    public boolean doBack() {
        if (history.size() <= 1) {
            return false;
        }
        history.pop();
        GeckoEvent e = new GeckoEvent("session-back", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    private class HistoryEntryTask extends AsyncTask<HistoryEntry, Void, Void> {
        protected Void doInBackground(HistoryEntry... entries) {
            HistoryEntry entry = entries[0];
            Browser.updateVisitedHistory(GeckoApp.mAppContext.getContentResolver(), entry.mUri, true);
            return null;
        }
    }
} 
