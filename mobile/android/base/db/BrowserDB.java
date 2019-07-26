




package org.mozilla.gecko.db;

import java.util.List;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.ExpirePriority;
import org.mozilla.gecko.favicons.decoders.LoadFaviconResult;
import org.mozilla.gecko.mozglue.RobocopTarget;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.ContentObserver;
import android.database.Cursor;
import android.database.CursorWrapper;
import android.graphics.drawable.BitmapDrawable;
import android.util.SparseArray;

public class BrowserDB {
    private static boolean sAreContentProvidersEnabled = true;

    public static interface URLColumns {
        public static String URL = "url";
        public static String TITLE = "title";
        public static String FAVICON = "favicon";
        public static String THUMBNAIL = "thumbnail";
        public static String DATE_LAST_VISITED = "date-last-visited";
        public static String VISITS = "visits";
        public static String KEYWORD = "keyword";
    }

    private static BrowserDBIface sDb = null;

    public interface BrowserDBIface {
        public void invalidateCachedState();

        @RobocopTarget
        public Cursor filter(ContentResolver cr, CharSequence constraint, int limit);

        
        
        public Cursor getTopSites(ContentResolver cr, int limit);

        public void updateVisitedHistory(ContentResolver cr, String uri);

        public void updateHistoryTitle(ContentResolver cr, String uri, String title);

        public void updateHistoryEntry(ContentResolver cr, String uri, String title,
                                       long date, int visits);

        @RobocopTarget
        public Cursor getAllVisitedHistory(ContentResolver cr);

        public Cursor getRecentHistory(ContentResolver cr, int limit);

        public void expireHistory(ContentResolver cr, ExpirePriority priority);

        public void removeHistoryEntry(ContentResolver cr, int id);

        @RobocopTarget
        public void removeHistoryEntry(ContentResolver cr, String url);

        public void clearHistory(ContentResolver cr);

        @RobocopTarget
        public Cursor getBookmarksInFolder(ContentResolver cr, long folderId);

        public Cursor getReadingList(ContentResolver cr);

        public boolean isVisited(ContentResolver cr, String uri);

        public int getReadingListCount(ContentResolver cr);

        @RobocopTarget
        public boolean isBookmark(ContentResolver cr, String uri);

        public boolean isReadingListItem(ContentResolver cr, String uri);

        



        public int getItemFlags(ContentResolver cr, String uri);

        public String getUrlForKeyword(ContentResolver cr, String keyword);

        @RobocopTarget
        public void addBookmark(ContentResolver cr, String title, String uri);

        public void removeBookmark(ContentResolver cr, int id);

        @RobocopTarget
        public void removeBookmarksWithURL(ContentResolver cr, String uri);

        @RobocopTarget
        public void updateBookmark(ContentResolver cr, int id, String uri, String title, String keyword);

        public void addReadingListItem(ContentResolver cr, ContentValues values);

        public void removeReadingListItemWithURL(ContentResolver cr, String uri);

        public void removeReadingListItem(ContentResolver cr, int id);

        public LoadFaviconResult getFaviconForUrl(ContentResolver cr, String uri);

        public String getFaviconUrlForHistoryUrl(ContentResolver cr, String url);

        public void updateFaviconForUrl(ContentResolver cr, String pageUri, byte[] encodedFavicon, String faviconUri);

        public void updateThumbnailForUrl(ContentResolver cr, String uri, BitmapDrawable thumbnail);

        @RobocopTarget
        public byte[] getThumbnailForUrl(ContentResolver cr, String uri);

        public Cursor getThumbnailsForUrls(ContentResolver cr, List<String> urls);

        @RobocopTarget
        public void removeThumbnails(ContentResolver cr);

        public void registerBookmarkObserver(ContentResolver cr, ContentObserver observer);

        public void registerHistoryObserver(ContentResolver cr, ContentObserver observer);

        public int getCount(ContentResolver cr, String database);

        public void pinSite(ContentResolver cr, String url, String title, int position);

        public void unpinSite(ContentResolver cr, int position);

        public void unpinAllSites(ContentResolver cr);

        public Cursor getPinnedSites(ContentResolver cr, int limit);

        @RobocopTarget
        public Cursor getBookmarkForUrl(ContentResolver cr, String url);
    }

    static {
        
        sDb = null;
    }

    public static void initialize(String profile) {
        sDb = new LocalBrowserDB(profile);
    }

    public static void invalidateCachedState() {
        sDb.invalidateCachedState();
    }

    @RobocopTarget
    public static Cursor filter(ContentResolver cr, CharSequence constraint, int limit) {
        return sDb.filter(cr, constraint, limit);
    }

    public static Cursor getTopSites(ContentResolver cr, int minLimit, int maxLimit) {
        
        
        Cursor pinnedSites = sDb.getPinnedSites(cr, minLimit);
        Cursor topSites = sDb.getTopSites(cr, maxLimit - pinnedSites.getCount());
        return new TopSitesCursorWrapper(pinnedSites, topSites, minLimit);
    }

    public static void updateVisitedHistory(ContentResolver cr, String uri) {
        if (sAreContentProvidersEnabled) {
            sDb.updateVisitedHistory(cr, uri);
        }
    }

    public static void updateHistoryTitle(ContentResolver cr, String uri, String title) {
        if (sAreContentProvidersEnabled) {
            sDb.updateHistoryTitle(cr, uri, title);
        }
    }

    public static void updateHistoryEntry(ContentResolver cr, String uri, String title,
                                          long date, int visits) {
        if (sAreContentProvidersEnabled) {
            sDb.updateHistoryEntry(cr, uri, title, date, visits);
        }
    }

    @RobocopTarget
    public static Cursor getAllVisitedHistory(ContentResolver cr) {
        return (sAreContentProvidersEnabled ? sDb.getAllVisitedHistory(cr) : null);
    }

    public static Cursor getRecentHistory(ContentResolver cr, int limit) {
        return sDb.getRecentHistory(cr, limit);
    }

    public static void expireHistory(ContentResolver cr, ExpirePriority priority) {
        if (sDb == null)
            return;

        if (priority == null)
            priority = ExpirePriority.NORMAL;
        sDb.expireHistory(cr, priority);
    }

    public static void removeHistoryEntry(ContentResolver cr, int id) {
        sDb.removeHistoryEntry(cr, id);
    }

    @RobocopTarget
    public static void removeHistoryEntry(ContentResolver cr, String url) {
        sDb.removeHistoryEntry(cr, url);
    }

    @RobocopTarget
    public static void clearHistory(ContentResolver cr) {
        sDb.clearHistory(cr);
    }

    @RobocopTarget
    public static Cursor getBookmarksInFolder(ContentResolver cr, long folderId) {
        return sDb.getBookmarksInFolder(cr, folderId);
    }

    @RobocopTarget
    public static Cursor getReadingList(ContentResolver cr) {
        return sDb.getReadingList(cr);
    }

    public static String getUrlForKeyword(ContentResolver cr, String keyword) {
        return sDb.getUrlForKeyword(cr, keyword);
    }

    public static boolean isVisited(ContentResolver cr, String uri) {
        return sDb.isVisited(cr, uri);
    }

    public static int getReadingListCount(ContentResolver cr) {
        return sDb.getReadingListCount(cr);
    }

    @RobocopTarget
    public static boolean isBookmark(ContentResolver cr, String uri) {
        return (sAreContentProvidersEnabled && sDb.isBookmark(cr, uri));
    }

    public static boolean isReadingListItem(ContentResolver cr, String uri) {
        return (sAreContentProvidersEnabled && sDb.isReadingListItem(cr, uri));
    }

    public static int getItemFlags(ContentResolver cr, String uri) {
        if (!sAreContentProvidersEnabled) {
            return 0;
        }
        return sDb.getItemFlags(cr, uri);
    }

    public static void addBookmark(ContentResolver cr, String title, String uri) {
        sDb.addBookmark(cr, title, uri);
    }

    public static void removeBookmark(ContentResolver cr, int id) {
        sDb.removeBookmark(cr, id);
    }

    @RobocopTarget
    public static void removeBookmarksWithURL(ContentResolver cr, String uri) {
        sDb.removeBookmarksWithURL(cr, uri);
    }

    @RobocopTarget
    public static void updateBookmark(ContentResolver cr, int id, String uri, String title, String keyword) {
        sDb.updateBookmark(cr, id, uri, title, keyword);
    }

    public static void addReadingListItem(ContentResolver cr, ContentValues values) {
        sDb.addReadingListItem(cr, values);
    }

    public static void removeReadingListItemWithURL(ContentResolver cr, String uri) {
        sDb.removeReadingListItemWithURL(cr, uri);
    }

    public static void removeReadingListItem(ContentResolver cr, int id) {
        sDb.removeReadingListItem(cr, id);
    }

    public static LoadFaviconResult getFaviconForFaviconUrl(ContentResolver cr, String faviconURL) {
        return sDb.getFaviconForUrl(cr, faviconURL);
    }

    public static String getFaviconUrlForHistoryUrl(ContentResolver cr, String url) {
        return sDb.getFaviconUrlForHistoryUrl(cr, url);
    }

    public static void updateFaviconForUrl(ContentResolver cr, String pageUri, byte[] encodedFavicon, String faviconUri) {
        sDb.updateFaviconForUrl(cr, pageUri, encodedFavicon, faviconUri);
    }

    public static void updateThumbnailForUrl(ContentResolver cr, String uri, BitmapDrawable thumbnail) {
        sDb.updateThumbnailForUrl(cr, uri, thumbnail);
    }

    @RobocopTarget
    public static byte[] getThumbnailForUrl(ContentResolver cr, String uri) {
        return sDb.getThumbnailForUrl(cr, uri);
    }

    public static Cursor getThumbnailsForUrls(ContentResolver cr, List<String> urls) {
        return sDb.getThumbnailsForUrls(cr, urls);
    }

    @RobocopTarget
    public static void removeThumbnails(ContentResolver cr) {
        sDb.removeThumbnails(cr);
    }

    public static void registerBookmarkObserver(ContentResolver cr, ContentObserver observer) {
        sDb.registerBookmarkObserver(cr, observer);
    }

    public static void registerHistoryObserver(ContentResolver cr, ContentObserver observer) {
        sDb.registerHistoryObserver(cr, observer);
    }

    public static void unregisterContentObserver(ContentResolver cr, ContentObserver observer) {
        cr.unregisterContentObserver(observer);
    }

    public static int getCount(ContentResolver cr, String database) {
        return sDb.getCount(cr, database);
    }

    public static void pinSite(ContentResolver cr, String url, String title, int position) {
        sDb.pinSite(cr, url, title, position);
    }

    public static void unpinSite(ContentResolver cr, int position) {
        sDb.unpinSite(cr, position);
    }

    public static void unpinAllSites(ContentResolver cr) {
        sDb.unpinAllSites(cr);
    }

    public static Cursor getPinnedSites(ContentResolver cr, int limit) {
        return sDb.getPinnedSites(cr, limit);
    }

    @RobocopTarget
    public static Cursor getBookmarkForUrl(ContentResolver cr, String url) {
        return sDb.getBookmarkForUrl(cr, url);
    }

    public static boolean areContentProvidersDisabled() {
        return sAreContentProvidersEnabled;
    }

    public static void setEnableContentProviders(boolean enableContentProviders) {
        sAreContentProvidersEnabled = enableContentProviders;
    }

    public static class PinnedSite {
        public String title = "";
        public String url = "";

        public PinnedSite(String aTitle, String aUrl) {
            title = aTitle;
            url = aUrl;
        }
    }

    



    public static class TopSitesCursorWrapper extends CursorWrapper {
        int mIndex = -1; 
        Cursor mCursor = null;
        int mSize = 0;
        private SparseArray<PinnedSite> mPinnedSites = null;

        public TopSitesCursorWrapper(Cursor pinnedCursor, Cursor normalCursor, int minSize) {
            super(normalCursor);

            setPinnedSites(pinnedCursor);
            mCursor = normalCursor;
            mSize = Math.max(minSize, mPinnedSites.size() + mCursor.getCount());
        }

        public void setPinnedSites(Cursor c) {
            mPinnedSites = new SparseArray<PinnedSite>();

            if (c == null) {
                return;
            }

            try {
                if (c.getCount() <= 0) {
                    return;
                }
                c.moveToPosition(0);
                do {
                    int pos = c.getInt(c.getColumnIndex(Bookmarks.POSITION));
                    String url = c.getString(c.getColumnIndex(URLColumns.URL));
                    String title = c.getString(c.getColumnIndex(URLColumns.TITLE));
                    mPinnedSites.put(pos, new PinnedSite(title, url));
                } while (c.moveToNext());
            } finally {
                c.close();
            }
        }

        public boolean hasPinnedSites() {
            return mPinnedSites != null && mPinnedSites.size() > 0;
        }

        public PinnedSite getPinnedSite(int position) {
            if (!hasPinnedSites()) {
                return null;
            }
            return mPinnedSites.get(position);
        }

        public boolean isPinned() {
            return mPinnedSites.get(mIndex) != null;
        }

        private int getPinnedBefore(int position) {
            int numFound = 0;
            if (!hasPinnedSites()) {
                return numFound;
            }

            for (int i = 0; i < position; i++) {
                if (mPinnedSites.get(i) != null) {
                    numFound++;
                }
            }

            return numFound;
        }

        @Override
        public int getPosition() { return mIndex; }
        @Override
        public int getCount() { return mSize; }
        @Override
        public boolean isAfterLast() { return mIndex >= mSize; }
        @Override
        public boolean isBeforeFirst() { return mIndex < 0; }
        @Override
        public boolean isLast() { return mIndex == mSize - 1; }
        @Override
        public boolean moveToNext() { return moveToPosition(mIndex + 1); }
        @Override
        public boolean moveToPrevious() { return moveToPosition(mIndex - 1); }

        @Override
        public boolean moveToPosition(int position) {
            mIndex = position;

            
            
            
            int before = getPinnedBefore(position);
            int p2 = position - before;
            if (p2 <= -1) {
                super.moveToPosition(-1);
            } else if (p2 >= mCursor.getCount()) {
                super.moveToPosition(mCursor.getCount());
            } else {
                super.moveToPosition(p2);
            }

            return !(isBeforeFirst() || isAfterLast());
        }

        @Override
        public long getLong(int columnIndex) {
            if (hasPinnedSites()) {
                PinnedSite site = getPinnedSite(mIndex);
                if (site != null) {
                    return 0;
                }
            }

            if (!super.isBeforeFirst() && !super.isAfterLast())
                return super.getLong(columnIndex);
            return 0;
        }

        @Override
        public int getInt(int columnIndex) {
            if (hasPinnedSites()) {
                PinnedSite site = getPinnedSite(mIndex);
                if (site != null) {
                    return 0;
                }
            }

            if (!super.isBeforeFirst() && !super.isAfterLast())
                return super.getInt(columnIndex);
            return 0;
        }

        @Override
        public String getString(int columnIndex) {
            if (hasPinnedSites()) {
                PinnedSite site = getPinnedSite(mIndex);
                if (site != null) {
                    if (columnIndex == mCursor.getColumnIndex(URLColumns.URL)) {
                        return site.url;
                    } else if (columnIndex == mCursor.getColumnIndex(URLColumns.TITLE)) {
                        return site.title;
                    }
                    return "";
                }
            }

            if (!super.isBeforeFirst() && !super.isAfterLast())
                return super.getString(columnIndex);
            return "";
        }

        @Override
        public boolean move(int offset) {
            return moveToPosition(mIndex + offset);
        }

        @Override
        public boolean moveToFirst() {
            return moveToPosition(0);
        }

        @Override
        public boolean moveToLast() {
            return moveToPosition(mSize-1);
        }
    }
}
