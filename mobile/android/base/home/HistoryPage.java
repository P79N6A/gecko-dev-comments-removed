




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.IconTabWidget;
import android.support.v4.app.Fragment;
import android.content.Context;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;

import android.widget.ImageButton;

public class HistoryPage extends HomeFragment
                        implements IconTabWidget.OnTabChangedListener {
    
    private static final String LOGTAG = "GeckoHistoryPage";
    private IconTabWidget mTabWidget;
    private int mSelectedTab;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_history_page, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mTabWidget = (IconTabWidget) view.findViewById(R.id.tab_icon_widget);

        ImageButton button;
        final Resources resources = view.getContext().getResources();

        button = mTabWidget.addTab(R.drawable.icon_most_visited);
        button.setContentDescription(resources.getString(R.string.home_most_visited_title));

        button = mTabWidget.addTab(R.drawable.icon_most_recent);
        button.setContentDescription(resources.getString(R.string.home_most_recent_title));

        button = mTabWidget.addTab(R.drawable.icon_last_tabs);
        button.setContentDescription(resources.getString(R.string.home_last_tabs_title));

        
        showMostVisitedPage();

        mTabWidget.setTabSelectionListener(this);
        mTabWidget.setCurrentTab(mSelectedTab);
    }

    @Override
    public void load() {}

    @Override
    public void onTabChanged(int index) {
        if (index == mSelectedTab) {
            return;
        }

        if (index == 0) {
            showMostVisitedPage();
        } else if (index == 1) {
            showMostRecentPage();
        } else if (index == 2) {
            showLastTabsPage();
        }

        mTabWidget.setCurrentTab(index);
        mSelectedTab = index;
    }

    private void showSubPage(Fragment subPage) {
        getChildFragmentManager().beginTransaction()
                .addToBackStack(null).replace(R.id.visited_page_container, subPage)
                .commitAllowingStateLoss();
    }

    private void showMostVisitedPage() {
        final MostVisitedPage mostVisitedPage = MostVisitedPage.newInstance();
        showSubPage(mostVisitedPage);
    }

    private void showMostRecentPage() {
        final MostRecentPage mostRecentPage = MostRecentPage.newInstance();
        showSubPage(mostRecentPage);
    }

    private void showLastTabsPage() {
        final LastTabsPage lastTabsPage = LastTabsPage.newInstance();
        showSubPage(lastTabsPage);
    }
}
