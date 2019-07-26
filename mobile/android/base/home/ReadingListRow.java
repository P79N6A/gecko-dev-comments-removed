




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.home.TwoLinePageRow;

import android.content.Context;
import android.util.AttributeSet;

public class ReadingListRow extends TwoLinePageRow {

    public ReadingListRow(Context context) {
        this(context, null);
    }

    public ReadingListRow(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void updateDisplayedUrl() {
        String pageUrl = getUrl();

        boolean isPrivate = Tabs.getInstance().getSelectedTab().isPrivate();
        Tab tab = Tabs.getInstance().getFirstTabForUrl(pageUrl, isPrivate);

        if (tab != null && AboutPages.isAboutReader(tab.getURL())) {
            setUrl(R.string.switch_to_tab);
            setSwitchToTabIcon(R.drawable.ic_url_bar_tab);
        } else {
            setUrl(pageUrl);
            setSwitchToTabIcon(NO_ICON);
        }
    }

}
