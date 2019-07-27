



package org.mozilla.gecko.db;

import java.io.File;
import java.util.Collection;
import java.util.EnumSet;
import java.util.List;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.db.BrowserContract.ExpirePriority;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.favicons.decoders.LoadFaviconResult;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.drawable.BitmapDrawable;










public interface BrowserDB {
    public interface Factory {
        public BrowserDB get(String profileName, File profileDir);
    }

    public static enum FilterFlags {
        EXCLUDE_PINNED_SITES
    }

    public abstract Searches getSearches();
    public abstract TabsAccessor getTabsAccessor();
    public abstract URLMetadata getURLMetadata();

    



    public abstract int addDefaultBookmarks(Context context, ContentResolver cr, int offset);

    



    public abstract int addDistributionBookmarks(ContentResolver cr, Distribution distribution, int offset);

    


    public abstract void invalidate();

    public abstract int getCount(ContentResolver cr, String database);

    



    public abstract Cursor filter(ContentResolver cr, CharSequence constraint,
                                  int limit, EnumSet<BrowserDB.FilterFlags> flags);

    



    public abstract Cursor getTopSites(ContentResolver cr, int limit);

    




    public abstract Cursor getTopSites(ContentResolver cr, int minLimit, int maxLimit);

    public abstract void updateVisitedHistory(ContentResolver cr, String uri);

    public abstract void updateHistoryTitle(ContentResolver cr, String uri, String title);

    


    public abstract Cursor getAllVisitedHistory(ContentResolver cr);

    


    public abstract Cursor getRecentHistory(ContentResolver cr, int limit);

    public abstract void expireHistory(ContentResolver cr, ExpirePriority priority);

    public abstract void removeHistoryEntry(ContentResolver cr, String url);

    public abstract void clearHistory(ContentResolver cr);


    public abstract String getUrlForKeyword(ContentResolver cr, String keyword);

    public abstract boolean isBookmark(ContentResolver cr, String uri);
    public abstract void addBookmark(ContentResolver cr, String title, String uri);
    public abstract Cursor getBookmarkForUrl(ContentResolver cr, String url);
    public abstract void removeBookmarksWithURL(ContentResolver cr, String uri);
    public abstract void registerBookmarkObserver(ContentResolver cr, ContentObserver observer);
    public abstract void updateBookmark(ContentResolver cr, int id, String uri, String title, String keyword);

    


    public abstract Cursor getBookmarksInFolder(ContentResolver cr, long folderId);

    


    public abstract Cursor getReadingList(ContentResolver cr);
    public abstract Cursor getReadingListUnfetched(ContentResolver cr);
    public abstract boolean isReadingListItem(ContentResolver cr, String uri);
    public abstract void addReadingListItem(ContentResolver cr, ContentValues values);
    public abstract void updateReadingListItem(ContentResolver cr, ContentValues values);
    public abstract void removeReadingListItemWithURL(ContentResolver cr, String uri);


    






    public abstract LoadFaviconResult getFaviconForUrl(ContentResolver cr, String faviconURL);

    


    public abstract String getFaviconURLFromPageURL(ContentResolver cr, String uri);

    public abstract void updateFaviconForUrl(ContentResolver cr, String pageUri, byte[] encodedFavicon, String faviconUri);

    public abstract byte[] getThumbnailForUrl(ContentResolver cr, String uri);
    public abstract void updateThumbnailForUrl(ContentResolver cr, String uri, BitmapDrawable thumbnail);

    






    public abstract Cursor getThumbnailsForUrls(ContentResolver cr,
            List<String> urls);

    public abstract void removeThumbnails(ContentResolver cr);

    
    public abstract void updateHistoryInBatch(ContentResolver cr,
            Collection<ContentProviderOperation> operations, String url,
            String title, long date, int visits);

    public abstract void updateBookmarkInBatch(ContentResolver cr,
            Collection<ContentProviderOperation> operations, String url,
            String title, String guid, long parent, long added, long modified,
            long position, String keyword, int type);

    public abstract void updateFaviconInBatch(ContentResolver cr,
            Collection<ContentProviderOperation> operations, String url,
            String faviconUrl, String faviconGuid, byte[] data);


    public abstract Cursor getPinnedSites(ContentResolver cr, int limit);
    public abstract void pinSite(ContentResolver cr, String url, String title, int position);
    public abstract void unpinSite(ContentResolver cr, int position);

    public abstract boolean hideSuggestedSite(String url);
    public abstract void setSuggestedSites(SuggestedSites suggestedSites);
    public abstract boolean hasSuggestedImageUrl(String url);
    public abstract String getSuggestedImageUrlForUrl(String url);
    public abstract int getSuggestedBackgroundColorForUrl(String url);
    public abstract int getTrackingIdForUrl(String url);
}
