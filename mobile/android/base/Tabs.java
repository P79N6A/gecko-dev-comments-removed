




































package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentResolver;
import android.util.Log;

public class Tabs implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTabs";

    private static int selectedTab = -1;
    private HashMap<Integer, Tab> tabs;
    private ArrayList<Tab> order;
    private ContentResolver resolver;

    private Tabs() {
        tabs = new HashMap<Integer, Tab>();
        order = new ArrayList<Tab>();
        GeckoAppShell.registerGeckoEventListener("SessionHistory:New", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Back", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Forward", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Goto", this);
        GeckoAppShell.registerGeckoEventListener("SessionHistory:Purge", this);
    }

    public int getCount() {
        return tabs.size();
    }

    public Tab addTab(JSONObject params) throws JSONException {
        int id = params.getInt("tabID");
        if (tabs.containsKey(id))
           return tabs.get(id);

        String url = params.getString("uri");
        Boolean external = params.getBoolean("external");
        int parentId = params.getInt("parentId");
        String title = params.getString("title");

        Tab tab = new Tab(id, url, external, parentId, title);
        tabs.put(id, tab);
        order.add(tab);
        Log.i(LOGTAG, "Added a tab with id: " + id + ", url: " + url);
        return tab;
    }

    public void removeTab(int id) {
        if (tabs.containsKey(id)) {
            order.remove(getTab(id));
            tabs.remove(id);
            Log.i(LOGTAG, "Removed a tab with id: " + id);
        }
    }

    public Tab selectTab(int id) {
        if (!tabs.containsKey(id))
            return null;
 
        selectedTab = id;
        return tabs.get(id);
    }

    public int getIndexOf(Tab tab) {
        return order.lastIndexOf(tab);
    }

    public Tab getTabAt(int index) {
        if (index >= 0 && index < order.size())
            return order.get(index);
        else
            return null;
    }

    public Tab getSelectedTab() {
        return tabs.get(selectedTab);
    }

    public int getSelectedTabId() {
        return selectedTab;
    }

    public boolean isSelectedTab(Tab tab) {
        return (tab.getId() == selectedTab);
    }

    public Tab getTab(int id) {
        if (getCount() == 0)
            return null;

        if (!tabs.containsKey(id))
           return null;

        return tabs.get(id);
    }

    
    public void closeTab(Tab tab) {
        closeTab(tab, getNextTab(tab));
    }

    
    public void closeTab(Tab tab, Tab nextTab) {
        if (tab == null || nextTab == null)
            return;

        
        
        GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Select", String.valueOf(nextTab.getId())));

        int tabId = tab.getId();
        removeTab(tabId);
        tab.removeAllDoorHangers();

        final Tab closedTab = tab;
        GeckoApp.mAppContext.mMainHandler.post(new Runnable() { 
            public void run() {
                GeckoApp.mAppContext.onTabsChanged(closedTab);
                GeckoApp.mBrowserToolbar.updateTabs(Tabs.getInstance().getCount());
                GeckoApp.mDoorHangerPopup.updatePopup();
            }
        });

        
        GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Closed", String.valueOf(tabId)));
    }

    
    public Tab getNextTab(Tab tab) {
        Tab selectedTab = getSelectedTab();
        if (selectedTab != tab)
            return selectedTab;

        int index = getIndexOf(tab);
        Tab nextTab = getTabAt(index + 1);
        if (nextTab == null)
            nextTab = getTabAt(index - 1);

        Tab parent = getTab(tab.getParentId());
        if (parent != null) {
            
            if (nextTab != null && nextTab.getParentId() == tab.getParentId())
                return nextTab;
            else
                return parent;
        }
        return nextTab;
    }

    public HashMap<Integer, Tab> getTabs() {
        if (getCount() == 0)
            return null;

        return tabs;
    }
    
    public ArrayList<Tab> getTabsInOrder() {
        if (getCount() == 0)
            return null;

        return order;
    }

    public void setContentResolver(ContentResolver resolver) {
        this.resolver = resolver;
    }

    public ContentResolver getContentResolver() {
        return resolver;
    }

    
    private static class TabsInstanceHolder {
        private static final Tabs INSTANCE = new Tabs();
    }

    public static Tabs getInstance() {
       return Tabs.TabsInstanceHolder.INSTANCE;
    }

    

    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.startsWith("SessionHistory:")) {
                Tab tab = getTab(message.getInt("tabID"));
                if (tab != null) {
                    event = event.substring("SessionHistory:".length());
                    tab.handleSessionHistoryMessage(event, message);
                }
            }
        } catch (Exception e) { 
            Log.i(LOGTAG, "handleMessage throws " + e + " for message: " + event);
        }
    }

    public void refreshThumbnails() {
        Iterator<Tab> iterator = tabs.values().iterator();
        while (iterator.hasNext())
            GeckoApp.mAppContext.getAndProcessThumbnailForTab(iterator.next());
    }
}
