




































package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.Iterator;

import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class TabsTray extends Activity implements GeckoApp.OnTabsChangedListener {

    private ListView mList;
    private TabsAdapter mTabsAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.tabs_tray);

        mList = (ListView) findViewById(R.id.list);

        LinearLayout addTab = (LinearLayout) findViewById(R.id.add_tab);
        addTab.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                GeckoApp.mAppContext.addTab();
                finishActivity();
            }
        });

        
        LinearLayout lastDivider = new LinearLayout(this);
        lastDivider.setOrientation(LinearLayout.HORIZONTAL);
        lastDivider.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, mList.getDividerHeight()));
        lastDivider.setBackgroundDrawable(mList.getDivider());
        addTab.addView(lastDivider, 0);
        
        LinearLayout container = (LinearLayout) findViewById(R.id.container);
        container.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                finishActivity();
            }
        });

        GeckoApp.registerOnTabsChangedListener(this);
        onTabsChanged(null);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        GeckoApp.unregisterOnTabsChangedListener(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        
        
        if (hasFocus) {
            int position = mTabsAdapter.getPositionForTab(Tabs.getInstance().getSelectedTab());
            if (position != -1) 
                mList.smoothScrollToPosition(position);
        }
    } 
   
    public void onTabsChanged(Tab tab) {
        if (Tabs.getInstance().getCount() == 1)
            finishActivity();

        if (mTabsAdapter == null) {
            mTabsAdapter = new TabsAdapter(this, Tabs.getInstance().getTabsInOrder());
            mList.setAdapter(mTabsAdapter);
            return;
        }
        
        int position = mTabsAdapter.getPositionForTab(tab);
        if (position == -1)
            return;

        if (Tabs.getInstance().getIndexOf(tab) == -1) {
            mTabsAdapter = new TabsAdapter(this, Tabs.getInstance().getTabsInOrder());
            mList.setAdapter(mTabsAdapter);
        } else {
            View view = mList.getChildAt(position - mList.getFirstVisiblePosition());
            mTabsAdapter.assignValues(view, tab);
        }
    }

    void finishActivity() {
        finish();
        overridePendingTransition(0, R.anim.shrink_fade_out);
    }

    
    private class TabsAdapter extends BaseAdapter {
	public TabsAdapter(Context context, ArrayList<Tab> tabs) {
            mContext = context;
            mTabs = new ArrayList<Tab>();
            for (int i = 0; i < tabs.size(); i++) {
                mTabs.add(tabs.get(i));
            }

            mInflater = LayoutInflater.from(mContext);
        }

        @Override    
        public int getCount() {
            return mTabs.size();
        }
    
        @Override    
        public Tab getItem(int position) {
            return mTabs.get(position);
        }

        @Override    
        public long getItemId(int position) {
            return position;
        }

        public int getPositionForTab(Tab tab) {
            if (mTabs == null || tab == null)
                return -1;

            return mTabs.indexOf(tab);
        }

        public void assignValues(View view, Tab tab) {
            if (view == null || tab == null)
                return;

            ImageView favicon = (ImageView) view.findViewById(R.id.favicon);

            Drawable faviconImage = tab.getFavicon();
            if (faviconImage != null)
                favicon.setImageDrawable(faviconImage);
            else
                favicon.setImageResource(R.drawable.favicon);

            TextView title = (TextView) view.findViewById(R.id.title);
            title.setText(tab.getDisplayTitle());

            if (Tabs.getInstance().isSelectedTab(tab))
                title.setTypeface(title.getTypeface(), Typeface.BOLD);
        }

        @Override    
        public View getView(int position, View convertView, ViewGroup parent) {

            convertView = mInflater.inflate(R.layout.tabs_row, null);
            
            Tab tab = mTabs.get(position);

            RelativeLayout info = (RelativeLayout) convertView.findViewById(R.id.info);
            info.setTag("" + tab.getId());
            info.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Select", "" + v.getTag()));
                    finishActivity();
                }
            });

            assignValues(convertView, tab);
            
            ImageButton close = (ImageButton) convertView.findViewById(R.id.close);
            if (mTabs.size() > 1) {
                close.setTag("" + tab.getId());
                close.setOnClickListener(new Button.OnClickListener() {
                    public void onClick(View v) {
                        int tabId = Integer.parseInt("" + v.getTag());
                        Tabs tabs = Tabs.getInstance();
                        Tab tab = tabs.getTab(tabId);

                        if (tabs.isSelectedTab(tab)) {
                            int index = tabs.getIndexOf(tab);
                            if (index >= 1)
                                index--;
                            else
                                index = 1;
                            int id = tabs.getTabAt(index).getId();
                            GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Select", "" + id));
                            GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Close", "" + v.getTag()));
                        } else {
                            GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Close", "" + v.getTag()));
                            GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Select", "" + tabs.getSelectedTabId()));
                        }
                    }
                });
            } else {
                close.setVisibility(View.GONE);
            }

            return convertView;
        }

        @Override
        public void notifyDataSetChanged() {
        }

        @Override
        public void notifyDataSetInvalidated() {
        }
    
        private Context mContext;
        private ArrayList<Tab> mTabs;
        private LayoutInflater mInflater;
    }
}
