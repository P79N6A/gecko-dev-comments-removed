



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.home.HomeConfig.PanelType;
import org.mozilla.gecko.tabs.TabsPanel.PanelView;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;






class RemoteTabsPanel extends FrameLayout implements PanelView {
    private TabsPanel tabsPanel;

    public RemoteTabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.remote_tabs_panel, this);

        final View link = findViewById(R.id.go_to_panel);
        link.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                
                
                
                
                
                
                
                Tabs.getInstance().loadUrl(AboutPages.getURLForBuiltinPanelType(PanelType.REMOTE_TABS),
                        Tabs.LOADURL_NEW_TAB);
                if (tabsPanel != null) {
                    tabsPanel.autoHidePanel();
                }
            }
        });
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        tabsPanel = panel;
    }

    @Override
    public void show() {
        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
    }

    @Override
    public boolean shouldExpand() {
        final LinearLayout container = (LinearLayout) findViewById(R.id.container);
        return container.getOrientation() == LinearLayout.VERTICAL;
    }
}
