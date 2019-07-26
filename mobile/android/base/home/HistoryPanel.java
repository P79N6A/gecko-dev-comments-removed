




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.IconTabWidget;

import android.content.res.Configuration;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class HistoryPanel extends HomeFragment
                          implements IconTabWidget.OnTabChangedListener {
    
    private static final String LOGTAG = "GeckoHistoryPanel";
    private IconTabWidget mTabWidget;
    private int mSelectedTab;
    private boolean initializeRecentPanel;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_history_panel, container, false);
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
        
        
        if (!initializeRecentPanel) {
            showMostRecentPanel();
            initializeRecentPanel = true;
        }
    }

    @Override
    public void onTabChanged(int index) {
        if (index == mSelectedTab) {
            return;
        }

        if (index == 0) {
            showMostRecentPanel();
        } else if (index == 1) {
            showLastTabsPanel();
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

    private void showSubPanel(Fragment subPanel) {
        final Bundle args = new Bundle();
        args.putBoolean(HomePager.CAN_LOAD_ARG, getCanLoadHint());
        subPanel.setArguments(args);

        getChildFragmentManager().beginTransaction()
                .addToBackStack(null).replace(R.id.history_panel_container, subPanel)
                .commitAllowingStateLoss();
    }

    private void showMostRecentPanel() {
        final MostRecentPanel mostRecentPanel = MostRecentPanel.newInstance();
        showSubPanel(mostRecentPanel);
    }

    private void showLastTabsPanel() {
        final LastTabsPanel lastTabsPanel = LastTabsPanel.newInstance();
        showSubPanel(lastTabsPanel);
    }
}
