




package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import java.util.ArrayList;


public class TabsLayoutAdapter extends BaseAdapter {
    public static final String LOGTAG = "Gecko" + TabsLayoutAdapter.class.getSimpleName();

    private final Context mContext;
    private final int mTabLayoutId;
    private ArrayList<Tab> mTabs;
    private final LayoutInflater mInflater;

    public TabsLayoutAdapter (Context context, int tabLayoutId) {
        mContext = context;
        mInflater = LayoutInflater.from(mContext);
        mTabLayoutId = tabLayoutId;
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
    public boolean isEnabled(int position) {
        return true;
    }

    @Override
    final public TabsLayoutItemView getView(int position, View convertView, ViewGroup parent) {
        final TabsLayoutItemView view;
        if (convertView == null) {
            view = newView(position, parent);
        } else {
            view = (TabsLayoutItemView) convertView;
        }
        final Tab tab = mTabs.get(position);
        bindView(view, tab);
        return view;
    }

    TabsLayoutItemView newView(int position, ViewGroup parent) {
        return (TabsLayoutItemView) mInflater.inflate(mTabLayoutId, parent, false);
    }

    void bindView(TabsLayoutItemView view, Tab tab) {
        view.assignValues(tab);
    }
}