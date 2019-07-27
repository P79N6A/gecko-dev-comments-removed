




package org.mozilla.gecko.home;

import android.accounts.Account;
import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.RemoteClientsDialogFragment.RemoteClientsListener;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.RemoteClient;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;




public abstract class RemoteTabsBaseFragment extends HomeFragment implements RemoteClientsListener {
    
    private static final String LOGTAG = "GeckoRemoteTabsBaseFragment";

    private static final String[] STAGES_TO_SYNC_ON_REFRESH = new String[] { "clients", "tabs" };

    
    private static final long LAST_SYNCED_TIME_UPDATE_INTERVAL_IN_MILLISECONDS = 60 * 1000; 

    
    protected static final int LOADER_ID_REMOTE_TABS = 0;

    
    protected static final String DIALOG_TAG_REMOTE_TABS = "dialog_tag_remote_tabs";

    
    
    protected static RemoteTabsExpandableListState sState;

    
    protected RemoteTabsExpandableListAdapter mAdapter;

    
    
    protected final List<RemoteClient> mHiddenClients = new ArrayList<>();

    
    protected CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    protected GeckoSwipeRefreshLayout mRefreshLayout;

    
    protected RemoteTabsSyncListener mSyncStatusListener;

    
    protected View mEmptyView;

    
    protected View mFooterView;

    
    protected Handler mHandler;

    
    protected Runnable mLastSyncedTimeUpdateRunnable;

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mRefreshLayout = (GeckoSwipeRefreshLayout) view.findViewById(R.id.remote_tabs_refresh_layout);
        mRefreshLayout.setColorScheme(
                R.color.swipe_refresh_orange, R.color.swipe_refresh_white,
                R.color.swipe_refresh_orange, R.color.swipe_refresh_white);
        mRefreshLayout.setOnRefreshListener(new RemoteTabsRefreshListener());

        mSyncStatusListener = new RemoteTabsSyncListener();
        FirefoxAccounts.addSyncStatusListener(mSyncStatusListener);

        mHandler = new Handler(); 
        mLastSyncedTimeUpdateRunnable = new LastSyncTimeUpdateRunnable();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        
        
        
        
        if (sState == null) {
            sState = new RemoteTabsExpandableListState(GeckoSharedPrefs.forProfile(getActivity()));
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        if (mSyncStatusListener != null) {
            FirefoxAccounts.removeSyncStatusListener(mSyncStatusListener);
            mSyncStatusListener = null;
        }

        if (mLastSyncedTimeUpdateRunnable != null) {
            mHandler.removeCallbacks(mLastSyncedTimeUpdateRunnable);
            mLastSyncedTimeUpdateRunnable = null;
            mHandler = null;
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenu.ContextMenuInfo menuInfo) {
        if (!(menuInfo instanceof RemoteTabsClientContextMenuInfo)) {
            
            
            super.onCreateContextMenu(menu, view, menuInfo);
            return;
        }

        
        final MenuInflater inflater = new MenuInflater(view.getContext());
        inflater.inflate(R.menu.home_remote_tabs_client_contextmenu, menu);

        final RemoteTabsClientContextMenuInfo info = (RemoteTabsClientContextMenuInfo) menuInfo;
        menu.setHeaderTitle(info.client.name);

        
        final boolean isHidden = sState.isClientHidden(info.client.guid);
        final MenuItem item = menu.findItem(isHidden
                ? R.id.home_remote_tabs_hide_client
                : R.id.home_remote_tabs_show_client);
        item.setVisible(false);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (super.onContextItemSelected(item)) {
            
            return true;
        }

        final ContextMenu.ContextMenuInfo menuInfo = item.getMenuInfo();
        if (!(menuInfo instanceof RemoteTabsClientContextMenuInfo)) {
            return false;
        }

        final RemoteTabsClientContextMenuInfo info = (RemoteTabsClientContextMenuInfo) menuInfo;

        final int itemId = item.getItemId();
        if (itemId == R.id.home_remote_tabs_hide_client) {
            sState.setClientHidden(info.client.guid, true);
            getLoaderManager().restartLoader(LOADER_ID_REMOTE_TABS, null, mCursorLoaderCallbacks);
            return true;
        }

        if (itemId == R.id.home_remote_tabs_show_client) {
            sState.setClientHidden(info.client.guid, false);
            getLoaderManager().restartLoader(LOADER_ID_REMOTE_TABS, null, mCursorLoaderCallbacks);
            return true;
        }

        return false;
    }

    @Override
    public void onClients(List<RemoteClient> clients) {
        
        
        for (RemoteClient client : clients) {
            sState.setClientHidden(client.guid, false);
            
            
            sState.setClientCollapsed(client.guid, false);
        }
        getLoaderManager().restartLoader(LOADER_ID_REMOTE_TABS, null, mCursorLoaderCallbacks);
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_REMOTE_TABS, null, mCursorLoaderCallbacks);
    }

    protected abstract void updateUiFromClients(List<RemoteClient> clients, List<RemoteClient> hiddenClients);

    private static class RemoteTabsCursorLoader extends SimpleCursorLoader {
        private final GeckoProfile mProfile;

        public RemoteTabsCursorLoader(Context context) {
            super(context);
            mProfile = GeckoProfile.get(context);
        }

        @Override
        public Cursor loadCursor() {
            return mProfile.getDB().getTabsAccessor().getRemoteTabsCursor(getContext());
        }
    }

    protected class CursorLoaderCallbacks extends TransitionAwareCursorLoaderCallbacks {
        private BrowserDB mDB;    

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            mDB = GeckoProfile.get(getActivity()).getDB();
            return new RemoteTabsCursorLoader(getActivity());
        }

        @Override
        public void onLoadFinishedAfterTransitions(Loader<Cursor> loader, Cursor c) {
            final List<RemoteClient> clients = mDB.getTabsAccessor().getClientsFromCursor(c);

            
            
            
            mHiddenClients.clear();
            final Iterator<RemoteClient> it = clients.iterator();
            while (it.hasNext()) {
                final RemoteClient client = it.next();
                if (sState.isClientHidden(client.guid)) {
                    it.remove();
                    mHiddenClients.add(client);
                }
            }

            mAdapter.replaceClients(clients);
            updateUiFromClients(clients, mHiddenClients);
            scheduleLastSyncedTime();
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            super.onLoaderReset(loader);
            mAdapter.replaceClients(null);
        }
    }

    protected class RemoteTabsRefreshListener implements GeckoSwipeRefreshLayout.OnRefreshListener {
        @Override
        public void onRefresh() {
            if (FirefoxAccounts.firefoxAccountsExist(getActivity())) {
                final Account account = FirefoxAccounts.getFirefoxAccount(getActivity());
                FirefoxAccounts.requestSync(account, FirefoxAccounts.FORCE, STAGES_TO_SYNC_ON_REFRESH, null);
            } else {
                Log.wtf(LOGTAG, "No Firefox Account found; this should never happen. Ignoring.");
                mRefreshLayout.setRefreshing(false);
            }
        }
    }

    protected class RemoteTabsSyncListener implements FirefoxAccounts.SyncStatusListener {
        @Override
        public Context getContext() {
            return getActivity();
        }

        @Override
        public Account getAccount() {
            return FirefoxAccounts.getFirefoxAccount(getContext());
        }

        @Override
        public void onSyncStarted() {
        }

        @Override
        public void onSyncFinished() {
            mRefreshLayout.setRefreshing(false);
        }
    }

    


    protected static class RemoteTabsClientContextMenuInfo extends HomeContextMenuInfo {
        protected final RemoteClient client;

        public RemoteTabsClientContextMenuInfo(View targetView, int position, long id, RemoteClient client) {
            super(targetView, position, id);
            this.client = client;
        }
    }

    


    protected class LastSyncTimeUpdateRunnable implements Runnable  {
        @Override
        public void run() {
            updateAndScheduleLastSyncedTime();
        }
    }

    protected void scheduleLastSyncedTime() {
        
        mHandler.postDelayed(mLastSyncedTimeUpdateRunnable, LAST_SYNCED_TIME_UPDATE_INTERVAL_IN_MILLISECONDS);
    }

    protected void updateAndScheduleLastSyncedTime() {
        
        mAdapter.notifyDataSetChanged();
        scheduleLastSyncedTime();
    }
}
