




package org.mozilla.gecko.home;

import java.util.EnumSet;
import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter;
import org.mozilla.gecko.TabsAccessor;
import org.mozilla.gecko.TabsAccessor.RemoteClient;
import org.mozilla.gecko.TabsAccessor.RemoteTab;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.ExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnChildClickListener;
import android.widget.ExpandableListView.OnGroupClickListener;
import android.widget.ImageView;
import android.widget.TextView;






public class RemoteTabsExpandableListFragment extends HomeFragment {
    
    @SuppressWarnings("unused")
    private static final String LOGTAG = "GeckoRemoteTabsExpList";

    
    private static final int LOADER_ID_REMOTE_TABS = 0;

    
    private RemoteTabsExpandableListAdapter mAdapter;

    
    private ExpandableListView mList;

    
    private View mEmptyView;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

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
        mList = (ExpandableListView) view.findViewById(R.id.list);
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
                
                
                return true;
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
        mEmptyView = null;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mAdapter = new RemoteTabsExpandableListAdapter(R.layout.home_remote_tabs_group, R.layout.home_remote_tabs_child, null);
        mList.setAdapter(mAdapter);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    private void updateUiFromClients(List<RemoteClient> clients) {
        if (clients != null && !clients.isEmpty()) {
            for (int i = 0; i < mList.getExpandableListAdapter().getGroupCount(); i++) {
                mList.expandGroup(i);
            }
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
            mAdapter.replaceClients(clients);
            updateUiFromClients(clients);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            mAdapter.replaceClients(null);
        }
    }
}
