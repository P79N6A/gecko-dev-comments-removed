




































package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.BitmapDrawable;
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
                finish();
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
                finish();
            }
        });

        GeckoApp.registerOnTabsChangedListener(this);
        onTabsChanged();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        GeckoApp.unregisterOnTabsChangedListener(this);
    } 
   
    public void onTabsChanged() {
        if (Tabs.getInstance().getCount() == 1)
            finish();

        HashMap<Integer, Tab> tabs = Tabs.getInstance().getTabs();
        mTabsAdapter = new TabsAdapter(this, tabs);
        mList.setAdapter(mTabsAdapter);
    }

    
    private class TabsAdapter extends BaseAdapter {
	public TabsAdapter(Context context, HashMap<Integer, Tab> tabs) {
            mContext = context;
            mTabs = new ArrayList<Tab>();
            
            if (tabs != null) {
                Iterator keys = tabs.keySet().iterator();
                Tab tab;
                while (keys.hasNext()) {
                    tab = tabs.get(keys.next());
                    mTabs.add(tab);
                }
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

        @Override    
        public View getView(int position, View convertView, ViewGroup parent) {

    	    if (convertView == null)
                convertView = mInflater.inflate(R.layout.tabs_row, null);

            Tab tab = mTabs.get(position);

            RelativeLayout info = (RelativeLayout) convertView.findViewById(R.id.info);
            info.setTag("" + tab.getId());
            info.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Select", "" + v.getTag()));
                    finish();
                }
            });

            ImageView favicon = (ImageView) convertView.findViewById(R.id.favicon);

            Drawable faviconImage = tab.getFavicon();
            if (faviconImage != null)
                favicon.setImageDrawable(faviconImage);
            else
                favicon.setImageResource(R.drawable.favicon);

            TextView title = (TextView) convertView.findViewById(R.id.title);
            title.setText(tab.getDisplayTitle());
            
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
