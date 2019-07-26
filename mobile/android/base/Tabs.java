




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.OnAccountsUpdateListener;
import android.content.ContentResolver;
import android.database.ContentObserver;
import android.graphics.Color;
import android.net.Uri;
import android.util.Log;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class Tabs implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTabs";

    
    private final CopyOnWriteArrayList<Tab> mOrder = new CopyOnWriteArrayList<Tab>();

    
    
    private volatile Tab mSelectedTab;

    
    private final HashMap<Integer, Tab> mTabs = new HashMap<Integer, Tab>();

    
    private volatile int mScore = 0;

    private AccountManager mAccountManager;
    private OnAccountsUpdateListener mAccountListener = null;

    public static final int LOADURL_NONE = 0;
    public static final int LOADURL_NEW_TAB = 1;
    public static final int LOADURL_USER_ENTERED = 2;
    public static final int LOADURL_PRIVATE = 4;
    public static final int LOADURL_PINNED = 8;
    public static final int LOADURL_DELAY_LOAD = 16;
    public static final int LOADURL_DESKTOP = 32;
    public static final int LOADURL_BACKGROUND = 64;

    private static final int SCORE_INCREMENT_TAB_LOCATION_CHANGE = 5;
    private static final int SCORE_INCREMENT_TAB_SELECTED = 10;
    private static final int SCORE_THRESHOLD = 30;

    private static AtomicInteger sTabId = new AtomicInteger(0);
    private volatile boolean mInitialTabsAdded;

    private GeckoApp mActivity;
    private ContentObserver mContentObserver;

    private Tabs() {
        registerEventListener("Session:RestoreEnd");
        registerEventListener("SessionHistory:New");
        registerEventListener("SessionHistory:Back");
        registerEventListener("SessionHistory:Forward");
        registerEventListener("SessionHistory:Goto");
        registerEventListener("SessionHistory:Purge");
        registerEventListener("Tab:Added");
        registerEventListener("Tab:Close");
        registerEventListener("Tab:Select");
        registerEventListener("Content:LocationChange");
        registerEventListener("Content:SecurityChange");
        registerEventListener("Content:ReaderEnabled");
        registerEventListener("Content:StateChange");
        registerEventListener("Content:LoadError");
        registerEventListener("Content:PageShow");
        registerEventListener("DOMContentLoaded");
        registerEventListener("DOMTitleChanged");
        registerEventListener("DOMLinkAdded");
        registerEventListener("DesktopMode:Changed");
    }

    public synchronized void attachToActivity(GeckoApp activity) {
        if (mActivity == activity) {
            return;
        }

        if (mActivity != null) {
            detachFromActivity(mActivity);
        }

        mActivity = activity;
        mAccountManager = AccountManager.get(mActivity);

        mAccountListener = new OnAccountsUpdateListener() {
            @Override
            public void onAccountsUpdated(Account[] accounts) {
                persistAllTabs();
            }
        };

        
        mAccountManager.addOnAccountsUpdatedListener(mAccountListener, ThreadUtils.getBackgroundHandler(), false);

        if (mContentObserver != null) {
            BrowserDB.registerBookmarkObserver(getContentResolver(), mContentObserver);
        }
    }

    
    
    
    
    public synchronized void detachFromActivity(GeckoApp activity) {
        if (mContentObserver != null) {
            BrowserDB.unregisterContentObserver(getContentResolver(), mContentObserver);
        }

        if (mAccountListener != null) {
            mAccountManager.removeOnAccountsUpdatedListener(mAccountListener);
            mAccountListener = null;
        }
    }

    









    public synchronized int getDisplayCount() {
        
        
        boolean getPrivate = mSelectedTab != null && mSelectedTab.isPrivate();
        int count = 0;
        for (Tab tab : mOrder) {
            if (tab.isPrivate() == getPrivate) {
                count++;
            }
        }
        return count;
    }

    
    private void lazyRegisterBookmarkObserver() {
        if (mContentObserver == null) {
            mContentObserver = new ContentObserver(null) {
                @Override
                public void onChange(boolean selfChange) {
                    for (Tab tab : mOrder) {
                        tab.updateBookmark();
                    }
                }
            };
            BrowserDB.registerBookmarkObserver(getContentResolver(), mContentObserver);
        }
    }

    private Tab addTab(int id, String url, boolean external, int parentId, String title, boolean isPrivate) {
        final Tab tab = isPrivate ? new PrivateTab(id, url, external, parentId, title) :
                                    new Tab(id, url, external, parentId, title);
        synchronized (this) {
            lazyRegisterBookmarkObserver();
            mTabs.put(id, tab);
            mOrder.add(tab);
        }

        
        if (mInitialTabsAdded) {
            notifyListeners(tab, TabEvents.ADDED);
        }

        return tab;
    }

    public synchronized void removeTab(int id) {
        if (mTabs.containsKey(id)) {
            Tab tab = getTab(id);
            mOrder.remove(tab);
            mTabs.remove(id);
        }
    }

    public synchronized Tab selectTab(int id) {
        if (!mTabs.containsKey(id))
            return null;

        final Tab oldTab = getSelectedTab();
        final Tab tab = mTabs.get(id);

        
        
        if (tab == null || oldTab == tab) {
            return null;
        }

        mSelectedTab = tab;
        notifyListeners(tab, TabEvents.SELECTED);

        if (oldTab != null) {
            notifyListeners(oldTab, TabEvents.UNSELECTED);
        }

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Selected", String.valueOf(tab.getId())));
        return tab;
    }

    private int getIndexOf(Tab tab) {
        return mOrder.lastIndexOf(tab);
    }

    private Tab getNextTabFrom(Tab tab, boolean getPrivate) {
        int numTabs = mOrder.size();
        int index = getIndexOf(tab);
        for (int i = index + 1; i < numTabs; i++) {
            Tab next = mOrder.get(i);
            if (next.isPrivate() == getPrivate) {
                return next;
            }
        }
        return null;
    }

    private Tab getPreviousTabFrom(Tab tab, boolean getPrivate) {
        int numTabs = mOrder.size();
        int index = getIndexOf(tab);
        for (int i = index - 1; i >= 0; i--) {
            Tab prev = mOrder.get(i);
            if (prev.isPrivate() == getPrivate) {
                return prev;
            }
        }
        return null;
    }

    







    public Tab getSelectedTab() {
        return mSelectedTab;
    }

    public boolean isSelectedTab(Tab tab) {
        return tab != null && tab == mSelectedTab;
    }

    public synchronized Tab getTab(int id) {
        if (mTabs.size() == 0)
            return null;

        if (!mTabs.containsKey(id))
           return null;

        return mTabs.get(id);
    }

    
    public synchronized void closeTab(Tab tab) {
        closeTab(tab, getNextTab(tab));
    }

    
    public synchronized void closeTab(final Tab tab, Tab nextTab) {
        if (tab == null)
            return;

        if (nextTab == null)
            nextTab = loadUrl("about:home", LOADURL_NEW_TAB);

        selectTab(nextTab.getId());

        int tabId = tab.getId();
        removeTab(tabId);

        tab.onDestroy();

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Closed", String.valueOf(tabId)));
    }

    
    public Tab getNextTab(Tab tab) {
        Tab selectedTab = getSelectedTab();
        if (selectedTab != tab)
            return selectedTab;

        boolean getPrivate = tab.isPrivate();
        Tab nextTab = getNextTabFrom(tab, getPrivate);
        if (nextTab == null)
            nextTab = getPreviousTabFrom(tab, getPrivate);
        if (nextTab == null && getPrivate) {
            
            Tab lastTab = mOrder.get(mOrder.size() - 1);
            nextTab = getPreviousTabFrom(lastTab, false);
        }

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

    



    private synchronized GeckoApp getActivity() {
        if (mActivity == null) {
            throw new IllegalStateException("Tabs not initialized with a GeckoApp instance.");
        }
        return mActivity;
    }

    public ContentResolver getContentResolver() {
        return getActivity().getContentResolver();
    }

    
    private static class TabsInstanceHolder {
        private static final Tabs INSTANCE = new Tabs();
    }

    public static Tabs getInstance() {
       return Tabs.TabsInstanceHolder.INSTANCE;
    }

    

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Session:RestoreEnd")) {
                notifyListeners(null, TabEvents.RESTORED);
                return;
            }

            
            int id = message.getInt("tabID");
            Tab tab = getTab(id);

            
            if (event.equals("Tab:Added")) {
                String url = message.isNull("uri") ? null : message.getString("uri");

                if (message.getBoolean("stub")) {
                    if (tab == null) {
                        
                        return;
                    }
                    tab.updateURL(url);
                } else {
                    tab = addTab(id, url, message.getBoolean("external"),
                                          message.getInt("parentId"),
                                          message.getString("title"),
                                          message.getBoolean("isPrivate"));
                }

                if (message.getBoolean("selected"))
                    selectTab(id);
                if (message.getBoolean("delayLoad"))
                    tab.setState(Tab.STATE_DELAYED);
                if (message.getBoolean("desktopMode"))
                    tab.setDesktopMode(true);
                return;
            }

            
            if (tab == null)
                return;

            if (event.startsWith("SessionHistory:")) {
                event = event.substring("SessionHistory:".length());
                tab.handleSessionHistoryMessage(event, message);
            } else if (event.equals("Tab:Close")) {
                closeTab(tab);
            } else if (event.equals("Tab:Select")) {
                selectTab(tab.getId());
            } else if (event.equals("Content:LocationChange")) {
                tab.handleLocationChange(message);
            } else if (event.equals("Content:SecurityChange")) {
                tab.updateIdentityData(message.getJSONObject("identity"));
                notifyListeners(tab, TabEvents.SECURITY_CHANGE);
            } else if (event.equals("Content:ReaderEnabled")) {
                tab.setReaderEnabled(true);
                notifyListeners(tab, TabEvents.READER_ENABLED);
            } else if (event.equals("Content:StateChange")) {
                int state = message.getInt("state");
                if ((state & GeckoAppShell.WPL_STATE_IS_NETWORK) != 0) {
                    if ((state & GeckoAppShell.WPL_STATE_START) != 0) {
                        boolean showProgress = message.getBoolean("showProgress");
                        tab.handleDocumentStart(showProgress, message.getString("uri"));
                        notifyListeners(tab, Tabs.TabEvents.START, showProgress);
                    } else if ((state & GeckoAppShell.WPL_STATE_STOP) != 0) {
                        tab.handleDocumentStop(message.getBoolean("success"));
                        notifyListeners(tab, Tabs.TabEvents.STOP);
                    }
                }
            } else if (event.equals("Content:LoadError")) {
                notifyListeners(tab, Tabs.TabEvents.LOAD_ERROR);
            } else if (event.equals("Content:PageShow")) {
                notifyListeners(tab, TabEvents.PAGE_SHOW);
            } else if (event.equals("DOMContentLoaded")) {
                String backgroundColor = message.getString("bgColor");
                if (backgroundColor != null) {
                    tab.setBackgroundColor(backgroundColor);
                } else {
                    
                    tab.setBackgroundColor(Color.WHITE);
                }
                notifyListeners(tab, Tabs.TabEvents.LOADED);
            } else if (event.equals("DOMTitleChanged")) {
                tab.updateTitle(message.getString("title"));
            } else if (event.equals("DOMLinkAdded")) {
                tab.updateFaviconURL(message.getString("href"), message.getInt("size"));
                notifyListeners(tab, TabEvents.LINK_ADDED);
            } else if (event.equals("DesktopMode:Changed")) {
                tab.setDesktopMode(message.getBoolean("desktopMode"));
                notifyListeners(tab, TabEvents.DESKTOP_MODE_CHANGE);
            }
        } catch (Exception e) {
            Log.w(LOGTAG, "handleMessage threw for " + event, e);
        }
    }

    public void refreshThumbnails() {
        final ThumbnailHelper helper = ThumbnailHelper.getInstance();
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                for (final Tab tab : mOrder) {
                    helper.getAndProcessThumbnailFor(tab);
                }
            }
        });
    }

    public interface OnTabsChangedListener {
        public void onTabChanged(Tab tab, TabEvents msg, Object data);
    }

    private static List<OnTabsChangedListener> mTabsChangedListeners =
        Collections.synchronizedList(new ArrayList<OnTabsChangedListener>());

    public static void registerOnTabsChangedListener(OnTabsChangedListener listener) {
        mTabsChangedListeners.add(listener);
    }

    public static void unregisterOnTabsChangedListener(OnTabsChangedListener listener) {
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
        MENU_UPDATED,
        PAGE_SHOW,
        LINK_ADDED,
        SECURITY_CHANGE,
        READER_ENABLED,
        DESKTOP_MODE_CHANGE
    }

    public void notifyListeners(Tab tab, TabEvents msg) {
        notifyListeners(tab, msg, "");
    }

    
    public void notifyListeners(final Tab tab, final TabEvents msg, final Object data) {
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                onTabChanged(tab, msg, data);

                synchronized (mTabsChangedListeners) {
                    if (mTabsChangedListeners.isEmpty()) {
                        return;
                    }

                    Iterator<OnTabsChangedListener> items = mTabsChangedListeners.iterator();
                    while (items.hasNext()) {
                        items.next().onTabChanged(tab, msg, data);
                    }
                }
            }
        });
    }

    private void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch (msg) {
            case LOCATION_CHANGE:
                mScore += SCORE_INCREMENT_TAB_LOCATION_CHANGE;
                break;
            case RESTORED:
                mInitialTabsAdded = true;
                break;

            
            
            
            
            case SELECTED:
                mScore += SCORE_INCREMENT_TAB_SELECTED;
            case UNSELECTED:
                tab.onChange();
                break;
            default:
                break;
        }

        if (mScore > SCORE_THRESHOLD) {
            persistAllTabs();
            mScore = 0;
        }
    }

    
    public void persistAllTabs() {
        final GeckoApp activity = getActivity();
        final Iterable<Tab> tabs = getTabsInOrder();
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                boolean syncIsSetup = SyncAccounts.syncAccountsExist(activity);
                if (syncIsSetup) {
                    TabsAccessor.persistLocalTabs(getContentResolver(), tabs);
                }
            }
        });
    }

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    




    public void loadUrl(String url) {
        loadUrl(url, LOADURL_NONE);
    }

    







    public Tab loadUrl(String url, int flags) {
        return loadUrl(url, null, -1, flags);
    }

    










    public Tab loadUrl(String url, String searchEngine, int parentId, int flags) {
        JSONObject args = new JSONObject();
        Tab added = null;
        boolean delayLoad = (flags & LOADURL_DELAY_LOAD) != 0;
        boolean background = (flags & LOADURL_BACKGROUND) != 0;

        try {
            boolean isPrivate = (flags & LOADURL_PRIVATE) != 0;
            boolean userEntered = (flags & LOADURL_USER_ENTERED) != 0;
            boolean desktopMode = (flags & LOADURL_DESKTOP) != 0;

            args.put("url", url);
            args.put("engine", searchEngine);
            args.put("parentId", parentId);
            args.put("userEntered", userEntered);
            args.put("newTab", (flags & LOADURL_NEW_TAB) != 0);
            args.put("isPrivate", isPrivate);
            args.put("pinned", (flags & LOADURL_PINNED) != 0);
            args.put("delayLoad", delayLoad);
            args.put("desktopMode", desktopMode);
            args.put("selected", !background);

            if ((flags & LOADURL_NEW_TAB) != 0) {
                int tabId = getNextTabId();
                args.put("tabID", tabId);

                
                
                
                String tabUrl = (url != null && Uri.parse(url).getScheme() != null) ? url : null;

                added = addTab(tabId, tabUrl, false, parentId, url, isPrivate);
                added.setDesktopMode(desktopMode);
            }
        } catch (Exception e) {
            Log.w(LOGTAG, "Error building JSON arguments for loadUrl.", e);
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Load", args.toString()));

        if ((added != null) && !delayLoad && !background) {
            selectTab(added.getId());
        }

        return added;
    }

    








    public void loadUrlInTab(String url) {
        Iterable<Tab> tabs = getTabsInOrder();
        for (Tab tab : tabs) {
            if (url.equals(tab.getURL())) {
                selectTab(tab.getId());
                return;
            }
        }

        
        
        
        int parentId = -1;
        Tab selectedTab = getSelectedTab();
        if (selectedTab != null) {
            parentId = selectedTab.getId();
        }

        loadUrl(url, null, parentId, LOADURL_NEW_TAB);
    }

    




    public static int getNextTabId() {
        return sTabId.getAndIncrement();
    }
}
