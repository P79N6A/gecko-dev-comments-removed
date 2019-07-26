




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.util.StringUtils;

import android.text.TextUtils;
import android.view.View;
import android.widget.AdapterView.AdapterContextMenuInfo;





public class HomeContextMenuInfo extends AdapterContextMenuInfo {

    public String url;
    public String title;
    public boolean isFolder = false;
    public boolean inReadingList = false;
    public int display = Combined.DISPLAY_NORMAL;
    public int historyId = -1;
    public int bookmarkId = -1;

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
        return inReadingList;
    }

    public String getDisplayTitle() {
        if (!TextUtils.isEmpty(title)) {
            return title;
        }
        return StringUtils.stripCommonSubdomains(StringUtils.stripScheme(url, StringUtils.UrlFlags.STRIP_HTTPS));
    }
}