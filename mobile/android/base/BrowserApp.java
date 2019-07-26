




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoAsyncTask;
import org.mozilla.gecko.util.GeckoBackgroundThread;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Interpolator;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.io.InputStream;
import java.net.URL;
import java.util.EnumSet;
import java.util.Vector;

abstract public class BrowserApp extends GeckoApp
                                 implements TabsPanel.TabsLayoutChangeListener,
                                            PropertyAnimator.PropertyAnimationListener {
    private static final String LOGTAG = "GeckoBrowserApp";

    public static BrowserToolbar mBrowserToolbar;
    private AboutHomeContent mAboutHomeContent;
    private Boolean mAboutHomeShowing = null;
    protected Telemetry.Timer mAboutHomeStartupTimer = null;

    private static final int ADDON_MENU_OFFSET = 1000;
    private class MenuItemInfo {
        public int id;
        public String label;
        public String icon;
        public boolean checkable;
        public boolean checked;
        public boolean enabled;
        public boolean visible;
        public int parent;
    }

    private Vector<MenuItemInfo> mAddonMenuItemsCache;

    private PropertyAnimator mMainLayoutAnimator;

    private static final Interpolator sTabsInterpolator = new Interpolator() {
        public float getInterpolation(float t) {
            t -= 1.0f;
            return t * t * t * t * t + 1.0f;
        }
    };

    private FindInPageBar mFindInPageBar;

    
    private static final int FEEDBACK_LAUNCH_COUNT = 15;

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case LOCATION_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    maybeCancelFaviconLoad(tab);
                }
                
            case SELECTED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    if ("about:home".equals(tab.getURL()))
                        showAboutHome();
                    else
                        hideAboutHome();

                    
                    SiteIdentityPopup.getInstance().dismiss();

                    final TabsPanel.Panel panel = tab.isPrivate()
                                                ? TabsPanel.Panel.PRIVATE_TABS
                                                : TabsPanel.Panel.NORMAL_TABS;
                    mMainHandler.post(new Runnable() {
                        public void run() {
                            if (areTabsShown() && mTabsPanel.getCurrentPanel() != panel)
                                showTabs(panel);
                        }
                    });
                }
                break;
            case LOAD_ERROR:
            case START:
            case STOP:
            case MENU_UPDATED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    invalidateOptionsMenu();
                }
                break;
        }
        super.onTabChanged(tab, msg, data);
    }

    @Override
    void handlePageShow(final int tabId) {
        super.handlePageShow(tabId);
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() {
            public void run() {
                loadFavicon(tab);
            }
        });
    }

    @Override
    void handleLinkAdded(final int tabId, String rel, final String href, int size) {
        super.handleLinkAdded(tabId, rel, href, size);
        if (rel.indexOf("[icon]") == -1)
            return;

        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        
        
        
        
        if (tab.getState() != Tab.STATE_LOADING) {
            mMainHandler.post(new Runnable() {
                public void run() {
                    loadFavicon(tab);
                }
            });
        }
    }

    @Override
    void handleClearHistory() {
        super.handleClearHistory();
        updateAboutHomeTopSites();
    }

    @Override
    void handleSecurityChange(final int tabId, final JSONObject identityData) {
        super.handleSecurityChange(tabId, identityData);
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() { 
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setSecurityMode(tab.getSecurityMode());
            }
        });
    }

    void handleReaderEnabled(final int tabId) {
        super.handleReaderEnabled(tabId);
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setReaderMode(tab.getReaderEnabled());
            }
        });
    }

    @Override
    void onStatePurged() {
        mMainHandler.post(new Runnable() {
            public void run() {
                if (mAboutHomeContent != null)
                    mAboutHomeContent.setLastTabsVisibility(false);
            }
        });

        super.onStatePurged();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mAboutHomeStartupTimer = new Telemetry.Timer("FENNEC_STARTUP_TIME_ABOUTHOME");

        super.onCreate(savedInstanceState);

        LinearLayout actionBar = (LinearLayout) getActionBarLayout();
        mMainLayout.addView(actionBar, 0);

        ((GeckoApp.MainLayout) mMainLayout).setOnInterceptTouchListener(new HideTabsTouchListener());

        mBrowserToolbar = new BrowserToolbar(this);
        mBrowserToolbar.from(actionBar);

        if (mTabsPanel != null)
            mTabsPanel.setTabsLayoutChangeListener(this);

        mFindInPageBar = (FindInPageBar) findViewById(R.id.find_in_page);

        registerEventListener("CharEncoding:Data");
        registerEventListener("CharEncoding:State");
        registerEventListener("Feedback:LastUrl");
        registerEventListener("Feedback:OpenPlayStore");
        registerEventListener("Feedback:MaybeLater");
        registerEventListener("Telemetry:Gather");

        Distribution.init(this);
        JavaAddonManager.getInstance().init(getApplicationContext());
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mAboutHomeContent != null)
            mAboutHomeContent.onDestroy();
        if (mBrowserToolbar != null)
            mBrowserToolbar.onDestroy();

        unregisterEventListener("CharEncoding:Data");
        unregisterEventListener("CharEncoding:State");
        unregisterEventListener("Feedback:LastUrl");
        unregisterEventListener("Feedback:OpenPlayStore");
        unregisterEventListener("Feedback:MaybeLater");
        unregisterEventListener("Telemetry:Gather");
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
        if (mAboutHomeContent != null)
            mAboutHomeContent.onActivityContentChanged();
    }

    @Override
    protected void finishProfileMigration() {
        
        updateAboutHomeTopSites();

        super.finishProfileMigration();
    }

    @Override
    protected void initializeChrome(String uri, boolean isExternalURL) {
        super.initializeChrome(uri, isExternalURL);

        mBrowserToolbar.updateBackButton(false);
        mBrowserToolbar.updateForwardButton(false);

        mDoorHangerPopup.setAnchor(mBrowserToolbar.mFavicon);

        if (isExternalURL || mRestoreMode != RESTORE_NONE) {
            mAboutHomeStartupTimer.cancel();
        }

        if (!mIsRestoringActivity) {
            if (!isExternalURL) {
                
                if (mRestoreMode == RESTORE_NONE) {
                    Tab tab = Tabs.getInstance().loadUrl("about:home", Tabs.LOADURL_NEW_TAB);
                } else {
                    hideAboutHome();
                }
            } else {
                int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_USER_ENTERED;
                Tabs.getInstance().loadUrl(uri, flags);
            }
        }
    }

    @Override
    void toggleChrome(final boolean aShow) {
        mMainHandler.post(new Runnable() {
            public void run() {
                if (aShow) {
                    mBrowserToolbar.show();
                } else {
                    mBrowserToolbar.hide();
                    if (hasTabsSideBar()) {
                        hideTabs();
                    }
                }
            }
        });

        super.toggleChrome(aShow);
    }

    @Override
    void focusChrome() {
        mMainHandler.post(new Runnable() {
            public void run() {
                mBrowserToolbar.show();
                mBrowserToolbar.requestFocusFromTouch();
            }
        });
    }

    @Override
    public void refreshChrome() {
        
        if (Build.VERSION.SDK_INT >= 14 && !isTablet()) {
            int index = mMainLayout.indexOfChild(mBrowserToolbar.getLayout());
            mMainLayout.removeViewAt(index);

            LinearLayout actionBar = (LinearLayout) getActionBarLayout();
            mMainLayout.addView(actionBar, index);
            mBrowserToolbar.from(actionBar);
            mBrowserToolbar.refresh();

            
            if (mDoorHangerPopup != null)
                mDoorHangerPopup.setAnchor(mBrowserToolbar.mFavicon);
        }

        invalidateOptionsMenu();
        mTabsPanel.refresh();

        if (mAboutHomeContent != null)
            mAboutHomeContent.refresh();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        mBrowserToolbar.fromAwesomeBarSearch();
    }

    public View getActionBarLayout() {
        int actionBarRes;

        if (!hasPermanentMenuKey() || isTablet())
           actionBarRes = R.layout.browser_toolbar_menu;
        else
           actionBarRes = R.layout.browser_toolbar;

        LinearLayout actionBar = (LinearLayout) LayoutInflater.from(this).inflate(actionBarRes, null);
        actionBar.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                                                                (int) getResources().getDimension(R.dimen.browser_toolbar_height)));
        return actionBar;
    }

    @Override
    public boolean hasTabsSideBar() {
        return (mTabsPanel != null && mTabsPanel.isSideBar());
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Menu:Add")) {
                MenuItemInfo info = new MenuItemInfo();
                info.label = message.getString("name");
                info.id = message.getInt("id") + ADDON_MENU_OFFSET;
                info.checkable = false;
                info.checked = false;
                info.enabled = true;
                info.visible = true;
                String iconRes = null;
                try { 
                    iconRes = message.getString("icon");
                } catch (Exception ex) { }
                info.icon = iconRes;
                info.checkable = false;
                try {
                    info.checkable = message.getBoolean("checkable");
                } catch (Exception ex) { }
                try { 
                    info.parent = message.getInt("parent") + ADDON_MENU_OFFSET;
                } catch (Exception ex) { }
                final MenuItemInfo menuItemInfo = info;
                mMainHandler.post(new Runnable() {
                    public void run() {
                        addAddonMenuItem(menuItemInfo);
                    }
                });
            } else if (event.equals("Menu:Remove")) {
                final int id = message.getInt("id") + ADDON_MENU_OFFSET;
                mMainHandler.post(new Runnable() {
                    public void run() {
                        removeAddonMenuItem(id);
                    }
                });
            } else if (event.equals("Menu:Update")) {
                final int id = message.getInt("id") + ADDON_MENU_OFFSET;
                final JSONObject options = message.getJSONObject("options");
                mMainHandler.post(new Runnable() {
                    public void run() {
                        updateAddonMenuItem(id, options);
                    }
                });
            } else if (event.equals("CharEncoding:Data")) {
                final JSONArray charsets = message.getJSONArray("charsets");
                int selected = message.getInt("selected");

                final int len = charsets.length();
                final String[] titleArray = new String[len];
                for (int i = 0; i < len; i++) {
                    JSONObject charset = charsets.getJSONObject(i);
                    titleArray[i] = charset.getString("title");
                }

                final AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);
                dialogBuilder.setSingleChoiceItems(titleArray, selected, new AlertDialog.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        try {
                            JSONObject charset = charsets.getJSONObject(which);
                            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("CharEncoding:Set", charset.getString("code")));
                            dialog.dismiss();
                        } catch (JSONException e) {
                            Log.e(LOGTAG, "error parsing json", e);
                        }
                    }
                });
                dialogBuilder.setNegativeButton(R.string.button_cancel, new AlertDialog.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
                mMainHandler.post(new Runnable() {
                    public void run() {
                        dialogBuilder.show();
                    }
                });
            } else if (event.equals("CharEncoding:State")) {
                final boolean visible = message.getString("visible").equals("true");
                GeckoPreferences.setCharEncodingState(visible);
                final Menu menu = mMenu;
                mMainHandler.post(new Runnable() {
                    public void run() {
                        if (menu != null)
                            menu.findItem(R.id.char_encoding).setVisible(visible);
                    }
                });
            } else if (event.equals("Feedback:OpenPlayStore")) {
                Intent intent = new Intent(Intent.ACTION_VIEW);
                intent.setData(Uri.parse("market://details?id=" + getPackageName()));
                startActivity(intent);
            } else if (event.equals("Feedback:MaybeLater")) {
                resetFeedbackLaunchCount();
            } else if (event.equals("Feedback:LastUrl")) {
                getLastUrl();
            } else if (event.equals("Gecko:Ready")) {
                
                
                super.handleMessage(event, message);
                final Menu menu = mMenu;
                mMainHandler.post(new Runnable() {
                    public void run() {
                        if (menu != null)
                            menu.findItem(R.id.settings).setEnabled(true);
                    }
                });
            } else if (event.equals("Telemetry:Gather")) {
                Telemetry.HistogramAdd("PLACES_PAGES_COUNT", BrowserDB.getCount(getContentResolver(), "history"));
                Telemetry.HistogramAdd("PLACES_BOOKMARKS_COUNT", BrowserDB.getCount(getContentResolver(), "bookmarks"));
                Telemetry.HistogramAdd("FENNEC_FAVICONS_COUNT", BrowserDB.getCount(getContentResolver(), "favicons"));
                Telemetry.HistogramAdd("FENNEC_THUMBNAILS_COUNT", BrowserDB.getCount(getContentResolver(), "thumbnails"));
            } else {
                super.handleMessage(event, message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public void addTab() {
        showAwesomebar(AwesomeBar.Target.NEW_TAB);
    }

    public void addPrivateTab() {
        Tabs.getInstance().loadUrl("about:privatebrowsing", Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_PRIVATE);
    }

    public void showNormalTabs() {
        showTabs(TabsPanel.Panel.NORMAL_TABS);
    }

    public void showPrivateTabs() {
        showTabs(TabsPanel.Panel.PRIVATE_TABS);
    }

    public void showRemoteTabs() {
        showTabs(TabsPanel.Panel.REMOTE_TABS);
    }

    private void showTabs(TabsPanel.Panel panel) {
        if (Tabs.getInstance().getDisplayCount() == 0)
            return;

        mTabsPanel.show(panel);
    }

    public void hideTabs() {
        mTabsPanel.hide();
    }

    public boolean autoHideTabs() {
        if (!hasTabsSideBar() && areTabsShown()) {
            hideTabs();
            return true;
        }
        return false;
    }

    public boolean areTabsShown() {
        return mTabsPanel.isShown();
    }

    @Override
    public void onTabsLayoutChange(int width, int height) {
        if (mMainLayoutAnimator != null)
            mMainLayoutAnimator.stop();

        if (mTabsPanel.isShown())
            mTabsPanel.setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);

        mMainLayoutAnimator = new PropertyAnimator(450, sTabsInterpolator);
        mMainLayoutAnimator.setPropertyAnimationListener(this);

        boolean usingTextureView = mLayerView.shouldUseTextureView();
        mMainLayoutAnimator.setUseHardwareLayer(usingTextureView);

        if (hasTabsSideBar()) {
            mBrowserToolbar.prepareTabsAnimation(mMainLayoutAnimator, width);

            
            if (!mTabsPanel.isShown()) {
                ((LinearLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(0, 0, 0, 0);
                if (!usingTextureView)
                    mGeckoLayout.scrollTo(mTabsPanel.getWidth() * -1, 0);
                mGeckoLayout.requestLayout();
            }

            mMainLayoutAnimator.attach(mGeckoLayout,
                                       usingTextureView ? PropertyAnimator.Property.TRANSLATION_X :
                                                          PropertyAnimator.Property.SCROLL_X,
                                       usingTextureView ? width : -width);
        } else {
            mMainLayoutAnimator.attach(mMainLayout,
                                       usingTextureView ? PropertyAnimator.Property.TRANSLATION_Y :
                                                          PropertyAnimator.Property.SCROLL_Y,
                                       usingTextureView ? height : -height);
        }

        mMainLayoutAnimator.start();
    }

    @Override
    public void onPropertyAnimationStart() {
        mBrowserToolbar.updateTabs(true);

        
        
        
        if (Build.VERSION.SDK_INT >= 11)
            mTabsPanel.setLayerType(View.LAYER_TYPE_HARDWARE, null);
        else
            mTabsPanel.setDrawingCacheEnabled(true);
    }

    @Override
    public void onPropertyAnimationEnd() {
        
        if (Build.VERSION.SDK_INT >= 11)
            mTabsPanel.setLayerType(View.LAYER_TYPE_NONE, null);
        else
            mTabsPanel.setDrawingCacheEnabled(false);

        if (mTabsPanel.isShown()) {
            if (hasTabsSideBar()) {
                boolean usingTextureView = mLayerView.shouldUseTextureView();

                int leftMargin = (usingTextureView ? 0 : mTabsPanel.getWidth());
                int rightMargin = (usingTextureView ? mTabsPanel.getWidth() : 0);
                ((LinearLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(leftMargin, 0, rightMargin, 0);

                if (!usingTextureView)
                    mGeckoLayout.scrollTo(0, 0);
            }

            mGeckoLayout.requestLayout();
        } else {
            mBrowserToolbar.updateTabs(false);
            mBrowserToolbar.finishTabsAnimation();
            mTabsPanel.setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);
        }

        mBrowserToolbar.refreshBackground();

        if (hasTabsSideBar())
            mBrowserToolbar.adjustTabsAnimation(true);
    }

    
    private void loadFavicon(final Tab tab) {
        maybeCancelFaviconLoad(tab);

        long id = Favicons.getInstance().loadFavicon(tab.getURL(), tab.getFaviconURL(), !tab.isPrivate(),
                        new Favicons.OnFaviconLoadedListener() {

            public void onFaviconLoaded(String pageUrl, Bitmap favicon) {
                
                
                if (favicon == null)
                    return;

                
                
                if (!tab.getURL().equals(pageUrl))
                    return;

                tab.updateFavicon(favicon);
                tab.setFaviconLoadId(Favicons.NOT_LOADING);

                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setFavicon(tab.getFavicon());

                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.FAVICON);
            }
        });

        tab.setFaviconLoadId(id);
    }

    private void maybeCancelFaviconLoad(Tab tab) {
        long faviconLoadId = tab.getFaviconLoadId();

        if (faviconLoadId == Favicons.NOT_LOADING)
            return;

        
        Favicons.getInstance().cancelFaviconLoad(faviconLoadId);

        
        tab.setFaviconLoadId(Favicons.NOT_LOADING);
    }


    
    void updateAboutHomeTopSites() {
        if (mAboutHomeContent == null)
            return;

        mAboutHomeContent.update(EnumSet.of(AboutHomeContent.UpdateFlags.TOP_SITES));
    }

    private void showAboutHome() {
        
        
        if (mAboutHomeShowing != null && mAboutHomeShowing)
            return;

        mAboutHomeShowing = true;
        Runnable r = new AboutHomeRunnable(true);
        mMainHandler.postAtFrontOfQueue(r);
    }

    private void hideAboutHome() {
        
        
        if (mAboutHomeShowing != null && !mAboutHomeShowing)
            return;

        mBrowserToolbar.setShadowVisibility(true);
        mAboutHomeShowing = false;
        Runnable r = new AboutHomeRunnable(false);
        mMainHandler.postAtFrontOfQueue(r);
    }

    private class AboutHomeRunnable implements Runnable {
        boolean mShow;
        AboutHomeRunnable(boolean show) {
            mShow = show;
        }

        public void run() {
            if (mShow) {
                if (mAboutHomeContent == null) {
                    mAboutHomeContent = (AboutHomeContent) findViewById(R.id.abouthome_content);
                    mAboutHomeContent.init();
                    mAboutHomeContent.update(AboutHomeContent.UpdateFlags.ALL);
                    mAboutHomeContent.setUriLoadCallback(new AboutHomeContent.UriLoadCallback() {
                        public void callback(String url) {
                            mBrowserToolbar.setProgressVisibility(true);
                            Tabs.getInstance().loadUrl(url);
                        }
                    });
                    mAboutHomeContent.setLoadCompleteCallback(new AboutHomeContent.VoidCallback() {
                        public void callback() {
                            mAboutHomeStartupTimer.stop();
                        }
                    });
                } else {
                    mAboutHomeContent.update(EnumSet.of(AboutHomeContent.UpdateFlags.TOP_SITES,
                                                        AboutHomeContent.UpdateFlags.REMOTE_TABS));
                }
                mAboutHomeContent.setVisibility(View.VISIBLE);
            } else {
                findViewById(R.id.abouthome_content).setVisibility(View.GONE);
            }
        } 
    }

    private class HideTabsTouchListener implements OnInterceptTouchListener {
        private boolean mIsHidingTabs = false;

        @Override
        public boolean onInterceptTouchEvent(View view, MotionEvent event) {
            
            
            
            if (view.getScrollX() != 0 || view.getScrollY() != 0) {
                Rect rect = new Rect();
                view.getHitRect(rect);
                rect.offset(-view.getScrollX(), -view.getScrollY());

                int[] viewCoords = new int[2];
                view.getLocationOnScreen(viewCoords);

                int x = (int) event.getRawX() - viewCoords[0];
                int y = (int) event.getRawY() - viewCoords[1];

                if (!rect.contains(x, y))
                    return false;
            }

            
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN && autoHideTabs()) {
                mIsHidingTabs = true;
                return true;
            }
            return false;
        }

        @Override
        public boolean onTouch(View view, MotionEvent event) {
            if (mIsHidingTabs) {
                
                int action = event.getActionMasked();
                if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                    mIsHidingTabs = false;
                }
                return true;
            }
            return false;
        }
    }

    private void addAddonMenuItem(final MenuItemInfo info) {
        if (mMenu == null) {
            if (mAddonMenuItemsCache == null)
                mAddonMenuItemsCache = new Vector<MenuItemInfo>();

            mAddonMenuItemsCache.add(info);
            return;
        }

        Menu menu;
        if (info.parent == 0) {
            menu = mMenu;
        } else {
            MenuItem parent = mMenu.findItem(info.parent);
            if (parent == null)
                return;

            if (!parent.hasSubMenu()) {
                mMenu.removeItem(parent.getItemId());
                menu = mMenu.addSubMenu(Menu.NONE, parent.getItemId(), Menu.NONE, parent.getTitle());
                if (parent.getIcon() != null)
                    ((SubMenu) menu).getItem().setIcon(parent.getIcon());
            } else {
                menu = parent.getSubMenu();
            }
        }

        final MenuItem item = menu.add(Menu.NONE, info.id, Menu.NONE, info.label);
        item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                Log.i(LOGTAG, "menu item clicked");
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Menu:Clicked", Integer.toString(info.id - ADDON_MENU_OFFSET)));
                return true;
            }
        });

        if (info.icon != null) {
            if (info.icon.startsWith("data")) {
                BitmapDrawable drawable = new BitmapDrawable(BitmapUtils.getBitmapFromDataURI(info.icon));
                item.setIcon(drawable);
            }
            else if (info.icon.startsWith("jar:") || info.icon.startsWith("file://")) {
                GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        try {
                            URL url = new URL(info.icon);
                            InputStream is = (InputStream) url.getContent();
                            try {
                                Drawable drawable = Drawable.createFromStream(is, "src");
                                item.setIcon(drawable);
                            } finally {
                                is.close();
                            }
                        } catch (Exception e) {
                            Log.w(LOGTAG, "Unable to set icon", e);
                        }
                    }
                });
            }
        }

        item.setCheckable(info.checkable);
        item.setChecked(info.checked);
        item.setEnabled(info.enabled);
        item.setVisible(info.visible);
    }

    private void removeAddonMenuItem(int id) {
        
        if (mAddonMenuItemsCache != null && !mAddonMenuItemsCache.isEmpty()) {
            for (MenuItemInfo item : mAddonMenuItemsCache) {
                 if (item.id == id) {
                     mAddonMenuItemsCache.remove(item);
                     break;
                 }
            }
        }

        if (mMenu == null)
            return;

        MenuItem menuItem = mMenu.findItem(id);
        if (menuItem != null)
            mMenu.removeItem(id);
    }

    private void updateAddonMenuItem(int id, JSONObject options) {
        
        if (mAddonMenuItemsCache != null && !mAddonMenuItemsCache.isEmpty()) {
            for (MenuItemInfo item : mAddonMenuItemsCache) {
                 if (item.id == id) {
                     try {
                        item.checkable = options.getBoolean("checkable");
                     } catch (JSONException e) {}

                     try {
                        item.checked = options.getBoolean("checked");
                     } catch (JSONException e) {}

                     try {
                        item.enabled = options.getBoolean("enabled");
                     } catch (JSONException e) {}

                     try {
                        item.visible = options.getBoolean("visible");
                     } catch (JSONException e) {}
                     break;
                 }
            }
        }

        if (mMenu == null)
            return;

        MenuItem menuItem = mMenu.findItem(id);
        if (menuItem != null) {
            try {
               menuItem.setCheckable(options.getBoolean("checkable"));
            } catch (JSONException e) {}

            try {
               menuItem.setChecked(options.getBoolean("checked"));
            } catch (JSONException e) {}

            try {
               menuItem.setEnabled(options.getBoolean("enabled"));
            } catch (JSONException e) {}

            try {
               menuItem.setVisible(options.getBoolean("visible"));
            } catch (JSONException e) {}
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        
        if (menu instanceof GeckoMenu && isTablet())
            ((GeckoMenu) menu).setActionItemBarPresenter(mBrowserToolbar);

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.browser_app_menu, mMenu);

        
        if (mAddonMenuItemsCache != null && !mAddonMenuItemsCache.isEmpty()) {
            for (MenuItemInfo item : mAddonMenuItemsCache) {
                 addAddonMenuItem(item);
            }

            mAddonMenuItemsCache.clear();
        }

        return true;
    }

    @Override
    public void openOptionsMenu() {
        if (!hasTabsSideBar() && areTabsShown())
            return;

        
        if (mMenuPanel != null)
            mMenuPanel.scrollTo(0, 0);

        if (!mBrowserToolbar.openOptionsMenu())
            super.openOptionsMenu();
    }

    @Override
    public void closeOptionsMenu() {
        if (!mBrowserToolbar.closeOptionsMenu())
            super.closeOptionsMenu();
    }

    @Override
    public void setFullScreen(final boolean fullscreen) {
      super.setFullScreen(fullscreen);
      mMainHandler.post(new Runnable() {
          public void run() {
              if (fullscreen)
                  mBrowserToolbar.hide();
              else
                  mBrowserToolbar.show();
          }
      });
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu aMenu) {
        if (aMenu == null)
            return false;

        if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning))
            aMenu.findItem(R.id.settings).setEnabled(false);

        Tab tab = Tabs.getInstance().getSelectedTab();
        MenuItem bookmark = aMenu.findItem(R.id.bookmark);
        MenuItem forward = aMenu.findItem(R.id.forward);
        MenuItem share = aMenu.findItem(R.id.share);
        MenuItem saveAsPDF = aMenu.findItem(R.id.save_as_pdf);
        MenuItem charEncoding = aMenu.findItem(R.id.char_encoding);
        MenuItem findInPage = aMenu.findItem(R.id.find_in_page);
        MenuItem desktopMode = aMenu.findItem(R.id.desktop_mode);

        
        
        aMenu.findItem(R.id.quit).setVisible(Build.VERSION.SDK_INT < 14);

        if (tab == null || tab.getURL() == null) {
            bookmark.setEnabled(false);
            forward.setEnabled(false);
            share.setEnabled(false);
            saveAsPDF.setEnabled(false);
            findInPage.setEnabled(false);
            return true;
        }

        bookmark.setEnabled(!tab.getURL().startsWith("about:reader"));
        bookmark.setCheckable(true);
        bookmark.setChecked(tab.isBookmark());
        bookmark.setIcon(tab.isBookmark() ? R.drawable.ic_menu_bookmark_remove : R.drawable.ic_menu_bookmark_add);

        forward.setEnabled(tab.canDoForward());
        desktopMode.setChecked(tab.getDesktopMode());
        desktopMode.setIcon(tab.getDesktopMode() ? R.drawable.ic_menu_desktop_mode_on : R.drawable.ic_menu_desktop_mode_off);

        String url = tab.getURL();
        if (ReaderModeUtils.isAboutReader(url)) {
            String urlFromReader = ReaderModeUtils.getUrlFromAboutReader(url);
            if (urlFromReader != null)
                url = urlFromReader;
        }

        
        String scheme = Uri.parse(url).getScheme();
        share.setEnabled(!(scheme.equals("about") || scheme.equals("chrome") ||
                           scheme.equals("file") || scheme.equals("resource")));

        
        saveAsPDF.setEnabled(!(tab.getURL().equals("about:home") ||
                               tab.getContentType().equals("application/vnd.mozilla.xul+xml")));

        
        findInPage.setEnabled(!tab.getURL().equals("about:home"));

        charEncoding.setVisible(GeckoPreferences.getCharEncodingState());

        return true;
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.abouthome_topsites_edit:
                mAboutHomeContent.editSite();
                return true;

            case R.id.abouthome_topsites_unpin:
                mAboutHomeContent.unpinSite(AboutHomeContent.UnpinFlags.REMOVE_PIN);
                return true;

            case R.id.abouthome_topsites_pin:
                mAboutHomeContent.pinSite();
                return true;

            case R.id.abouthome_topsites_remove:
                mAboutHomeContent.unpinSite(AboutHomeContent.UnpinFlags.REMOVE_HISTORY);
                return true;

        }
        return super.onContextItemSelected(item);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Tab tab = null;
        Intent intent = null;
        switch (item.getItemId()) {
            case R.id.bookmark:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    if (item.isChecked()) {
                        tab.removeBookmark();
                        Toast.makeText(this, R.string.bookmark_removed, Toast.LENGTH_SHORT).show();
                        item.setIcon(R.drawable.ic_menu_bookmark_add);
                    } else {
                        tab.addBookmark();
                        Toast.makeText(this, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        item.setIcon(R.drawable.ic_menu_bookmark_remove);
                    }
                }
                return true;
            case R.id.share:
                shareCurrentUrl();
                return true;
            case R.id.reload:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null)
                    tab.doReload();
                return true;
            case R.id.forward:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null)
                    tab.doForward();
                return true;
            case R.id.save_as_pdf:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SaveAs:PDF", null));
                return true;
            case R.id.settings:
                intent = new Intent(this, GeckoPreferences.class);
                startActivity(intent);
                return true;
            case R.id.addons:
                Tabs.getInstance().loadUrlInTab("about:addons");
                return true;
            case R.id.downloads:
                Tabs.getInstance().loadUrlInTab("about:downloads");
                return true;
            case R.id.apps:
                Tabs.getInstance().loadUrlInTab("about:apps");
                return true;
            case R.id.char_encoding:
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("CharEncoding:Get", null));
                return true;
            case R.id.find_in_page:
                mFindInPageBar.show();
                return true;
            case R.id.desktop_mode:
                Tab selectedTab = Tabs.getInstance().getSelectedTab();
                if (selectedTab == null)
                    return true;
                JSONObject args = new JSONObject();
                try {
                    args.put("desktopMode", !item.isChecked());
                    args.put("tabId", selectedTab.getId());
                } catch (JSONException e) {
                    Log.e(LOGTAG, "error building json arguments");
                }
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("DesktopMode:Change", args.toString()));
                return true;
            case R.id.new_tab:
                addTab();
                return true;
            case R.id.new_private_tab:
                addPrivateTab();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    


    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                return tab.showAllHistory();
            }
        }
        return super.onKeyLongPress(keyCode, event);
    }

    


 
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);

        if (!Intent.ACTION_MAIN.equals(intent.getAction()) || !mInitialized)
            return;

        (new GeckoAsyncTask<Void, Void, Boolean>(mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public synchronized Boolean doInBackground(Void... params) {
                
                SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
                String keyName = getPackageName() + ".feedback_launch_count";
                int launchCount = settings.getInt(keyName, 0);
                if (launchCount >= FEEDBACK_LAUNCH_COUNT)
                    return false;

                
                launchCount++;
                settings.edit().putInt(keyName, launchCount).commit();

                
                return launchCount == FEEDBACK_LAUNCH_COUNT;
            }

            @Override
            public void onPostExecute(Boolean shouldShowFeedbackPage) {
                if (shouldShowFeedbackPage)
                    Tabs.getInstance().loadUrlInTab("about:feedback");
            }
        }).execute();
    }

    private void resetFeedbackLaunchCount() {
        GeckoBackgroundThread.post(new Runnable() {
            @Override
            public synchronized void run() {
                SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
                settings.edit().putInt(getPackageName() + ".feedback_launch_count", 0).commit();
            }
        });
    }

    private void getLastUrl() {
        (new GeckoAsyncTask<Void, Void, String>(mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public synchronized String doInBackground(Void... params) {
                
                String url = "";
                Cursor c = BrowserDB.getRecentHistory(getContentResolver(), 1);
                if (c.moveToFirst()) {
                    url = c.getString(c.getColumnIndexOrThrow(Combined.URL));
                }
                c.close();
                return url;
            }

            @Override
            public void onPostExecute(String url) {
                
                if (url.length() > 0)
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feedback:LastUrl", url));
            }
        }).execute();
    }
}
