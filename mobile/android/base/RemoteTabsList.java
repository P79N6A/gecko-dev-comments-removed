



package org.mozilla.gecko;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;





class RemoteTabsList extends ExpandableListView
                            implements ExpandableListView.OnGroupClickListener,
                                       ExpandableListView.OnChildClickListener,
                                       TabsAccessor.OnQueryTabsCompleteListener {
    private static final String[] CLIENT_KEY = new String[] { "name" };
    private static final String[] TAB_KEY = new String[] { "title", "url" };
    private static final int[] CLIENT_RESOURCE = new int[] { R.id.client };
    private static final int[] TAB_RESOURCE = new int[] { R.id.tab, R.id.url };

    private final Context context;
    private TabsPanel tabsPanel;

    private ArrayList <ArrayList <HashMap <String, String>>> tabsList;

    public RemoteTabsList(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;

        setOnGroupClickListener(this);
        setOnChildClickListener(this);
    }

    public void setTabsPanel(TabsPanel panel) {
        tabsPanel = panel;
    }

    private void autoHidePanel() {
        tabsPanel.autoHidePanel();
    }

    @Override
    public boolean onGroupClick(ExpandableListView parent, View view, int position, long id) {
        
        return true;
    }

    @Override
    public boolean onChildClick(ExpandableListView parent, View view, int groupPosition, int childPosition, long id) {
        HashMap <String, String> tab = tabsList.get(groupPosition).get(childPosition);
        if (tab == null) {
            autoHidePanel();
            return true;
        }

        Tabs.getInstance().loadUrl(tab.get("url"), Tabs.LOADURL_NEW_TAB);
        autoHidePanel();
        return true;
    }

    @Override
    public void onQueryTabsComplete(List<TabsAccessor.RemoteTab> remoteTabsList) {
        ArrayList<TabsAccessor.RemoteTab> remoteTabs = new ArrayList<TabsAccessor.RemoteTab> (remoteTabsList);
        if (remoteTabs == null || remoteTabs.size() == 0)
            return;

        ArrayList <HashMap <String, String>> clients = new ArrayList <HashMap <String, String>>();

        tabsList = new ArrayList <ArrayList <HashMap <String, String>>>();

        String oldGuid = null;
        ArrayList <HashMap <String, String>> tabsForClient = null;
        HashMap <String, String> client;
        HashMap <String, String> tab;

        for (TabsAccessor.RemoteTab remoteTab : remoteTabs) {
            if (oldGuid == null || !TextUtils.equals(oldGuid, remoteTab.guid)) {
                client = new HashMap <String, String>();
                client.put("name", remoteTab.name);
                clients.add(client);

                tabsForClient = new ArrayList <HashMap <String, String>>();
                tabsList.add(tabsForClient);

                oldGuid = new String(remoteTab.guid);
            }

            tab = new HashMap<String, String>();
            tab.put("title", TextUtils.isEmpty(remoteTab.title) ? remoteTab.url : remoteTab.title);
            tab.put("url", remoteTab.url);
            tabsForClient.add(tab);
        }

        setAdapter(new SimpleExpandableListAdapter(context,
                                                   clients,
                                                   R.layout.remote_tabs_group,
                                                   CLIENT_KEY,
                                                   CLIENT_RESOURCE,
                                                   tabsList,
                                                   R.layout.remote_tabs_child,
                                                   TAB_KEY,
                                                   TAB_RESOURCE));

        for (int i = 0; i < clients.size(); i++) {
            expandGroup(i);
        }
    }
}
