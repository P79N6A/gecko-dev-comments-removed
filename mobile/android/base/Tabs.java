




package org.mozilla.gecko;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.mozglue.JNITarget;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.OnAccountsUpdateListener;
import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;

public class Tabs implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTabs";

    
    private final CopyOnWriteArrayList<Tab> mOrder = new CopyOnWriteArrayList<Tab>();

    
    
    private volatile Tab mSelectedTab;

    
    private final HashMap<Integer, Tab> mTabs = new HashMap<Integer, Tab>();

    private AccountManager mAccountManager;
    private OnAccountsUpdateListener mAccountListener;

    public static final int LOADURL_NONE         = 0;
    public static final int LOADURL_NEW_TAB      = 1 << 0;
    public static final int LOADURL_USER_ENTERED = 1 << 1;
    public static final int LOADURL_PRIVATE      = 1 << 2;
    public static final int LOADURL_PINNED       = 1 << 3;
    public static final int LOADURL_DELAY_LOAD   = 1 << 4;
    public static final int LOADURL_DESKTOP      = 1 << 5;
    public static final int LOADURL_BACKGROUND   = 1 << 6;
    public static final int LOADURL_EXTERNAL     = 1 << 7;

    private static final long PERSIST_TABS_AFTER_MILLISECONDS = 1000 * 5;

    public static final int INVALID_TAB_ID = -1;

    private static AtomicInteger sTabId = new AtomicInteger(0);
    private volatile boolean mInitialTabsAdded;

    private Context mAppContext;
    private ContentObserver mContentObserver;

    private final Runnable mPersistTabsRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                final Context context = getAppContext();
                boolean syncIsSetup = SyncAccounts.syncAccountsExist(context) ||
                                      FirefoxAccounts.firefoxAccountsExist(context);
                if (syncIsSetup) {
                    TabsAccessor.persistLocalTabs(getContentResolver(), getTabsInOrder());
                }
            } catch (SecurityException se) {} 
        }
    };

    private Tabs() {
        EventDispatcher.getInstance().registerGeckoThreadListener(this,
            "Session:RestoreEnd",
            "SessionHistory:New",
            "SessionHistory:Back",
            "SessionHistory:Forward",
            "SessionHistory:Goto",
            "SessionHistory:Purge",
            "Tab:Added",
            "Tab:Close",
            "Tab:Select",
            "Content:LocationChange",
            "Content:SecurityChange",
            "Content:ReaderEnabled",
            "Content:StateChange",
            "Content:LoadError",
            "Content:PageShow",
            "DOMContentLoaded",
            "DOMTitleChanged",
            "Link:Favicon",
            "Link:Feed",
            "Link:OpenSearch",
            "DesktopMode:Changed",
            "Tab:ViewportMetadata",
            "Tab:StreamStart",
            "Tab:StreamStop");

    }

    public synchronized void attachToContext(Context context) {
        final Context appContext = context.getApplicationContext();
        if (mAppContext == appContext) {
            return;
        }

        if (mAppContext != null) {
            
            Log.w(LOGTAG, "The application context has changed!");
        }

        mAppContext = appContext;
        mAccountManager = AccountManager.get(appContext);

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

    public int isOpen(String url) {
        for (Tab tab : mOrder) {
            if (tab.getURL().equals(url)) {
                return tab.getId();
            }
        }
        return -1;
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

    private Tab addTab(int id, String url, boolean external, int parentId, String title, boolean isPrivate, int tabIndex) {
        final Tab tab = isPrivate ? new PrivateTab(mAppContext, id, url, external, parentId, title) :
                                    new Tab(mAppContext, id, url, external, parentId, title);
        synchronized (this) {
            lazyRegisterBookmarkObserver();
            mTabs.put(id, tab);

            if (tabIndex > -1) {
                mOrder.add(tabIndex, tab);
            } else {
                mOrder.add(tab);
            }
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

    public boolean isSelectedTabId(int tabId) {
        final Tab selected = mSelectedTab;
        return selected != null && selected.getId() == tabId;
    }

    @RobocopTarget
    public synchronized Tab getTab(int id) {
        if (id == -1)
            return null;

        if (mTabs.size() == 0)
            return null;

        if (!mTabs.containsKey(id))
           return null;

        return mTabs.get(id);
    }

    
    @RobocopTarget
    public synchronized void closeTab(Tab tab) {
        closeTab(tab, getNextTab(tab));
    }

    public synchronized void closeTab(Tab tab, Tab nextTab) {
        closeTab(tab, nextTab, false);
    }

    public synchronized void closeTab(Tab tab, boolean showUndoToast) {
        closeTab(tab, getNextTab(tab), showUndoToast);
    }

    
    public synchronized void closeTab(final Tab tab, Tab nextTab, boolean showUndoToast) {
        if (tab == null)
            return;

        int tabId = tab.getId();
        removeTab(tabId);

        if (nextTab == null) {
            nextTab = loadUrl(AboutPages.HOME, LOADURL_NEW_TAB);
        }

        selectTab(nextTab.getId());

        tab.onDestroy();

        final JSONObject args = new JSONObject();
        try {
            args.put("tabId", String.valueOf(tabId));
            args.put("showUndoToast", showUndoToast);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error building Tab:Closed arguments: " + e);
        }

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Closed", args.toString()));
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
            if (!lastTab.isPrivate()) {
                nextTab = lastTab;
            } else {
                nextTab = getPreviousTabFrom(lastTab, false);
            }
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

    



    private synchronized Context getAppContext() {
        if (mAppContext == null) {
            throw new IllegalStateException("Tabs not initialized with a GeckoApp instance.");
        }
        return mAppContext;
    }

    public ContentResolver getContentResolver() {
        return getAppContext().getContentResolver();
    }

    
    private static class TabsInstanceHolder {
        private static final Tabs INSTANCE = new Tabs();
    }

    @RobocopTarget
    public static Tabs getInstance() {
       return Tabs.TabsInstanceHolder.INSTANCE;
    }

    
    @Override
    public void handleMessage(String event, JSONObject message) {
        Log.d(LOGTAG, "handleMessage: " + event);
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
                } else {
                    tab = addTab(id, url, message.getBoolean("external"),
                                          message.getInt("parentId"),
                                          message.getString("title"),
                                          message.getBoolean("isPrivate"),
                                          message.getInt("tabIndex"));

                    
                    
                    if (message.getBoolean("selected"))
                        selectTab(id);
                }

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
                        boolean restoring = message.getBoolean("restoring");
                        tab.handleDocumentStart(restoring, message.getString("uri"));
                        notifyListeners(tab, Tabs.TabEvents.START);
                    } else if ((state & GeckoAppShell.WPL_STATE_STOP) != 0) {
                        tab.handleDocumentStop(message.getBoolean("success"));
                        notifyListeners(tab, Tabs.TabEvents.STOP);
                    }
                }
            } else if (event.equals("Content:LoadError")) {
                tab.handleContentLoaded();
                notifyListeners(tab, Tabs.TabEvents.LOAD_ERROR);
            } else if (event.equals("Content:PageShow")) {
                notifyListeners(tab, TabEvents.PAGE_SHOW);
            } else if (event.equals("DOMContentLoaded")) {
                tab.handleContentLoaded();
                String backgroundColor = message.getString("bgColor");
                if (backgroundColor != null) {
                    tab.setBackgroundColor(backgroundColor);
                } else {
                    
                    tab.setBackgroundColor(Color.WHITE);
                }
                tab.setErrorType(message.optString("errorType"));
                tab.setMetadata(message.optJSONObject("metadata"));
                notifyListeners(tab, Tabs.TabEvents.LOADED);
            } else if (event.equals("DOMTitleChanged")) {
                tab.updateTitle(message.getString("title"));
            } else if (event.equals("Link:Favicon")) {
                tab.updateFaviconURL(message.getString("href"), message.getInt("size"));
                notifyListeners(tab, TabEvents.LINK_FAVICON);
            } else if (event.equals("Link:Feed")) {
                tab.setHasFeeds(true);
                notifyListeners(tab, TabEvents.LINK_FEED);
            } else if (event.equals("Link:OpenSearch")) {
                boolean visible = message.getBoolean("visible");
                tab.setHasOpenSearch(visible);
            } else if (event.equals("DesktopMode:Changed")) {
                tab.setDesktopMode(message.getBoolean("desktopMode"));
                notifyListeners(tab, TabEvents.DESKTOP_MODE_CHANGE);
            } else if (event.equals("Tab:ViewportMetadata")) {
                tab.setZoomConstraints(new ZoomConstraints(message));
                tab.setIsRTL(message.getBoolean("isRTL"));
                notifyListeners(tab, TabEvents.VIEWPORT_CHANGE);
            } else if (event.equals("Tab:StreamStart")) {
                tab.setRecording(true);
                notifyListeners(tab, TabEvents.RECORDING_CHANGE);
            } else if (event.equals("Tab:StreamStop")) {
                tab.setRecording(false);
                notifyListeners(tab, TabEvents.RECORDING_CHANGE);
            }

        } catch (Exception e) {
            Log.w(LOGTAG, "handleMessage threw for " + event, e);
        }
    }

    


    public void updateFaviconForURL(String pageURL, Bitmap favicon) {
        
        
        
        for (Tab tab : mOrder) {
            String tabURL = tab.getURL();
            if (pageURL.equals(tabURL)) {
                tab.setFaviconLoadId(Favicons.NOT_LOADING);
                if (tab.updateFavicon(favicon)) {
                    notifyListeners(tab, TabEvents.FAVICON);
                }
            }
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

    private static final List<OnTabsChangedListener> TABS_CHANGED_LISTENERS = new CopyOnWriteArrayList<OnTabsChangedListener>();

    public static void registerOnTabsChangedListener(OnTabsChangedListener listener) {
        TABS_CHANGED_LISTENERS.add(listener);
    }

    public static void unregisterOnTabsChangedListener(OnTabsChangedListener listener) {
        TABS_CHANGED_LISTENERS.remove(listener);
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
        LINK_FAVICON,
        LINK_FEED,
        SECURITY_CHANGE,
        READER_ENABLED,
        DESKTOP_MODE_CHANGE,
        VIEWPORT_CHANGE,
        RECORDING_CHANGE,
        BOOKMARK_ADDED,
        BOOKMARK_REMOVED
    }

    public void notifyListeners(Tab tab, TabEvents msg) {
        notifyListeners(tab, msg, "");
    }

    public void notifyListeners(final Tab tab, final TabEvents msg, final Object data) {
        if (tab == null &&
            msg != TabEvents.RESTORED) {
            throw new IllegalArgumentException("onTabChanged:" + msg + " must specify a tab.");
        }

        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                onTabChanged(tab, msg, data);

                if (TABS_CHANGED_LISTENERS.isEmpty()) {
                    return;
                }

                Iterator<OnTabsChangedListener> items = TABS_CHANGED_LISTENERS.iterator();
                while (items.hasNext()) {
                    items.next().onTabChanged(tab, msg, data);
                }
            }
        });
    }

    private void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch (msg) {
            case LOCATION_CHANGE:
                queuePersistAllTabs();
                break;
            case RESTORED:
                mInitialTabsAdded = true;
                break;

            
            
            
            
            case SELECTED:
                queuePersistAllTabs();
            case UNSELECTED:
                tab.onChange();
                break;
            default:
                break;
        }
    }

    
    public void persistAllTabs() {
        ThreadUtils.postToBackgroundThread(mPersistTabsRunnable);
    }

    




    private void queuePersistAllTabs() {
        Handler backgroundHandler = ThreadUtils.getBackgroundHandler();
        backgroundHandler.removeCallbacks(mPersistTabsRunnable);
        backgroundHandler.postDelayed(mPersistTabsRunnable, PERSIST_TABS_AFTER_MILLISECONDS);
    }

    





    public Tab getFirstTabForUrl(String url) {
        return getFirstTabForUrlHelper(url, null);
    }

    







    public Tab getFirstTabForUrl(String url, boolean isPrivate) {
        return getFirstTabForUrlHelper(url, isPrivate);
    }

    private Tab getFirstTabForUrlHelper(String url, Boolean isPrivate) {
        if (url == null) {
            return null;
        }

        for (Tab tab : mOrder) {
            if (isPrivate != null && isPrivate != tab.isPrivate()) {
                continue;
            }
            if (url.equals(tab.getURL())) {
                return tab;
            }
        }

        return null;
    }

    













    public Tab getFirstReaderTabForUrl(String url, boolean isPrivate) {
        if (url == null) {
            return null;
        }

        if (AboutPages.isAboutReader(url)) {
            url = ReaderModeUtils.getUrlFromAboutReader(url);
        }
        for (Tab tab : mOrder) {
            if (isPrivate != tab.isPrivate()) {
                continue;
            }
            String tabUrl = tab.getURL();
            if (AboutPages.isAboutReader(tabUrl)) {
                tabUrl = ReaderModeUtils.getUrlFromAboutReader(tabUrl);
                if (url.equals(tabUrl)) {
                    return tab;
                }
            }
        }

        return null;
    }

    




    @RobocopTarget
    public Tab loadUrl(String url) {
        return loadUrl(url, LOADURL_NONE);
    }

    







    public Tab loadUrl(String url, int flags) {
        return loadUrl(url, null, -1, flags);
    }

    










    public Tab loadUrl(String url, String searchEngine, int parentId, int flags) {
        JSONObject args = new JSONObject();
        Tab added = null;
        boolean delayLoad = (flags & LOADURL_DELAY_LOAD) != 0;

        
        boolean background = delayLoad || (flags & LOADURL_BACKGROUND) != 0;

        try {
            boolean isPrivate = (flags & LOADURL_PRIVATE) != 0;
            boolean userEntered = (flags & LOADURL_USER_ENTERED) != 0;
            boolean desktopMode = (flags & LOADURL_DESKTOP) != 0;
            boolean external = (flags & LOADURL_EXTERNAL) != 0;

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

                
                final int tabIndex = -1;

                added = addTab(tabId, tabUrl, external, parentId, url, isPrivate, tabIndex);
                added.setDesktopMode(desktopMode);
            }
        } catch (Exception e) {
            Log.w(LOGTAG, "Error building JSON arguments for loadUrl.", e);
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Load", args.toString()));

        if (added == null) {
            return null;
        }

        if (!delayLoad && !background) {
            selectTab(added.getId());
        }

        
        if (AboutPages.isBuiltinIconPage(url)) {
            Log.d(LOGTAG, "Setting about: tab favicon inline.");
            added.updateFavicon(getAboutPageFavicon(url));
        }

        return added;
    }

    





    private Bitmap getAboutPageFavicon(final String url) {
        int faviconSize = Math.round(mAppContext.getResources().getDimension(R.dimen.browser_toolbar_favicon_size));
        return Favicons.getSizedFaviconForPageFromCache(url, faviconSize);
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

    


    @JNITarget
    public static int getNextTabId() {
        return sTabId.getAndIncrement();
    }
}
