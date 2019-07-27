




package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;
import org.mozilla.gecko.tabs.TabsPanel.CloseAllPanelView;
import org.mozilla.gecko.tabs.TabsPanel.TabsLayout;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;







class PrivateTabsPanel extends FrameLayout implements CloseAllPanelView {
    private final TabsLayout tabsLayout;

    public PrivateTabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.private_tabs_panel, this);
        tabsLayout = (TabsLayout) findViewById(R.id.private_tabs_layout);

        final ViewGroup emptyTabsFrame = (ViewGroup) findViewById(R.id.private_tabs_empty);
        tabsLayout.setEmptyView(emptyTabsFrame);
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        tabsLayout.setTabsPanel(panel);
    }

    @Override
    public void show() {
        tabsLayout.show();
        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
        tabsLayout.hide();
    }

    @Override
    public boolean shouldExpand() {
        return tabsLayout.shouldExpand();
    }

    @Override
    public void closeAll() {
        tabsLayout.closeAll();
    }
}
