




package org.mozilla.gecko.tabs;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import java.util.ArrayList;
import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;

class TabStripAdapter extends BaseAdapter {
    private static final String LOGTAG = "GeckoTabStripAdapter";

    private final Context context;
    private List<Tab> tabs;

    public TabStripAdapter(Context context) {
        this.context = context;
    }

    @Override
    public Tab getItem(int position) {
        return (tabs != null &&
                position >= 0 &&
                position < tabs.size() ? tabs.get(position) : null);
    }

    @Override
    public long getItemId(int position) {
        final Tab tab = getItem(position);
        return (tab != null ? tab.getId() : -1);
    }

    @Override
    public boolean hasStableIds() {
        return true;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final TabStripItemView item;
        if (convertView == null) {
            item = (TabStripItemView)
                    LayoutInflater.from(context).inflate(R.layout.tab_strip_item, parent, false);
        } else {
            item = (TabStripItemView) convertView;
        }

        final Tab tab = tabs.get(position);
        item.updateFromTab(tab);

        return item;
    }

    @Override
    public int getCount() {
        return (tabs != null ? tabs.size() : 0);
    }

    int getPositionForTab(Tab tab) {
        if (tabs == null || tab == null) {
            return -1;
        }

        return tabs.indexOf(tab);
    }

    void removeTab(Tab tab) {
        if (tabs == null) {
            return;
        }

        tabs.remove(tab);
        notifyDataSetChanged();
    }

    void refresh(List<Tab> tabs) {
        
        
        this.tabs = tabs;
        notifyDataSetChanged();
    }

    void clear() {
        tabs = null;
        notifyDataSetInvalidated();
    }
}