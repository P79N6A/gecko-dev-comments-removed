



package org.mozilla.gecko.tabspanel;

import java.util.Locale;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.activities.FxAccountCreateAccountActivity;
import org.mozilla.gecko.tabspanel.TabsPanel.PanelView;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.content.Intent;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;






class RemoteTabsSetupPanel extends LinearLayout implements PanelView {
    private TabsPanel tabsPanel;

    public RemoteTabsSetupPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        final View setupGetStartedButton = findViewById(R.id.remote_tabs_setup_get_started);
        setupGetStartedButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(final View v) {
                final Context context = getContext();
                
                
                final Intent intent = new Intent(context, FxAccountCreateAccountActivity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            }
        });

        final View setupOlderVersionLink = findViewById(R.id.remote_tabs_setup_old_sync_link);
        setupOlderVersionLink.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(final View v) {
                final String url = FirefoxAccounts.getOldSyncUpgradeURL(
                        getResources(), Locale.getDefault());
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
    }

    @Override
    public void show() {
        
        if (HardwareUtils.isTablet()) {
            return;
        }

        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
    }

    @Override
    public boolean shouldExpand() {
        return getOrientation() == LinearLayout.VERTICAL;
    }
}
