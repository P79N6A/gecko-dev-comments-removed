




































package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.drawable.BitmapDrawable;

public class BrowserDB {
    public static String ABOUT_PAGES_URL_FILTER = "about:%";

    public static interface URLColumns {
        public static String URL = "url";
        public static String TITLE = "title";
        public static String FAVICON = "favicon";
        public static String THUMBNAIL = "thumbnail";
        public static String DATE_LAST_VISITED = "date-last-visited";
        public static String VISITS = "visits";
        public static String KEYWORD = "keyword";
    }

    private static BrowserDBIface sDb;

    public interface BrowserDBIface {
        public Cursor filter(ContentResolver cr, CharSequence constraint, int limit);

        public Cursor getTopSites(ContentResolver cr, int limit);

        public void updateVisitedHistory(ContentResolver cr, String uri);

        public void updateHistoryTitle(ContentResolver cr, String uri, String title);

        public void updateHistoryEntry(ContentResolver cr, String uri, String title,
                                       long date, int visits);

        public Cursor getAllVisitedHistory(ContentResolver cr);

        public Cursor getRecentHistory(ContentResolver cr, int limit);

        public int getMaxHistoryCount();

        public void clearHistory(ContentResolver cr);

        public Cursor getAllBookmarks(ContentResolver cr);

        public Cursor getMobileBookmarks(ContentResolver cr);

        public Cursor getDesktopBookmarks(ContentResolver cr);

        public boolean isBookmark(ContentResolver cr, String uri);

        public String getUrlForKeyword(ContentResolver cr, String keyword);

        public void addBookmark(ContentResolver cr, String title, String uri);

        public void removeBookmark(ContentResolver cr, int id);

        public void removeBookmarksWithURL(ContentResolver cr, String uri);

        public void updateBookmark(ContentResolver cr, String oldUri, String uri, String title, String keyword);

        public BitmapDrawable getFaviconForUrl(ContentResolver cr, String uri);

        public void updateFaviconForUrl(ContentResolver cr, String uri, BitmapDrawable favicon);

        public void updateThumbnailForUrl(ContentResolver cr, String uri, BitmapDrawable thumbnail);

        public byte[] getThumbnailForUrl(ContentResolver cr, String uri);

        public void registerBookmarkObserver(ContentResolver cr, ContentObserver observer);
    }

    static {
        
        sDb = new LocalBrowserDB(BrowserContract.DEFAULT_PROFILE);
    }

    public static Cursor filter(ContentResolver cr, CharSequence constraint, int limit) {
        return sDb.filter(cr, constraint, limit);
    }

    public static Cursor getTopSites(ContentResolver cr, int limit) {
        return sDb.getTopSites(cr, limit);
    }

    public static void updateVisitedHistory(ContentResolver cr, String uri) {
        sDb.updateVisitedHistory(cr, uri);
    }

    public static void updateHistoryTitle(ContentResolver cr, String uri, String title) {
        sDb.updateHistoryTitle(cr, uri, title);
    }

    public static void updateHistoryEntry(ContentResolver cr, String uri, String title,
                                          long date, int visits) {
        sDb.updateHistoryEntry(cr, uri, title, date, visits);
    }

    public static Cursor getAllVisitedHistory(ContentResolver cr) {
        return sDb.getAllVisitedHistory(cr);
    }

    public static Cursor getRecentHistory(ContentResolver cr, int limit) {
        return sDb.getRecentHistory(cr, limit);
    }

    public static int getMaxHistoryCount() {
        return sDb.getMaxHistoryCount();
    }

    public static void clearHistory(ContentResolver cr) {
        sDb.clearHistory(cr);
    }

    public static Cursor getAllBookmarks(ContentResolver cr) {
        return sDb.getAllBookmarks(cr);
    }

    public static Cursor getMobileBookmarks(ContentResolver cr) {
        return sDb.getMobileBookmarks(cr);
    }

    public static Cursor getDesktopBookmarks(ContentResolver cr) {
        return sDb.getDesktopBookmarks(cr);
    }

    public static String getUrlForKeyword(ContentResolver cr, String keyword) {
        return sDb.getUrlForKeyword(cr, keyword);
    }
    
    public static boolean isBookmark(ContentResolver cr, String uri) {
        return sDb.isBookmark(cr, uri);
    }

    public static void addBookmark(ContentResolver cr, String title, String uri) {
        sDb.addBookmark(cr, title, uri);
    }

    public static void removeBookmark(ContentResolver cr, int id) {
        sDb.removeBookmark(cr, id);
    }

    public static void removeBookmarksWithURL(ContentResolver cr, String uri) {
        sDb.removeBookmarksWithURL(cr, uri);
    }

    public static void updateBookmark(ContentResolver cr, String oldUri, String uri, String title, String keyword) {
        sDb.updateBookmark(cr, oldUri, uri, title, keyword);
    }

    public static BitmapDrawable getFaviconForUrl(ContentResolver cr, String uri) {
        return sDb.getFaviconForUrl(cr, uri);
    }

    public static void updateFaviconForUrl(ContentResolver cr, String uri, BitmapDrawable favicon) {
        sDb.updateFaviconForUrl(cr, uri, favicon);
    }

    public static void updateThumbnailForUrl(ContentResolver cr, String uri, BitmapDrawable thumbnail) {
        sDb.updateThumbnailForUrl(cr, uri, thumbnail);
    }

    public static byte[] getThumbnailForUrl(ContentResolver cr, String uri) {
        return sDb.getThumbnailForUrl(cr, uri);
    }

    public static void registerBookmarkObserver(ContentResolver cr, ContentObserver observer) {
        sDb.registerBookmarkObserver(cr, observer);
    }

    public static void unregisterBookmarkObserver(ContentResolver cr, ContentObserver observer) {
        cr.unregisterContentObserver(observer);
    }
}
