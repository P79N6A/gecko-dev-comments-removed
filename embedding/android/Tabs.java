




































package org.mozilla.gecko;

import java.util.*;

import android.graphics.drawable.*;
import android.util.Log;

public class Tabs {

    private static final String LOG_FILE_NAME = "Tabs";
    private static int selectedTab = -1;
    private HashMap<Integer, Tab> tabs;

    private Tabs() {
        tabs = new HashMap<Integer, Tab>();
    }

    public int getCount() {
        return tabs.size();
    }

    public Tab addTab(int id, String url) {
        if (tabs.containsKey(id))
           return tabs.get(id);

        Tab tab = new Tab(id, url);
        tabs.put(id, tab);
        Log.i(LOG_FILE_NAME, "Added a tab with id: " + id + ", url: " + url);
        selectedTab = id;
        return tab;
    }

    public void removeTab(int id) {
        if (tabs.containsKey(id))
            tabs.remove(id);
        Log.i(LOG_FILE_NAME, "Removed a tab with id: " + id);
    }

    public Tab switchToTab(int id) {
        if (!tabs.containsKey(id))
            return null;
 
        selectedTab = id;
        return tabs.get(id);
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

    public HashMap<Integer, Tab> getTabs() {
        if (getCount() == 0)
            return null;

        return tabs;
    }

    
    private static class TabsInstanceHolder {
        private static final Tabs INSTANCE = new Tabs();
    }

    public static Tabs getInstance() {
       return Tabs.TabsInstanceHolder.INSTANCE;
    }
}
