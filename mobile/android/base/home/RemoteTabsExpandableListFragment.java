




package org.mozilla.gecko.home;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.RemoteClientsDialogFragment;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.RemoteClient;
import org.mozilla.gecko.db.RemoteTab;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.os.Bundle;
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






public class RemoteTabsExpandableListFragment extends RemoteTabsBaseFragment {
    
    private static final String LOGTAG = "GeckoRemoteTabsExpList";

    
    private HomeExpandableListView mList;

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
        super.onViewCreated(view, savedInstanceState);

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
                }

                if (type == ExpandableListView.PACKED_POSITION_TYPE_GROUP) {
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
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        
        
        
        
        mFooterView = LayoutInflater.from(getActivity()).inflate(R.layout.home_remote_tabs_hidden_devices_footer, mList, false);
        final View view = mFooterView.findViewById(R.id.hidden_devices);
        view.setClickable(true);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final RemoteClientsDialogFragment dialog = RemoteClientsDialogFragment.newInstance(
                        getResources().getString(R.string.home_remote_tabs_hidden_devices_title),
                        getResources().getString(R.string.home_remote_tabs_unhide_selected_devices),
                        RemoteClientsDialogFragment.ChoiceMode.MULTIPLE, new ArrayList<>(mHiddenClients));
                dialog.setTargetFragment(RemoteTabsExpandableListFragment.this, 0);
                dialog.show(getActivity().getSupportFragmentManager(), DIALOG_TAG_REMOTE_TABS);
            }
        });

        
        
        
        
        
        
        mList.addFooterView(mFooterView, null, true);

        
        mAdapter = new RemoteTabsExpandableListAdapter(R.layout.home_remote_tabs_group, R.layout.home_remote_tabs_child, null, true);
        mList.setAdapter(mAdapter);

        
        mList.removeFooterView(mFooterView);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    protected void updateUiFromClients(List<RemoteClient> clients, List<RemoteClient> hiddenClients) {
        if (getView() == null) {
            
            
            
            return;
        }

        
        
        
        
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
}
