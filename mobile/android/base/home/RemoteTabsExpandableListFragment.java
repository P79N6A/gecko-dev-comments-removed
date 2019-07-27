




package org.mozilla.gecko.home;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.Iterator;
import java.util.List;

import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.RemoteClientsDialogFragment;
import org.mozilla.gecko.RemoteClientsDialogFragment.ChoiceMode;
import org.mozilla.gecko.RemoteClientsDialogFragment.RemoteClientsListener;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter;
import org.mozilla.gecko.TabsAccessor;
import org.mozilla.gecko.TabsAccessor.RemoteClient;
import org.mozilla.gecko.TabsAccessor.RemoteTab;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout.OnRefreshListener;

import android.accounts.Account;
import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.ExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnChildClickListener;
import android.widget.ExpandableListView.OnGroupClickListener;
import android.widget.ImageView;
import android.widget.TextView;






public class RemoteTabsExpandableListFragment extends HomeFragment implements RemoteClientsListener {
    
    private static final String LOGTAG = "GeckoRemoteTabsExpList";

    
    private static final int LOADER_ID_REMOTE_TABS = 0;

    
    private static final String DIALOG_TAG_REMOTE_TABS = "dialog_tag_remote_tabs";

    private static final String[] STAGES_TO_SYNC_ON_REFRESH = new String[] { "clients", "tabs" };

    
    
    private static RemoteTabsExpandableListState sState;

    
    private RemoteTabsExpandableListAdapter mAdapter;

    
    
    private final ArrayList<RemoteClient> mHiddenClients = new ArrayList<RemoteClient>();

    
    private HomeExpandableListView mList;

    
    private View mEmptyView;

    
    private View mFooterView;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private GeckoSwipeRefreshLayout mRefreshLayout;

    
    private RemoteTabsSyncListener mSyncStatusListener;

    public static RemoteTabsExpandableListFragment newInstance() {
        return new RemoteTabsExpandableListFragment();
    }

    public RemoteTabsExpandableListFragment() {
        super();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_remote_tabs_list_panel, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mRefreshLayout = (GeckoSwipeRefreshLayout) view.findViewById(R.id.remote_tabs_refresh_layout);
        mRefreshLayout.setColorScheme(
                R.color.swipe_refresh_orange, R.color.swipe_refresh_white,
                R.color.swipe_refresh_orange, R.color.swipe_refresh_white);
        mRefreshLayout.setOnRefreshListener(new RemoteTabsRefreshListener());

        mSyncStatusListener = new RemoteTabsSyncListener();
        FirefoxAccounts.addSyncStatusListener(mSyncStatusListener);

        mList = (HomeExpandableListView) view.findViewById(R.id.list);
        mList.setTag(HomePager.LIST_TAG_REMOTE_TABS);

        mList.setOnChildClickListener(new OnChildClickListener() {
            @Override
            public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id) {
                final ExpandableListAdapter adapter = parent.getExpandableListAdapter();
                final RemoteTab tab = (RemoteTab) adapter.getChild(groupPosition, childPosition);
                if (tab == null) {
                    return false;
                }

                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM);

                
                mUrlOpenListener.onUrlOpen(tab.url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
                return true;
            }
        });

        mList.setOnGroupClickListener(new OnGroupClickListener() {
            @Override
            public boolean onGroupClick(ExpandableListView parent, View v, int groupPosition, long id) {
                final ExpandableListAdapter adapter = parent.getExpandableListAdapter();
                final RemoteClient client = (RemoteClient) adapter.getGroup(groupPosition);
                if (client != null) {
                    
                    sState.setClientCollapsed(client.guid, mList.isGroupExpanded(groupPosition));
                }

                
                return false;
            }
        });

        
        mList.setContextMenuInfoFactory(new HomeContextMenuInfo.ExpandableFactory() {
            @Override
            public HomeContextMenuInfo makeInfoForAdapter(View view, int position, long id, ExpandableListAdapter adapter) {
                long packedPosition = mList.getExpandableListPosition(position);
                final int groupPosition = ExpandableListView.getPackedPositionGroup(packedPosition);
                final int type = ExpandableListView.getPackedPositionType(packedPosition);
                if (type == ExpandableListView.PACKED_POSITION_TYPE_CHILD) {
                    final int childPosition = ExpandableListView.getPackedPositionChild(packedPosition);
                    final RemoteTab tab = (RemoteTab) adapter.getChild(groupPosition, childPosition);
                    final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                    info.url = tab.url;
                    info.title = tab.title;
                    return info;
                } else if (type == ExpandableListView.PACKED_POSITION_TYPE_GROUP) {
                    final RemoteClient client = (RemoteClient) adapter.getGroup(groupPosition);
                    final RemoteTabsClientContextMenuInfo info = new RemoteTabsClientContextMenuInfo(view, position, id, client);
                    return info;
                }
                return null;
            }
        });

        registerForContextMenu(mList);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
        mEmptyView = null;

        if (mSyncStatusListener != null) {
            FirefoxAccounts.removeSyncStatusListener(mSyncStatusListener);
            mSyncStatusListener = null;
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        
        
        
        
        if (sState == null) {
            sState = new RemoteTabsExpandableListState(GeckoSharedPrefs.forProfile(getActivity()));
        }

        
        
        
        
        
        mFooterView = LayoutInflater.from(getActivity()).inflate(R.layout.home_remote_tabs_hidden_devices_footer, mList, false);
        final View view = mFooterView.findViewById(R.id.hidden_devices);
        view.setClickable(true);
        view.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                final RemoteClientsDialogFragment dialog = RemoteClientsDialogFragment.newInstance(
                        getResources().getString(R.string.home_remote_tabs_hidden_devices_title),
                        getResources().getString(R.string.home_remote_tabs_unhide_selected_devices),
                        ChoiceMode.MULTIPLE, new ArrayList<RemoteClient>(mHiddenClients));
                dialog.setTargetFragment(RemoteTabsExpandableListFragment.this, 0);
                dialog.show(getActivity().getSupportFragmentManager(), DIALOG_TAG_REMOTE_TABS);
            }
        });

        
        
        
        
        
        
        mList.addFooterView(mFooterView, null, true);

        
        mAdapter = new RemoteTabsExpandableListAdapter(R.layout.home_remote_tabs_group, R.layout.home_remote_tabs_child, null);
        mList.setAdapter(mAdapter);

        
        mList.removeFooterView(mFooterView);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
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

        final ContextMenuInfo menuInfo = item.getMenuInfo();
        if (!(menuInfo instanceof RemoteTabsClientContextMenuInfo)) {
            return false;
        }

        final RemoteTabsClientContextMenuInfo info = (RemoteTabsClientContextMenuInfo) menuInfo;

        final int itemId = item.getItemId();
        if (itemId == R.id.home_remote_tabs_hide_client) {
            sState.setClientHidden(info.client.guid, true);
            getLoaderManager().restartLoader(LOADER_ID_REMOTE_TABS, null, mCursorLoaderCallbacks);
            return true;
        } else if (itemId == R.id.home_remote_tabs_show_client) {
            sState.setClientHidden(info.client.guid, false);
            getLoaderManager().restartLoader(LOADER_ID_REMOTE_TABS, null, mCursorLoaderCallbacks);
            return true;
        }
        return false;
    }

    private void updateUiFromClients(List<RemoteClient> clients, List<RemoteClient> hiddenClients) {
        
        
        
        
        boolean displayedSomeClients = false;

        if (hiddenClients == null || hiddenClients.isEmpty()) {
            mList.removeFooterView(mFooterView);
        } else {
            displayedSomeClients = true;

            final TextView textView = (TextView) mFooterView.findViewById(R.id.hidden_devices);
            if (hiddenClients.size() == 1) {
                textView.setText(getResources().getString(R.string.home_remote_tabs_one_hidden_device));
            } else {
                textView.setText(getResources().getString(R.string.home_remote_tabs_many_hidden_devices, hiddenClients.size()));
            }

            
            
            if (mList.getFooterViewsCount() < 1) {
                mList.addFooterView(mFooterView);
            }
        }

        if (clients != null && !clients.isEmpty()) {
            displayedSomeClients = true;

            
            int groupCount = Math.min(mList.getExpandableListAdapter().getGroupCount(), clients.size());
            for (int i = 0; i < groupCount; i++) {
                final RemoteClient client = clients.get(i);
                if (sState.isClientCollapsed(client.guid)) {
                    mList.collapseGroup(i);
                } else {
                    mList.expandGroup(i);
                }
            }
        }

        if (displayedSomeClients) {
            return;
        }

        
        
        if (mEmptyView == null) {
            
            final ViewStub emptyViewStub = (ViewStub) getView().findViewById(R.id.home_empty_view_stub);
            mEmptyView = emptyViewStub.inflate();

            final ImageView emptyIcon = (ImageView) mEmptyView.findViewById(R.id.home_empty_image);
            emptyIcon.setImageResource(R.drawable.icon_remote_tabs_empty);

            final TextView emptyText = (TextView) mEmptyView.findViewById(R.id.home_empty_text);
            emptyText.setText(R.string.home_remote_tabs_empty);

            mList.setEmptyView(mEmptyView);
        }
    }

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

    private static class RemoteTabsCursorLoader extends SimpleCursorLoader {
        public RemoteTabsCursorLoader(Context context) {
            super(context);
        }

        @Override
        public Cursor loadCursor() {
            return TabsAccessor.getRemoteTabsCursor(getContext());
        }
    }

    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            return new RemoteTabsCursorLoader(getActivity());
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            final List<RemoteClient> clients = TabsAccessor.getClientsFromCursor(c);

            
            
            
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
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            mAdapter.replaceClients(null);
        }
    }

    private class RemoteTabsRefreshListener implements OnRefreshListener {
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

    private class RemoteTabsSyncListener implements FirefoxAccounts.SyncStatusListener {
        @Override
        public Context getContext() {
            return getActivity();
        }

        @Override
        public Account getAccount() {
            return FirefoxAccounts.getFirefoxAccount(getContext());
        }

        public void onSyncStarted() {
        }

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
}
