



package org.mozilla.gecko;



import android.text.format.DateUtils;
import org.mozilla.gecko.db.RemoteClient;
import org.mozilla.gecko.db.RemoteTab;
import org.mozilla.gecko.home.TwoLinePageRow;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.List;








public class RemoteTabsExpandableListAdapter extends BaseExpandableListAdapter {
    


    private static final Date EARLIEST_VALID_SYNCED_DATE;
    static {
        final Calendar c = GregorianCalendar.getInstance();
        c.set(2000, Calendar.JANUARY, 1, 0, 0, 0);
        EARLIEST_VALID_SYNCED_DATE = c.getTime();
    }
    protected final ArrayList<RemoteClient> clients;
    private final boolean showGroupIndicator;
    protected int groupLayoutId;
    protected int childLayoutId;

    public static class GroupViewHolder {
        final TextView nameView;
        final TextView lastModifiedView;
        final ImageView deviceTypeView;
        final ImageView deviceExpandedView;

        public GroupViewHolder(View view) {
            nameView = (TextView) view.findViewById(R.id.client);
            lastModifiedView = (TextView) view.findViewById(R.id.last_synced);
            deviceTypeView = (ImageView) view.findViewById(R.id.device_type);
            deviceExpandedView = (ImageView) view.findViewById(R.id.device_expanded);
        }
    }

    












    public RemoteTabsExpandableListAdapter(int groupLayoutId, int childLayoutId, List<RemoteClient> clients, boolean showGroupIndicator) {
        this.groupLayoutId = groupLayoutId;
        this.childLayoutId = childLayoutId;
        this.clients = new ArrayList<>();
        if (clients != null) {
            this.clients.addAll(clients);
        }
        this.showGroupIndicator = showGroupIndicator;
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
            final GroupViewHolder holder = new GroupViewHolder(view);
            view.setTag(holder);
        }

        final RemoteClient client = clients.get(groupPosition);
        updateClientsItemView(isExpanded, context, view, client);

        return view;
    }

    public void updateClientsItemView(final boolean isExpanded, final Context context, final View view, final RemoteClient client) {
        final GroupViewHolder holder = (GroupViewHolder) view.getTag();

        
        
        
        final int deviceTypeResId;
        final int textColorResId;
        final int deviceExpandedResId;

        if (isExpanded && !client.tabs.isEmpty()) {
            deviceTypeResId = "desktop".equals(client.deviceType) ? R.drawable.sync_desktop : R.drawable.sync_mobile;
            textColorResId = R.color.placeholder_active_grey;
            deviceExpandedResId = showGroupIndicator ? R.drawable.home_group_expanded : R.drawable.home_group_collapsed;
        } else {
            deviceTypeResId = "desktop".equals(client.deviceType) ? R.drawable.sync_desktop_inactive : R.drawable.sync_mobile_inactive;
            textColorResId = R.color.tabs_tray_icon_grey;
            deviceExpandedResId = showGroupIndicator ? R.drawable.home_group_collapsed : 0;
        }

        
        holder.nameView.setText(client.name);
        holder.nameView.setTextColor(context.getResources().getColor(textColorResId));

        final long now = System.currentTimeMillis();

        
        
        final GeckoProfile profile = GeckoProfile.get(context);
        holder.lastModifiedView.setText(this.getLastSyncedString(context, now, client.lastModified));

        
        
        
        if (holder.deviceTypeView != null) {
            holder.deviceTypeView.setImageResource(deviceTypeResId);
        }

        if (holder.deviceExpandedView != null) {
            
            holder.deviceExpandedView.setImageResource(client.tabs.isEmpty() ? 0 : deviceExpandedResId);
        }
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

        
        
        
        if (view instanceof TwoLinePageRow) {
            ((TwoLinePageRow) view).update(tab.title, tab.url);
        } else {
            final TextView titleView = (TextView) view.findViewById(R.id.title);
            titleView.setText(TextUtils.isEmpty(tab.title) ? tab.url : tab.title);

            final TextView urlView = (TextView) view.findViewById(R.id.url);
            urlView.setText(tab.url);
        }

        return view;
    }

    






    public String getLastSyncedString(Context context, long now, long time) {
        if (new Date(time).before(EARLIEST_VALID_SYNCED_DATE)) {
            return context.getString(R.string.remote_tabs_never_synced);
        }
        final CharSequence relativeTimeSpanString = DateUtils.getRelativeTimeSpanString(time, now, DateUtils.MINUTE_IN_MILLIS);
        return context.getResources().getString(R.string.remote_tabs_last_synced, relativeTimeSpanString);
    }
}
