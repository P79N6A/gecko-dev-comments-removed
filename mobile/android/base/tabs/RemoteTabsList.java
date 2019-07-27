



package org.mozilla.gecko.tabs;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.RemoteTabsExpandableListAdapter;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.TabsAccessor;
import org.mozilla.gecko.TabsAccessor.RemoteClient;
import org.mozilla.gecko.TabsAccessor.RemoteTab;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ExpandableListView;





public class RemoteTabsList extends ExpandableListView
                            implements ExpandableListView.OnGroupClickListener,
                                       ExpandableListView.OnChildClickListener,
                                       TabsAccessor.OnQueryTabsCompleteListener {
    private TabsPanel tabsPanel;

    
    private List<String> expandedClientList;

    
    private String clientScrollPosition;

    public RemoteTabsList(Context context, AttributeSet attrs) {
        super(context, attrs);

        setOnGroupClickListener(this);
        setOnChildClickListener(this);
        setAdapter(new RemoteTabsExpandableListAdapter(R.layout.remote_tabs_group, R.layout.remote_tabs_child, null));
    }

    public void setTabsPanel(TabsPanel panel) {
        tabsPanel = panel;
    }

    private void autoHidePanel() {
        tabsPanel.autoHidePanel();
    }

    @Override
    public boolean onGroupClick(ExpandableListView parent, View view, int groupPosition, long id) {
        final RemoteClient client = (RemoteClient) parent.getExpandableListAdapter().getGroup(groupPosition);
        final String clientGuid = client.guid;

        if (isGroupExpanded(groupPosition)) {
            collapseGroup(groupPosition);
            expandedClientList.remove(clientGuid);
        } else {
            expandGroup(groupPosition);
            expandedClientList.add(clientGuid);
        }

        clientScrollPosition = clientGuid;
        return true;
    }

    @Override
    public boolean onChildClick(ExpandableListView parent, View view, int groupPosition, int childPosition, long id) {
        final RemoteTab tab = (RemoteTab) parent.getExpandableListAdapter().getChild(groupPosition, childPosition);
        if (tab == null) {
            autoHidePanel();
            return true;
        }

        Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM, "remote");

        Tabs.getInstance().loadUrl(tab.url, Tabs.LOADURL_NEW_TAB);
        autoHidePanel();

        final RemoteClient client = (RemoteClient) parent.getExpandableListAdapter().getGroup(groupPosition);
        clientScrollPosition = client.guid;

        return true;
    }

    @Override
    public void onQueryTabsComplete(List<RemoteClient> clients) {
        ((RemoteTabsExpandableListAdapter) getExpandableListAdapter()).replaceClients(clients);

        
        List<String> newExpandedClientList = new ArrayList<String>();
        for (int i = 0; i < clients.size(); i++) {
            final String clientGuid = clients.get(i).guid;

            if (expandedClientList == null) {
                
                newExpandedClientList.add(clientGuid);
                expandGroup(i);
            } else {
                
                if (expandedClientList.contains(clientGuid)) {
                    newExpandedClientList.add(clientGuid);
                    expandGroup(i);
                }

                
                if (clientGuid.equals(clientScrollPosition)) {
                    setSelectedGroup(i);
                }
            }
        }
        expandedClientList = newExpandedClientList;
    }
}
