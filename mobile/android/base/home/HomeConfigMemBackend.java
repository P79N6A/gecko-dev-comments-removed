




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.HomeConfig.HomeConfigBackend;
import org.mozilla.gecko.home.HomeConfig.OnChangeListener;
import org.mozilla.gecko.home.HomeConfig.PageEntry;
import org.mozilla.gecko.home.HomeConfig.PageType;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;

import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;

class HomeConfigMemBackend implements HomeConfigBackend {
    private final Context mContext;

    public HomeConfigMemBackend(Context context) {
        mContext = context;
    }

    public List<PageEntry> load() {
        final ArrayList<PageEntry> pageEntries = new ArrayList<PageEntry>();

        pageEntries.add(new PageEntry(PageType.TOP_SITES,
                                      mContext.getString(R.string.home_top_sites_title),
                                      EnumSet.of(PageEntry.Flags.DEFAULT_PAGE)));

        pageEntries.add(new PageEntry(PageType.BOOKMARKS,
                                      mContext.getString(R.string.bookmarks_title)));

        
        
        if (!HardwareUtils.isLowMemoryPlatform()) {
            pageEntries.add(new PageEntry(PageType.READING_LIST,
                                          mContext.getString(R.string.reading_list_title)));
        }

        final PageEntry historyEntry = new PageEntry(PageType.HISTORY,
                                                     mContext.getString(R.string.home_history_title));

        
        
        if (HardwareUtils.isTablet()) {
            pageEntries.add(historyEntry);
        } else {
            pageEntries.add(0, historyEntry);
        }

        return Collections.unmodifiableList(pageEntries);
    }

    public void save(List<PageEntry> entries) {
        
    }

    public void setOnChangeListener(OnChangeListener listener) {
        
    }
}