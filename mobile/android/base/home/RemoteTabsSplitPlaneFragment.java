package org.mozilla.gecko.home;

import android.content.Context;
import android.database.Cursor;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.TextView;

import org.mozilla.gecko.R;
import org.mozilla.gecko.RemoteClientsDialogFragment;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter.GroupViewHolder;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.RemoteClient;
import org.mozilla.gecko.db.RemoteTab;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;






public class RemoteTabsSplitPlaneFragment extends RemoteTabsBaseFragment {
    
    private static final String LOGTAG = "GeckoSplitPlaneFragment";

    private ArrayAdapter<RemoteTab> mTabsAdapter;
    private ArrayAdapter<RemoteClient> mClientsAdapter;

    
    private DataSetObserver mObserver;

    
    private HomeListView mClientList;
    private HomeListView mTabList;

    public static RemoteTabsSplitPlaneFragment newInstance() {
        return new RemoteTabsSplitPlaneFragment();
    }

    public RemoteTabsSplitPlaneFragment() {
        super();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_remote_tabs_split_plane_panel, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mClientList = (HomeListView) view.findViewById(R.id.clients_list);
        mTabList = (HomeListView) view.findViewById(R.id.tabs_list);

        mClientList.setTag(HomePager.LIST_TAG_REMOTE_TABS);

        mTabList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapter, View view, int position, long id) {
                final RemoteTab tab = (RemoteTab) adapter.getItemAtPosition(position);
                if (tab == null) {
                    return;
                }

                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM);

                
                mUrlOpenListener.onUrlOpen(tab.url, EnumSet.of(HomePager.OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        mClientList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapter, View view, int position, long id) {
                final RemoteClient client = (RemoteClient) adapter.getItemAtPosition(position);
                if (client != null) {
                    
                    mTabsAdapter.clear();
                    for (RemoteTab tab : client.tabs) {
                        mTabsAdapter.add(tab);
                    }

                    
                    
                    mClientsAdapter.notifyDataSetChanged();
                    mTabsAdapter.notifyDataSetChanged();
                }
            }
        });

        mTabList.setContextMenuInfoFactory(new HomeContextMenuInfo.ListFactory() {
            @Override
            public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                return null;
            }

            @Override
            public HomeContextMenuInfo makeInfoForAdapter(View view, int position, long id, ListAdapter adapter) {
                final RemoteTab tab = (RemoteTab) adapter.getItem(position);
                final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                info.url = tab.url;
                info.title = tab.title;
                return info;
            }
        });

        mClientList.setContextMenuInfoFactory(new HomeContextMenuInfo.ListFactory() {
            @Override
            public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                return null;
            }

            @Override
            public HomeContextMenuInfo makeInfoForAdapter(View view, int position, long id, ListAdapter adapter) {
                final RemoteClient client = (RemoteClient) adapter.getItem(position);
                return new RemoteTabsClientContextMenuInfo(view, position, id, client);
            }
        });

        registerForContextMenu(mClientList);
        registerForContextMenu(mTabList);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mClientList = null;
        mTabList = null;
        mEmptyView = null;
        mAdapter.unregisterDataSetObserver(mObserver);
        mObserver = null;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        
        
        
        
        mFooterView = LayoutInflater.from(getActivity()).inflate(R.layout.home_remote_tabs_hidden_devices_footer, mClientList, false);
        final View view = mFooterView.findViewById(R.id.hidden_devices);
        view.setClickable(true);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final RemoteClientsDialogFragment dialog = RemoteClientsDialogFragment.newInstance(
                        getResources().getString(R.string.home_remote_tabs_hidden_devices_title),
                        getResources().getString(R.string.home_remote_tabs_unhide_selected_devices),
                        RemoteClientsDialogFragment.ChoiceMode.MULTIPLE, new ArrayList<>(mHiddenClients));
                dialog.setTargetFragment(RemoteTabsSplitPlaneFragment.this, 0);
                dialog.show(getActivity().getSupportFragmentManager(), DIALOG_TAG_REMOTE_TABS);
            }
        });

        
        
        
        
        
        
        mClientList.addFooterView(mFooterView, null, true);

        
        mAdapter = new RemoteTabsExpandableListAdapter(R.layout.home_remote_tabs_group, R.layout.home_remote_tabs_child, null, false);

        mTabsAdapter = new RemoteTabsAdapter(getActivity(), R.layout.home_remote_tabs_child);
        mClientsAdapter = new RemoteClientAdapter(getActivity(), R.layout.home_remote_tabs_group, mAdapter);

        
        
        mTabsAdapter.setNotifyOnChange(false);
        mClientsAdapter.setNotifyOnChange(false);

        mTabList.setAdapter(mTabsAdapter);
        mClientList.setAdapter(mClientsAdapter);

        mObserver = new RemoteTabDataSetObserver();
        mAdapter.registerDataSetObserver(mObserver);

        
        mClientList.removeFooterView(mFooterView);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    protected void updateUiFromClients(List<RemoteClient> clients, List<RemoteClient> hiddenClients) {
        
        
        
        
        boolean displayedSomeClients = false;

        if (hiddenClients == null || hiddenClients.isEmpty()) {
            mClientList.removeFooterView(mFooterView);
        } else {
            displayedSomeClients = true;

            final TextView textView = (TextView) mFooterView.findViewById(R.id.hidden_devices);
            if (hiddenClients.size() == 1) {
                textView.setText(getResources().getString(R.string.home_remote_tabs_one_hidden_device));
            } else {
                textView.setText(getResources().getString(R.string.home_remote_tabs_many_hidden_devices, hiddenClients.size()));
            }

            
            
            if (mClientList.getFooterViewsCount() < 1) {
                mClientList.addFooterView(mFooterView);
            }
        }

        if (clients != null && !clients.isEmpty()) {
            displayedSomeClients = true;
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

            mClientList.setEmptyView(mEmptyView);
        }
    }

    private class RemoteTabDataSetObserver extends DataSetObserver {
        @Override
        public void onChanged() {
            super.onChanged();
            mClientsAdapter.clear();
            mTabsAdapter.clear();

            for (int i = 0; i < mAdapter.getGroupCount(); i++) {
                RemoteClient client = (RemoteClient) mAdapter.getGroup(i);
                mClientsAdapter.add(client);
            }

            
            
            mTabsAdapter.notifyDataSetChanged();
            mClientsAdapter.notifyDataSetChanged();
        }

        @Override
        public void onInvalidated() {
            super.onInvalidated();
            mClientsAdapter.clear();
            mTabsAdapter.clear();
            mTabsAdapter.notifyDataSetChanged();
            mClientsAdapter.notifyDataSetChanged();
        }
    }

    private static class RemoteTabsAdapter extends ArrayAdapter<RemoteTab> {
        private final Context context;
        private final int resource;

        public RemoteTabsAdapter(Context context, int resource) {
            super(context, resource);
            this.context = context;
            this.resource = resource;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final TwoLinePageRow view;
            if (convertView != null) {
                view = (TwoLinePageRow) convertView;
            } else {
                final LayoutInflater inflater = LayoutInflater.from(context);
                view = (TwoLinePageRow) inflater.inflate(resource, parent, false);
            }

            final RemoteTab tab = getItem(position);
            view.update(tab.title, tab.url);

            return view;
        }
    }

    private class RemoteClientAdapter extends ArrayAdapter<RemoteClient> {
        private final Context context;
        private final int resource;
        private final RemoteTabsExpandableListAdapter adapter;

        public RemoteClientAdapter(Context context, int resource, RemoteTabsExpandableListAdapter adapter) {
            super(context, resource);
            this.context = context;
            this.resource = resource;
            this.adapter = adapter;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final View view;
            if (convertView != null) {
                view = convertView;
            } else {
                final LayoutInflater inflater = LayoutInflater.from(context);
                view = inflater.inflate(resource, parent, false);
                final GroupViewHolder holder = new GroupViewHolder(view);
                view.setTag(holder);
            }

            
            final RemoteClient client = getItem(position);
            
            final boolean isSelected = false;
            adapter.updateClientsItemView(isSelected, context, view, getItem(position));
            return view;
        }
    }
}
