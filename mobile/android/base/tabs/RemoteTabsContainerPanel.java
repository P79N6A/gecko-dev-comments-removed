



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.TabsAccessor;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.tabs.TabsPanel.Panel;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout;

import android.accounts.Account;
import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;






public class RemoteTabsContainerPanel extends GeckoSwipeRefreshLayout
                                 implements TabsPanel.PanelView {
    private static final String[] STAGES_TO_SYNC_ON_REFRESH = new String[] { "clients", "tabs" };

    




    private static final long MINIMUM_REFRESH_INDICATOR_DURATION_IN_MS = 12 * 100; 

    private final Context context;
    private final RemoteTabsSyncObserver syncListener;
    private TabsPanel panel;
    private RemoteTabsList list;

    
    private boolean isListening;

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

        
        setColorScheme(R.color.swipe_refresh_orange1, R.color.swipe_refresh_orange2,
                       R.color.swipe_refresh_orange3, R.color.swipe_refresh_orange4);
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
        this.panel = panel;
        list.setTabsPanel(panel);
    }

    @Override
    public void show() {
        
        TabsAccessor.getTabs(context, list);
        
        
        Tabs.getInstance().persistAllTabs();

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
        
        
        protected volatile long lastSyncStarted;

        @Override
        public Context getContext() {
            return RemoteTabsContainerPanel.this.getContext();
        }

        @Override
        public Account getAccount() {
            return FirefoxAccounts.getFirefoxAccount(getContext());
        }

        public void onSyncStarted() {
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    lastSyncStarted = System.currentTimeMillis();

                    
                    
                    
                    
                    panel.setIconDrawable(Panel.REMOTE_TABS, R.drawable.tabs_synced_animated);
                    final Drawable iconDrawable = panel.getIconDrawable(Panel.REMOTE_TABS);
                    if (iconDrawable instanceof AnimationDrawable) {
                        ((AnimationDrawable) iconDrawable).start();
                    }
                }
            });
        }

        public void onSyncFinished() {
            final Handler uiHandler = ThreadUtils.getUiHandler();

            
            uiHandler.post(new Runnable() {
                @Override
                public void run() {
                    TabsAccessor.getTabs(context, list);
                }
            });

            
            
            final long last = lastSyncStarted;
            final long now = System.currentTimeMillis();
            final long delay = Math.max(0, MINIMUM_REFRESH_INDICATOR_DURATION_IN_MS - (now - last));

            uiHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    setRefreshing(false);

                    
                    
                    
                    panel.setIconDrawable(Panel.REMOTE_TABS, R.drawable.tabs_synced);
                }
            }, delay);
        }
    }
}
