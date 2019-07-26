




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.IconTabWidget;
import android.support.v4.app.Fragment;
import android.content.Context;
import android.content.res.Configuration;
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
    private boolean initializeRecentPage;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_history_page, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mTabWidget = (IconTabWidget) view.findViewById(R.id.tab_icon_widget);

        mTabWidget.addTab(R.drawable.icon_most_recent, R.string.home_most_recent_title);
        mTabWidget.addTab(R.drawable.icon_last_tabs, R.string.home_last_tabs_title);

        mTabWidget.setTabSelectionListener(this);
        mTabWidget.setCurrentTab(mSelectedTab);

        loadIfVisible();
    }

    @Override
    public void load() {
        
        
        if (!initializeRecentPage) {
            showMostRecentPage();
            initializeRecentPage = true;
        }
    }

    @Override
    public void onTabChanged(int index) {
        if (index == mSelectedTab) {
            return;
        }

        if (index == 0) {
            showMostRecentPage();
        } else if (index == 1) {
            showLastTabsPage();
        }

        mTabWidget.setCurrentTab(index);
        mSelectedTab = index;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
    }

    private void showSubPage(Fragment subPage) {
        final Bundle args = new Bundle();
        args.putBoolean(HomePager.CAN_LOAD_ARG, getCanLoadHint());
        subPage.setArguments(args);

        getChildFragmentManager().beginTransaction()
                .addToBackStack(null).replace(R.id.history_page_container, subPage)
                .commitAllowingStateLoss();
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
