




package org.mozilla.gecko.home;

import org.mozilla.gecko.util.StringUtils;

import android.database.Cursor;
import android.text.TextUtils;
import android.view.View;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ExpandableListAdapter;




public class HomeContextMenuInfo extends AdapterContextMenuInfo {

    public String url;
    public String title;
    public boolean isFolder;
    public int historyId = -1;
    public int bookmarkId = -1;
    public int readingListItemId = -1;
    public RemoveItemType itemType = null;

    
    public static enum RemoveItemType {
        BOOKMARKS, HISTORY, READING_LIST
    }

    public HomeContextMenuInfo(View targetView, int position, long id) {
        super(targetView, position, id);
    }

    public boolean hasBookmarkId() {
        return bookmarkId > -1;
    }

    public boolean hasHistoryId() {
        return historyId > -1;
    }

    public boolean isInReadingList() {
        return readingListItemId > -1;
    }

    public boolean canRemove() {
        return hasBookmarkId() || hasHistoryId() || isInReadingList();
    }

    public String getDisplayTitle() {
        if (!TextUtils.isEmpty(title)) {
            return title;
        }
        return StringUtils.stripCommonSubdomains(StringUtils.stripScheme(url, StringUtils.UrlFlags.STRIP_HTTPS));
    }

    


    public interface Factory {
        public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor);
    }

    


    public interface ExpandableFactory {
        public HomeContextMenuInfo makeInfoForAdapter(View view, int position, long id, ExpandableListAdapter adapter);
    }
}
