




package org.mozilla.gecko.tabs;

import java.util.Locale;

import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.tabs.TabsPanel.CloseAllPanelView;
import org.mozilla.gecko.tabs.TabsPanel.TabsLayout;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;







class PrivateTabsPanel extends FrameLayout implements CloseAllPanelView {
    private TabsPanel tabsPanel;

    private final TabsLayout tabsLayout;

    public PrivateTabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.private_tabs_panel, this);
        tabsLayout = (TabsLayout) findViewById(R.id.private_tabs_layout);

        final LinearLayout emptyTabsFrame = (LinearLayout) findViewById(R.id.private_tabs_empty);
        tabsLayout.setEmptyView(emptyTabsFrame);

        final View learnMore = findViewById(R.id.private_tabs_learn_more);
        learnMore.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                final String locale = Locales.getLanguageTag(Locale.getDefault());
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
