




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentResolver;
import android.content.Intent;
import android.os.SystemClock;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class Tabs implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTabs";

    private Tab mSelectedTab;
    private final HashMap<Integer, Tab> mTabs = new HashMap<Integer, Tab>();
    private final CopyOnWriteArrayList<Tab> mOrder = new CopyOnWriteArrayList<Tab>();
    private boolean mRestoringSession;

    
    private volatile int mScore = 0;

    public static final int LOADURL_NONE = 0;
    public static final int LOADURL_NEW_TAB = 1;
    public static final int LOADURL_USER_ENTERED = 2;
    public static final int LOADURL_PRIVATE = 4;

    private static final int SCORE_INCREMENT_TAB_LOCATION_CHANGE = 5;
    private static final int SCORE_INCREMENT_TAB_SELECTED = 10;
    private static final int SCORE_THRESHOLD = 30;

    private static AtomicInteger sTabId = new AtomicInteger(0);

    private GeckoApp mActivity;

    private Tabs() {
        registerEventListener("SessionHistory:New");
        registerEventListener("SessionHistory:Back");
        registerEventListener("SessionHistory:Forward");
        registerEventListener("SessionHistory:Goto");
        registerEventListener("SessionHistory:Purge");
        registerEventListener("Tab:Added");
        registerEventListener("Tab:Close");
        registerEventListener("Tab:Select");
        registerEventListener("Content:LocationChange");
        registerEventListener("Session:RestoreBegin");
        registerEventListener("Session:RestoreEnd");
        registerEventListener("Reader:Added");
        registerEventListener("Reader:Removed");
        registerEventListener("Reader:Share");
    }

    public void attachToActivity(GeckoApp activity) {
        mActivity = activity;
    }

    public int getCount() {
        return mTabs.size();
    }

    private Tab addTab(int id, String url, boolean external, int parentId, String title) {
        final Tab tab = new Tab(id, url, external, parentId, title);
        mTabs.put(id, tab);
        mOrder.add(tab);

        if (!mRestoringSession) {
            mActivity.runOnUiThread(new Runnable() {
                public void run() {
                    notifyListeners(tab, TabEvents.ADDED);
                }
            });
        }

        Log.i(LOGTAG, "Added a tab with id: " + id);
        return tab;
    }

    public void removeTab(int id) {
        if (mTabs.containsKey(id)) {
            Tab tab = getTab(id);
            mOrder.remove(tab);
            mTabs.remove(id);
            tab.freeBuffer();
            Log.i(LOGTAG, "Removed a tab with id: " + id);
        }
    }

    public Tab selectTab(int id) {
        if (!mTabs.containsKey(id))
            return null;

        final Tab oldTab = getSelectedTab();
        final Tab tab = mTabs.get(id);
        
        
        if (tab == null)
            return null;

        mSelectedTab = tab;
        mActivity.runOnUiThread(new Runnable() { 
            public void run() {
                mActivity.hideFormAssistPopup();
                if (isSelectedTab(tab)) {
                    String url = tab.getURL();
                    notifyListeners(tab, TabEvents.SELECTED);

                    if (oldTab != null)
                        notifyListeners(oldTab, TabEvents.UNSELECTED);
                }
            }
        });

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Selected", String.valueOf(tab.getId())));
        return tab;
    }

    public int getIndexOf(Tab tab) {
        return mOrder.lastIndexOf(tab);
    }

    public Tab getTabAt(int index) {
        if (index >= 0 && index < mOrder.size())
            return mOrder.get(index);
        else
            return null;
    }

    public Tab getSelectedTab() {
        return mSelectedTab;
    }

    public boolean isSelectedTab(Tab tab) {
        if (mSelectedTab == null)
            return false;

        return tab == mSelectedTab;
    }

    public Tab getTab(int id) {
        if (getCount() == 0)
            return null;

        if (!mTabs.containsKey(id))
           return null;

        return mTabs.get(id);
    }

    
    public void closeTab(Tab tab) {
        closeTab(tab, getNextTab(tab));
    }

    
    public void closeTab(final Tab tab, Tab nextTab) {
        if (tab == null || nextTab == null)
            return;

        selectTab(nextTab.getId());

        int tabId = tab.getId();
        removeTab(tabId);

        mActivity.runOnUiThread(new Runnable() { 
            public void run() {
                notifyListeners(tab, TabEvents.CLOSED);
                tab.onDestroy();
            }
        });

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Closed", String.valueOf(tabId)));
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

    public Iterable<Tab> getTabsInOrder() {
        return mOrder;
    }

    public ContentResolver getContentResolver() {
        return mActivity.getContentResolver();
    }

    
    private static class TabsInstanceHolder {
        private static final Tabs INSTANCE = new Tabs();
    }

    public static Tabs getInstance() {
       return Tabs.TabsInstanceHolder.INSTANCE;
    }

    

    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if (event.startsWith("SessionHistory:")) {
                Tab tab = getTab(message.getInt("tabID"));
                if (tab != null) {
                    event = event.substring("SessionHistory:".length());
                    tab.handleSessionHistoryMessage(event, message);
                }
            } else if (event.equals("Tab:Added")) {
                Log.i(LOGTAG, "Received message from Gecko: " + SystemClock.uptimeMillis() + " - Tab:Added");

                int id = message.getInt("tabID");
                Tab tab = null;

                if (mTabs.containsKey(id)) {
                    tab = mTabs.get(id);
                } else {
                    tab = addTab(id,
                                 message.isNull("uri") ? null : message.getString("uri"),
                                 message.getBoolean("external"),
                                 message.getInt("parentId"),
                                 message.getString("title"));
                }

                if (message.getBoolean("selected"))
                    selectTab(tab.getId());
                if (message.getBoolean("delayLoad"))
                    tab.setState(Tab.STATE_DELAYED);
                if (message.getBoolean("desktopMode"))
                    tab.setDesktopMode(true);
            } else if (event.equals("Tab:Close")) {
                Tab tab = getTab(message.getInt("tabID"));
                closeTab(tab);
            } else if (event.equals("Tab:Select")) {
                selectTab(message.getInt("tabID"));
            } else if (event.equals("Content:LocationChange")) {
                Tab tab = getTab(message.getInt("tabID"));
                if (tab != null) {
                    tab.handleLocationChange(message);
                }
            } else if (event.equals("Session:RestoreBegin")) {
                mRestoringSession = true;
            } else if (event.equals("Session:RestoreEnd")) {
                mRestoringSession = false;
                mActivity.runOnUiThread(new Runnable() {
                    public void run() {
                        notifyListeners(null, TabEvents.RESTORED);
                    }
                });
            } else if (event.equals("Reader:Added")) {
                final boolean success = message.getBoolean("success");
                final String title = message.getString("title");
                final String url = message.getString("url");
                handleReaderAdded(success, title, url);
            } else if (event.equals("Reader:Removed")) {
                final String url = message.getString("url");
                handleReaderRemoved(url);
            } else if (event.equals("Reader:Share")) {
                final String title = message.getString("title");
                final String url = message.getString("url");

                GeckoAppShell.openUriExternal(url, "text/plain", "", "",
                                              Intent.ACTION_SEND, title);
            }
        } catch (Exception e) { 
            Log.i(LOGTAG, "handleMessage throws " + e + " for message: " + event);
        }
    }

    void handleReaderAdded(boolean success, final String title, final String url) {
        if (!success) {
            mActivity.showToast(R.string.reading_list_failed, Toast.LENGTH_SHORT);
            return;
        }

        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                BrowserDB.addReadingListItem(getContentResolver(), title, url);
                mActivity.showToast(R.string.reading_list_added, Toast.LENGTH_SHORT);
            }
        });
    }

    void handleReaderRemoved(final String url) {
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                BrowserDB.removeReadingListItemWithURL(getContentResolver(), url);
                mActivity.showToast(R.string.reading_list_removed, Toast.LENGTH_SHORT);
            }
        });
    }

    public void refreshThumbnails() {
        Iterator<Tab> iterator = mTabs.values().iterator();
        while (iterator.hasNext()) {
            final Tab tab = iterator.next();
            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    mActivity.getAndProcessThumbnailForTab(tab);
                }
            });
        }
    }

    public interface OnTabsChangedListener {
        public void onTabChanged(Tab tab, TabEvents msg, Object data);
    }
    
    private static ArrayList<OnTabsChangedListener> mTabsChangedListeners;

    public static void registerOnTabsChangedListener(OnTabsChangedListener listener) {
        if (mTabsChangedListeners == null)
            mTabsChangedListeners = new ArrayList<OnTabsChangedListener>();
        
        mTabsChangedListeners.add(listener);
    }

    public static void unregisterOnTabsChangedListener(OnTabsChangedListener listener) {
        if (mTabsChangedListeners == null)
            return;
        
        mTabsChangedListeners.remove(listener);
    }

    public enum TabEvents {
        CLOSED,
        START,
        LOADED,
        LOAD_ERROR,
        STOP,
        FAVICON,
        THUMBNAIL,
        TITLE,
        SELECTED,
        UNSELECTED,
        ADDED,
        RESTORED,
        LOCATION_CHANGE,
        MENU_UPDATED
    }

    public void notifyListeners(Tab tab, TabEvents msg) {
        notifyListeners(tab, msg, "");
    }

    public void notifyListeners(Tab tab, TabEvents msg, Object data) {
        onTabChanged(tab, msg, data);

        if (mTabsChangedListeners == null)
            return;

        Iterator<OnTabsChangedListener> items = mTabsChangedListeners.iterator();
        while (items.hasNext()) {
            items.next().onTabChanged(tab, msg, data);
        }
    }

    private void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case LOCATION_CHANGE:
                mScore += SCORE_INCREMENT_TAB_LOCATION_CHANGE;
                break;

            
            
            
            
            case SELECTED:
                mScore += SCORE_INCREMENT_TAB_SELECTED;
            case UNSELECTED:
                tab.onChange();
                break;
        }

        if (mScore > SCORE_THRESHOLD) {
            persistAllTabs();
            mScore = 0;
        }
    }

    
    public void persistAllTabs() {
        final Iterable<Tab> tabs = getTabsInOrder();
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                TabsAccessor.persistLocalTabs(getContentResolver(), tabs);
            }
        });
    }

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    




    public void loadUrl(String url) {
        loadUrl(url, LOADURL_NONE);
    }

    





    public void loadUrl(String url, int flags) {
        loadUrl(url, null, -1, flags);
    }

    








    public void loadUrl(String url, String searchEngine, int parentId, int flags) {
        JSONObject args = new JSONObject();
        int tabId = -1;

        try {
            boolean isPrivate = (flags & LOADURL_PRIVATE) != 0;

            args.put("url", url);
            args.put("engine", searchEngine);
            args.put("parentId", parentId);
            args.put("userEntered", (flags & LOADURL_USER_ENTERED) != 0);
            args.put("newTab", (flags & LOADURL_NEW_TAB) != 0);
            args.put("isPrivate", isPrivate);

            if ((flags & LOADURL_NEW_TAB) != 0) {
                tabId = getNextTabId();
                args.put("tabID", tabId);
                addTab(tabId, null, false, parentId, url);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "error building JSON arguments");
        }

        Log.d(LOGTAG, "Sending message to Gecko: " + SystemClock.uptimeMillis() + " - Tab:Load");
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Load", args.toString()));

        if (tabId != -1) {
            selectTab(tabId);
        }
    }

    








    public void loadUrlInTab(String url) {
        Iterable<Tab> tabs = getTabsInOrder();
        for (Tab tab : tabs) {
            if (url.equals(tab.getURL())) {
                selectTab(tab.getId());
                return;
            }
        }

        loadUrl(url, null, getSelectedTab().getId(), LOADURL_NEW_TAB);
    }

    




    public static int getNextTabId() {
        return sTabId.getAndIncrement();
    }
}
