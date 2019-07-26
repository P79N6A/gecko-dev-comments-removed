



package org.mozilla.gecko.tabspanel;

import org.mozilla.gecko.R;
import org.mozilla.gecko.TabsAccessor;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout;

import android.accounts.Account;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;





public class RemoteTabsContainerPanel extends GeckoSwipeRefreshLayout
                                 implements TabsPanel.PanelView {
    private static final String[] STAGES_TO_SYNC_ON_REFRESH = new String[] { "tabs" };

    private final Context context;
    private final RemoteTabsSyncObserver syncListener;
    private RemoteTabsList list;

    
    private boolean isListening = false;

    public RemoteTabsContainerPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
        this.syncListener = new RemoteTabsSyncObserver();

        setOnRefreshListener(new RemoteTabsRefreshListener());
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        super.addView(child, index, params);

        list = (RemoteTabsList) child;

        
        setColorScheme(R.color.swipe_refresh_orange_dark, R.color.background_tabs,
                       R.color.swipe_refresh_orange_dark, R.color.background_tabs);
    }


    @Override
    public boolean canChildScrollUp() {
        
        
        if (FirefoxAccounts.firefoxAccountsExist(getContext())) {
            return super.canChildScrollUp();
        } else {
            return true;
        }
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        list.setTabsPanel(panel);
    }

    @Override
    public void show() {
        TabsAccessor.getTabs(context, list);
        if (!isListening) {
            isListening = true;
            FirefoxAccounts.addSyncStatusListener(syncListener);
        }
        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
        if (isListening) {
            isListening = false;
            FirefoxAccounts.removeSyncStatusListener(syncListener);
        }
    }

    @Override
    public boolean shouldExpand() {
        return true;
    }

    private class RemoteTabsRefreshListener implements OnRefreshListener {
        @Override
        public void onRefresh() {
            if (FirefoxAccounts.firefoxAccountsExist(getContext())) {
                final Account account = FirefoxAccounts.getFirefoxAccount(getContext());
                FirefoxAccounts.requestSync(account, FirefoxAccounts.FORCE, STAGES_TO_SYNC_ON_REFRESH, null);
            }
        }
    }

    private class RemoteTabsSyncObserver implements FirefoxAccounts.SyncStatusListener {
        @Override
        public Context getContext() {
            return RemoteTabsContainerPanel.this.getContext();
        }

        @Override
        public Account getAccount() {
            return FirefoxAccounts.getFirefoxAccount(getContext());
        }

        public void onSyncFinished() {
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    TabsAccessor.getTabs(context, list);
                    setRefreshing(false);
                }
            });
        }

        public void onSyncStarted() {}
    }
}
