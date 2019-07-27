




package org.mozilla.gecko.tabs;

import java.util.Locale;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.tabs.TabsPanel.CloseAllPanelView;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;







class PrivateTabsPanel extends FrameLayout implements CloseAllPanelView {
    private TabsPanel tabsPanel;
    private TabsListLayout tabsTray;

    public PrivateTabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.private_tabs_panel, this);
        tabsTray = (TabsListLayout) findViewById(R.id.private_tabs_tray);

        final View emptyView = findViewById(R.id.private_tabs_empty);
        tabsTray.setEmptyView(emptyView);

        final View learnMore = findViewById(R.id.private_tabs_learn_more);
        learnMore.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                final String locale = BrowserLocaleManager.getLanguageTag(Locale.getDefault());
                final String url =
                        getResources().getString(R.string.private_tabs_panel_learn_more_link, locale);
                Tabs.getInstance().loadUrlInTab(url);
                if (tabsPanel != null) {
                    tabsPanel.autoHidePanel();
                }
            }
        });
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        tabsPanel = panel;
        tabsTray.setTabsPanel(panel);
    }

    @Override
    public void show() {
        tabsTray.show();
        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
        tabsTray.hide();
    }

    @Override
    public boolean shouldExpand() {
        return tabsTray.shouldExpand();
    }

    @Override
    public void closeAll() {
        tabsTray.closeAll();
    }
}
