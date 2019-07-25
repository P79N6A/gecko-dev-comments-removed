




































package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.database.Cursor;
import android.graphics.drawable.BitmapDrawable;

public class BrowserDB {
    private static final String LOGTAG = "GeckoBrowserDB";

    public static interface URLColumns {
        public static String URL = "url";
        public static String TITLE = "title";
        public static String FAVICON = "favicon";
        public static String THUMBNAIL = "thumbnail";
        public static String DATE_LAST_VISITED = "date-last-visited";
    }

    private static BrowserDBIface sDb;

    public interface BrowserDBIface {
        public Cursor filter(ContentResolver cr, CharSequence constraint, int limit);

        public void updateVisitedHistory(ContentResolver cr, String uri);

        public void updateHistoryTitle(ContentResolver cr, String uri, String title);

        public Cursor getAllVisitedHistory(ContentResolver cr);

        public Cursor getRecentHistory(ContentResolver cr, int limit);

        public void clearHistory(ContentResolver cr);

        public Cursor getAllBookmarks(ContentResolver cr);

        public boolean isBookmark(ContentResolver cr, String uri);

        public void addBookmark(ContentResolver cr, String title, String uri);

        public void removeBookmark(ContentResolver cr, String uri);

        public BitmapDrawable getFaviconForUrl(ContentResolver cr, String uri);

        public void updateFaviconForUrl(ContentResolver cr, String uri, BitmapDrawable favicon);

        public void updateThumbnailForUrl(ContentResolver cr, String uri, BitmapDrawable thumbnail);
    }

    static {
        
        
        sDb = new AndroidBrowserDB();
    }

    public static Cursor filter(ContentResolver cr, CharSequence constraint, int limit) {
        return sDb.filter(cr, constraint, limit);
    }

    public static void updateVisitedHistory(ContentResolver cr, String uri) {
        sDb.updateVisitedHistory(cr, uri);
    }

    public static void updateHistoryTitle(ContentResolver cr, String uri, String title) {
        sDb.updateHistoryTitle(cr, uri, title);
    }

    public static Cursor getAllVisitedHistory(ContentResolver cr) {
        return sDb.getAllVisitedHistory(cr);
    }

    public static Cursor getRecentHistory(ContentResolver cr, int limit) {
        return sDb.getRecentHistory(cr, limit);
    }

    public static void clearHistory(ContentResolver cr) {
        sDb.clearHistory(cr);
    }

    public static Cursor getAllBookmarks(ContentResolver cr) {
        return sDb.getAllBookmarks(cr);
    }

    public static boolean isBookmark(ContentResolver cr, String uri) {
        return sDb.isBookmark(cr, uri);
    }

    public static void addBookmark(ContentResolver cr, String title, String uri) {
        sDb.addBookmark(cr, title, uri);
    }

    public static void removeBookmark(ContentResolver cr, String uri) {
        sDb.removeBookmark(cr, uri);
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
}
