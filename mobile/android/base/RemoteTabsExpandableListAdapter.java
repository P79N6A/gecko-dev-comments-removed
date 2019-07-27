



package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.TabsAccessor.RemoteClient;
import org.mozilla.gecko.TabsAccessor.RemoteTab;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ImageView;
import android.widget.TextView;








public class RemoteTabsExpandableListAdapter extends BaseExpandableListAdapter {
    protected final ArrayList<RemoteClient> clients;
    protected int groupLayoutId;
    protected int childLayoutId;

    











    public RemoteTabsExpandableListAdapter(int groupLayoutId, int childLayoutId, List<RemoteClient> clients) {
        this.groupLayoutId = groupLayoutId;
        this.childLayoutId = childLayoutId;
        this.clients = new ArrayList<TabsAccessor.RemoteClient>();
        if (clients != null) {
            this.clients.addAll(clients);
        }
    }

    public void replaceClients(List<RemoteClient> clients) {
        this.clients.clear();
        if (clients != null) {
            this.clients.addAll(clients);
            this.notifyDataSetChanged();
        } else {
            this.notifyDataSetInvalidated();
        }
    }

    @Override
    public boolean hasStableIds() {
        return false; 
    }

    @Override
    public long getGroupId(int groupPosition) {
        return clients.get(groupPosition).guid.hashCode();
    }

    @Override
    public int getGroupCount() {
        return clients.size();
    }

    @Override
    public Object getGroup(int groupPosition) {
        return clients.get(groupPosition);
    }

    @Override
    public int getChildrenCount(int groupPosition) {
        return clients.get(groupPosition).tabs.size();
    }

    @Override
    public View getGroupView(int groupPosition, boolean isExpanded, View convertView, ViewGroup parent) {
        final Context context = parent.getContext();
        final View view;
        if (convertView != null) {
            view = convertView;
        } else {
            final LayoutInflater inflater = LayoutInflater.from(context);
            view = inflater.inflate(groupLayoutId, parent, false);
        }

        final RemoteClient client = clients.get(groupPosition);

        final TextView nameView = (TextView) view.findViewById(R.id.client);
        nameView.setText(client.name);

        final TextView lastModifiedView = (TextView) view.findViewById(R.id.last_synced);
        final long now = System.currentTimeMillis();
        lastModifiedView.setText(TabsAccessor.getLastSyncedString(context, now, client.lastModified));

        
        
        
        final ImageView deviceTypeView = (ImageView) view.findViewById(R.id.device_type);
        if (deviceTypeView != null) {
            if ("desktop".equals(client.deviceType)) {
                deviceTypeView.setBackgroundResource(R.drawable.sync_desktop);
            } else {
                deviceTypeView.setBackgroundResource(R.drawable.sync_mobile);
            }
        }

        return view;
    }

    @Override
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return true;
    }

    @Override
    public Object getChild(int groupPosition, int childPosition) {
        return clients.get(groupPosition).tabs.get(childPosition);
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return clients.get(groupPosition).tabs.get(childPosition).hashCode();
    }

    @Override
    public View getChildView(int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent) {
        final Context context = parent.getContext();
        final View view;
        if (convertView != null) {
            view = convertView;
        } else {
            final LayoutInflater inflater = LayoutInflater.from(context);
            view = inflater.inflate(childLayoutId, parent, false);
        }

        final RemoteClient client = clients.get(groupPosition);
        final RemoteTab tab = client.tabs.get(childPosition);

        final TextView titleView = (TextView) view.findViewById(R.id.title);
        titleView.setText(TextUtils.isEmpty(tab.title) ? tab.url : tab.title);

        final TextView urlView = (TextView) view.findViewById(R.id.url);
        urlView.setText(tab.url);

        return view;
    }
}
