




package org.mozilla.gecko.tabs;

import java.util.ArrayList;

import org.mozilla.gecko.Tab;
import org.mozilla.gecko.R;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;


public class TabsLayoutAdapter extends BaseAdapter {
    private Context mContext;
    private ArrayList<Tab> mTabs;
    private LayoutInflater mInflater;

    public TabsLayoutAdapter (Context context) {
        mContext = context;
        mInflater = LayoutInflater.from(mContext);
    }

    final void setTabs (ArrayList<Tab> tabs) {
        mTabs = tabs;
        notifyDataSetChanged(); 
    }

    final boolean removeTab (Tab tab) {
        boolean tabRemoved = mTabs.remove(tab);
        if (tabRemoved) {
            notifyDataSetChanged(); 
        }
        return tabRemoved;
    }

    final void clear() {
        mTabs = null;
        notifyDataSetChanged(); 
    }

    @Override
    public int getCount() {
        return (mTabs == null ? 0 : mTabs.size());
    }

    @Override
    public Tab getItem(int position) {
        return mTabs.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    final int getPositionForTab(Tab tab) {
        if (mTabs == null || tab == null)
            return -1;

        return mTabs.indexOf(tab);
    }

    @Override
    final public View getView(int position, View convertView, ViewGroup parent) {
        final View view;
        if (convertView == null) {
            view = newView(position, parent);
        } else {
            view = convertView;
        }
        final Tab tab = mTabs.get(position);
        bindView(view, tab);
        return view;
    }

    View newView(int position, ViewGroup parent) {
        final View view = mInflater.inflate(R.layout.tabs_layout_item_view, parent, false);
        final TabsLayoutItemView item = new TabsLayoutItemView(view);
        view.setTag(item);
        return view;
    }

    void bindView(View view, Tab tab) {
        TabsLayoutItemView item = (TabsLayoutItemView) view.getTag();
        item.assignValues(tab);
    }
}