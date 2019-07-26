




package org.mozilla.gecko;

import org.mozilla.gecko.BrowserToolbar.EditingTarget;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.GeckoLayerClient;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PanZoomController;
import org.mozilla.gecko.health.BrowserHealthReporter;
import org.mozilla.gecko.home.BrowserSearch;
import org.mozilla.gecko.home.HomePager;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;
import org.mozilla.gecko.widget.GeckoActionProvider;
import org.mozilla.gecko.widget.ButtonToast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.NfcEvent;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.InputDevice;
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
import android.widget.RelativeLayout;
import android.widget.Toast;

import java.io.File;
import java.io.InputStream;
import java.net.URL;
import java.util.EnumSet;
import java.util.List;
import java.util.Vector;

abstract public class BrowserApp extends GeckoApp
                                 implements TabsPanel.TabsLayoutChangeListener,
                                            PropertyAnimator.PropertyAnimationListener,
                                            View.OnKeyListener,
                                            GeckoLayerClient.OnMetricsChangedListener,
                                            BrowserSearch.OnSearchListener,
                                            BrowserSearch.OnEditSuggestionListener,
                                            HomePager.OnNewTabsListener,
                                            HomePager.OnUrlOpenListener {
    private static final String LOGTAG = "GeckoBrowserApp";

    private static final String PREF_CHROME_DYNAMICTOOLBAR = "browser.chrome.dynamictoolbar";

    private static final String ABOUT_HOME = "about:home";

    private static final int TABS_ANIMATION_DURATION = 450;

    private static final int READER_ADD_SUCCESS = 0;
    private static final int READER_ADD_FAILED = 1;
    private static final int READER_ADD_DUPLICATE = 2;

    private static final String ADD_SHORTCUT_TOAST = "add_shortcut_toast";

    private static final String STATE_ABOUT_HOME_TOP_PADDING = "abouthome_top_padding";
    private static final String STATE_DYNAMIC_TOOLBAR_ENABLED = "dynamic_toolbar";

    private static final String BROWSER_SEARCH_TAG = "browser_search";
    private BrowserSearch mBrowserSearch;
    private View mBrowserSearchContainer;

    public static BrowserToolbar mBrowserToolbar;
    private HomePager mHomePager;
    private View mHomePagerContainer;
    protected Telemetry.Timer mAboutHomeStartupTimer = null;

    private static final int GECKO_TOOLS_MENU = -1;
    private static final int ADDON_MENU_OFFSET = 1000;
    private class MenuItemInfo {
        public int id;
        public String label;
        public String icon;
        public boolean checkable = false;
        public boolean checked = false;
        public boolean enabled = true;
        public boolean visible = true;
        public int parent;
    }

    private Vector<MenuItemInfo> mAddonMenuItemsCache;
    private PropertyAnimator mMainLayoutAnimator;

    private static final Interpolator sTabsInterpolator = new Interpolator() {
        @Override
        public float getInterpolation(float t) {
            t -= 1.0f;
            return t * t * t * t * t + 1.0f;
        }
    };

    private FindInPageBar mFindInPageBar;

    private boolean mAccessibilityEnabled = false;

    
    private static final int FEEDBACK_LAUNCH_COUNT = 15;

    
    private boolean mDynamicToolbarEnabled = false;

    
    private int mToolbarHeight = 0;

    
    
    private boolean mDynamicToolbarCanScroll = false;

    private Integer mPrefObserverId;

    private SharedPreferencesHelper mSharedPreferencesHelper;

    private OrderedBroadcastHelper mOrderedBroadcastHelper;

    private BrowserHealthReporter mBrowserHealthReporter;

    private SiteIdentityPopup mSiteIdentityPopup;

    public SiteIdentityPopup getSiteIdentityPopup() {
        if (mSiteIdentityPopup == null)
            mSiteIdentityPopup = new SiteIdentityPopup(this);

        return mSiteIdentityPopup;
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case LOCATION_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    maybeCancelFaviconLoad(tab);
                }
                
            case SELECTED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    if (isAboutHome(tab)) {
                        showHomePager(tab.getAboutHomePage());

                        if (isDynamicToolbarEnabled()) {
                            
                            mLayerView.getLayerMarginsAnimator().showMargins(false);
                        }
                    } else {
                        hideHomePager();
                    }

                    if (mSiteIdentityPopup != null)
                        mSiteIdentityPopup.dismiss();

                    final TabsPanel.Panel panel = tab.isPrivate()
                                                ? TabsPanel.Panel.PRIVATE_TABS
                                                : TabsPanel.Panel.NORMAL_TABS;
                    
                    
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (areTabsShown() && mTabsPanel.getCurrentPanel() != panel)
                                showTabs(panel);
                        }
                    });
                }
                break;
            case START:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    invalidateOptionsMenu();

                    if (isDynamicToolbarEnabled()) {
                        
                        mLayerView.getLayerMarginsAnimator().showMargins(false);
                    }
                }
                break;
            case LOAD_ERROR:
            case STOP:
            case MENU_UPDATED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    invalidateOptionsMenu();
                }
                break;
            case PAGE_SHOW:
                loadFavicon(tab);
                break;
            case LINK_FAVICON:
                
                
                
                
                if (tab.getState() != Tab.STATE_LOADING) {
                    loadFavicon(tab);
                }
                break;
        }
        super.onTabChanged(tab, msg, data);
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        
        
        if (event.getAction() != KeyEvent.ACTION_DOWN) {
            return false;
        }

        
        if (Build.VERSION.SDK_INT >= 9 &&
            (event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_BUTTON_Y:
                    
                    if (mBrowserToolbar.isVisible()) {
                        if (isDynamicToolbarEnabled() && !mHomePager.isVisible()) {
                            if (mLayerView != null) {
                                mLayerView.getLayerMarginsAnimator().hideMargins(false);
                                mLayerView.requestFocus();
                            }
                        } else {
                            
                            
                            mBrowserToolbar.requestFocusFromTouch();
                        }
                    } else {
                        if (mLayerView != null) {
                            mLayerView.getLayerMarginsAnimator().showMargins(false);
                        }
                        mBrowserToolbar.requestFocusFromTouch();
                    }
                    return true;
                case KeyEvent.KEYCODE_BUTTON_L1:
                    
                    Tabs.getInstance().getSelectedTab().doBack();
                    return true;
                case KeyEvent.KEYCODE_BUTTON_R1:
                    
                    Tabs.getInstance().getSelectedTab().doForward();
                    return true;
            }
        }

        
        final Tab tab = Tabs.getInstance().getSelectedTab();
        if (Build.VERSION.SDK_INT >= 11 && tab != null && event.isCtrlPressed()) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_LEFT_BRACKET:
                    tab.doBack();
                    return true;

                case KeyEvent.KEYCODE_RIGHT_BRACKET:
                    tab.doForward();
                    return true;

                case KeyEvent.KEYCODE_R:
                    tab.doReload();
                    return true;

                case KeyEvent.KEYCODE_PERIOD:
                    tab.doStop();
                    return true;

                case KeyEvent.KEYCODE_T:
                    addTab();
                    return true;

                case KeyEvent.KEYCODE_W:
                    Tabs.getInstance().closeTab(tab);
                    return true;

                case KeyEvent.KEYCODE_F:
                    mFindInPageBar.show();
                return true;
            }
        }

        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!mBrowserToolbar.isEditing() && onKey(null, keyCode, event)) {
            return true;
        }

        if (mBrowserToolbar.onKey(keyCode, event)) {
            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    void handleReaderListCountRequest() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final int count = BrowserDB.getReadingListCount(getContentResolver());
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:ListCountReturn", Integer.toString(count)));
            }
        });
    }

    void handleReaderAdded(int result, final String title, final String url) {
        if (result != READER_ADD_SUCCESS) {
            if (result == READER_ADD_FAILED) {
                showToast(R.string.reading_list_failed, Toast.LENGTH_SHORT);
            } else if (result == READER_ADD_DUPLICATE) {
                showToast(R.string.reading_list_duplicate, Toast.LENGTH_SHORT);
            }

            return;
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                BrowserDB.addReadingListItem(getContentResolver(), title, url);
                showToast(R.string.reading_list_added, Toast.LENGTH_SHORT);

                final int count = BrowserDB.getReadingListCount(getContentResolver());
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:ListCountUpdated", Integer.toString(count)));
            }
        });
    }

    void handleReaderRemoved(final String url) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                BrowserDB.removeReadingListItemWithURL(getContentResolver(), url);
                showToast(R.string.reading_list_removed, Toast.LENGTH_SHORT);

                final int count = BrowserDB.getReadingListCount(getContentResolver());
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:ListCountUpdated", Integer.toString(count)));
            }
        });
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mAboutHomeStartupTimer = new Telemetry.Timer("FENNEC_STARTUP_TIME_ABOUTHOME");

        String args = getIntent().getStringExtra("args");
        if (args != null && args.contains("--guest-mode")) {
            mProfile = GeckoProfile.createGuestProfile(this);
        }

        super.onCreate(savedInstanceState);

        mBrowserToolbar = (BrowserToolbar) findViewById(R.id.browser_toolbar);

        ((GeckoApp.MainLayout) mMainLayout).setTouchEventInterceptor(new HideTabsTouchListener());
        ((GeckoApp.MainLayout) mMainLayout).setMotionEventInterceptor(new MotionEventInterceptor() {
            @Override
            public boolean onInterceptMotionEvent(View view, MotionEvent event) {
                
                
                if (mLayerView != null && !mLayerView.hasFocus() && GamepadUtils.isPanningControl(event)) {
                    if (mHomePager.isVisible()) {
                        mLayerView.requestFocus();
                    } else {
                        mHomePager.requestFocus();
                    }
                }
                return false;
            }
        });

        mHomePager = (HomePager) findViewById(R.id.home_pager);
        mHomePagerContainer = findViewById(R.id.home_pager_container);

        mBrowserSearchContainer = findViewById(R.id.search_container);
        mBrowserSearch = (BrowserSearch) getSupportFragmentManager().findFragmentByTag(BROWSER_SEARCH_TAG);
        if (mBrowserSearch == null) {
            mBrowserSearch = BrowserSearch.newInstance();
            mBrowserSearch.setUserVisibleHint(false);
        }

        mBrowserToolbar.setOnActivateListener(new BrowserToolbar.OnActivateListener() {
            public void onActivate() {
                enterEditingMode(EditingTarget.CURRENT_TAB, HomePager.Page.VISITED);
            }
        });

        mBrowserToolbar.setOnCommitListener(new BrowserToolbar.OnCommitListener() {
            public void onCommit(EditingTarget target) {
                commitEditingMode(target);
            }
        });

        mBrowserToolbar.setOnDismissListener(new BrowserToolbar.OnDismissListener() {
            public void onDismiss() {
                dismissEditingMode();
            }
        });

        mBrowserToolbar.setOnFilterListener(new BrowserToolbar.OnFilterListener() {
            public void onFilter(String searchText, AutocompleteHandler handler) {
                filterEditingMode(searchText, handler);
            }
        });

        
        mBrowserToolbar.setOnKeyListener(this);

        if (mTabsPanel != null) {
            mTabsPanel.setTabsLayoutChangeListener(this);
            updateSideBarState();
        }

        mFindInPageBar = (FindInPageBar) findViewById(R.id.find_in_page);

        registerEventListener("CharEncoding:Data");
        registerEventListener("CharEncoding:State");
        registerEventListener("Feedback:LastUrl");
        registerEventListener("Feedback:OpenPlayStore");
        registerEventListener("Feedback:MaybeLater");
        registerEventListener("Telemetry:Gather");
        registerEventListener("Settings:Show");
        registerEventListener("Updater:Launch");
        registerEventListener("Reader:GoToReadingList");

        Distribution.init(this, getPackageResourcePath());
        JavaAddonManager.getInstance().init(getApplicationContext());
        mSharedPreferencesHelper = new SharedPreferencesHelper(getApplicationContext());
        mOrderedBroadcastHelper = new OrderedBroadcastHelper(getApplicationContext());
        mBrowserHealthReporter = new BrowserHealthReporter();

        if (AppConstants.MOZ_ANDROID_BEAM && Build.VERSION.SDK_INT >= 14) {
            NfcAdapter nfc = NfcAdapter.getDefaultAdapter(this);
            if (nfc != null) {
                nfc.setNdefPushMessageCallback(new NfcAdapter.CreateNdefMessageCallback() {
                    @Override
                    public NdefMessage createNdefMessage(NfcEvent event) {
                        Tab tab = Tabs.getInstance().getSelectedTab();
                        if (tab == null || tab.isPrivate()) {
                            return null;
                        }
                        return new NdefMessage(new NdefRecord[] { NdefRecord.createUri(tab.getURL()) });
                    }
                }, this);
            }
        }

        if (savedInstanceState != null) {
            mDynamicToolbarEnabled = savedInstanceState.getBoolean(STATE_DYNAMIC_TOOLBAR_ENABLED);
            
            
        }

        
        mPrefObserverId = PrefsHelper.getPref(PREF_CHROME_DYNAMICTOOLBAR, new PrefsHelper.PrefHandlerBase() {
            @Override
            public void prefValue(String pref, boolean value) {
                if (value == mDynamicToolbarEnabled) {
                    return;
                }
                mDynamicToolbarEnabled = value;

                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        
                        
                        if (!mAccessibilityEnabled) {
                            setDynamicToolbarEnabled(mDynamicToolbarEnabled);
                        }
                    }
                });
            }

            @Override
            public boolean isObserver() {
                
                
                return true;
            }
        });
    }

    @Override
    public void onBackPressed() {
        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            super.onBackPressed();
            return;
        }

        if (dismissEditingMode()) {
            return;
        }

        if (mSiteIdentityPopup != null && mSiteIdentityPopup.isShowing()) {
            mSiteIdentityPopup.dismiss();
            return;
        }

        super.onBackPressed();
    }

    private void showBookmarkDialog() {
        final Tab tab = Tabs.getInstance().getSelectedTab();
        final Prompt ps = new Prompt(this, new Prompt.PromptCallback() {
            @Override
            public void onPromptFinished(String result) {
                int itemId = -1;
                try {
                  itemId = new JSONObject(result).getInt("button");
                } catch(JSONException ex) {
                    Log.e(LOGTAG, "Exception reading bookmark prompt result", ex);
                }

                if (tab == null)
                    return;

                if (itemId == 0) {
                    new EditBookmarkDialog(BrowserApp.this).show(tab.getURL());
                } else if (itemId == 1) {
                    String url = tab.getURL();
                    String title = tab.getDisplayTitle();
                    Bitmap favicon = tab.getFavicon();
                    if (url != null && title != null) {
                        GeckoAppShell.createShortcut(title, url, url, favicon == null ? null : favicon, "");
                    }
                }
            }
        });

        final Prompt.PromptListItem[] items = new Prompt.PromptListItem[2];
        Resources res = getResources();
        items[0] = new Prompt.PromptListItem(res.getString(R.string.contextmenu_edit_bookmark));
        items[1] = new Prompt.PromptListItem(res.getString(R.string.contextmenu_add_to_launcher));

        ps.show("", "", items, false);
    }

    private void setDynamicToolbarEnabled(boolean enabled) {
        if (enabled) {
            if (mLayerView != null) {
                mLayerView.getLayerClient().setOnMetricsChangedListener(this);
            }
            setToolbarMargin(0);
            
            
        } else {
            
            
            if (mLayerView != null) {
                mLayerView.getLayerClient().setOnMetricsChangedListener(null);
            }
            
            
            if (mBrowserToolbar != null) {
                mBrowserToolbar.scrollTo(0, 0);
            }
        }

        refreshToolbarHeight();
    }

    private boolean isDynamicToolbarEnabled() {
        return mDynamicToolbarEnabled && !mAccessibilityEnabled;
    }

    private boolean isAboutHome(Tab tab) {
        return TextUtils.equals(ABOUT_HOME, tab.getURL());
    }

    @Override
    public boolean onSearchRequested() {
        return enterEditingMode(EditingTarget.CURRENT_TAB);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.pasteandgo: {
                String text = Clipboard.getText();
                if (!TextUtils.isEmpty(text)) {
                    Tabs.getInstance().loadUrl(text);
                }
                return true;
            }
            case R.id.site_settings: {
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Permissions:Get", null));
                return true;
            }
            case R.id.paste: {
                String text = Clipboard.getText();
                if (!TextUtils.isEmpty(text)) {
                    enterEditingMode(EditingTarget.CURRENT_TAB, text);
                }
                return true;
            }
            case R.id.share: {
                shareCurrentUrl();
                return true;
            }
            case R.id.subscribe: {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null && tab.getFeedsEnabled()) {
                    JSONObject args = new JSONObject();
                    try {
                        args.put("tabId", tab.getId());
                    } catch (JSONException e) {
                        Log.e(LOGTAG, "error building json arguments");
                    }
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feeds:Subscribe", args.toString()));
                }
                return true;
            }
            case R.id.copyurl: {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    String url = tab.getURL();
                    if (url != null) {
                        Clipboard.setText(url);
                    }
                }
                return true;
            }
            case R.id.add_to_launcher: {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    final String url = tab.getURL();
                    final String title = tab.getDisplayTitle();
                    if (url == null || title == null) {
                        return true;
                    }

                    Favicons favicons = Favicons.getInstance();
                    favicons.loadFavicon(url, tab.getFaviconURL(), 0,
                    new Favicons.OnFaviconLoadedListener() {
                        @Override
                        public void onFaviconLoaded(String url, Bitmap favicon) {
                            GeckoAppShell.createShortcut(title, url, url, favicon == null ? null : favicon, "");
                        }
                    });
                }
                return true;
            }
        }
        return false;
    }

    @Override
    public void setAccessibilityEnabled(boolean enabled) {
        if (mAccessibilityEnabled == enabled) {
            return;
        }

        
        
        mAccessibilityEnabled = enabled;
        if (mDynamicToolbarEnabled) {
            setDynamicToolbarEnabled(!enabled);
        }
    }

    @Override
    public void onDestroy() {
        if (mPrefObserverId != null) {
            PrefsHelper.removeObserver(mPrefObserverId);
            mPrefObserverId = null;
        }
        if (mBrowserToolbar != null)
            mBrowserToolbar.onDestroy();

        if (mSharedPreferencesHelper != null) {
            mSharedPreferencesHelper.uninit();
            mSharedPreferencesHelper = null;
        }

        if (mOrderedBroadcastHelper != null) {
            mOrderedBroadcastHelper.uninit();
            mOrderedBroadcastHelper = null;
        }

        if (mBrowserHealthReporter != null) {
            mBrowserHealthReporter.uninit();
            mBrowserHealthReporter = null;
        }

        unregisterEventListener("CharEncoding:Data");
        unregisterEventListener("CharEncoding:State");
        unregisterEventListener("Feedback:LastUrl");
        unregisterEventListener("Feedback:OpenPlayStore");
        unregisterEventListener("Feedback:MaybeLater");
        unregisterEventListener("Telemetry:Gather");
        unregisterEventListener("Settings:Show");
        unregisterEventListener("Updater:Launch");
        unregisterEventListener("Reader:GoToReadingList");

        if (AppConstants.MOZ_ANDROID_BEAM && Build.VERSION.SDK_INT >= 14) {
            NfcAdapter nfc = NfcAdapter.getDefaultAdapter(this);
            if (nfc != null) {
                
                
                
                nfc.setNdefPushMessageCallback(null, this);
            }
        }

        super.onDestroy();
    }

    @Override
    protected void initializeChrome() {
        super.initializeChrome();

        mBrowserToolbar.updateBackButton(false);
        mBrowserToolbar.updateForwardButton(false);

        mDoorHangerPopup.setAnchor(mBrowserToolbar.mFavicon);

        
        if (isDynamicToolbarEnabled()) {
            refreshToolbarHeight();
            mLayerView.getLayerMarginsAnimator().showMargins(true);
            mLayerView.getLayerClient().setOnMetricsChangedListener(this);
        }

        
        mLayerView.setOnKeyListener(this);
    }

    private void shareCurrentUrl() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null)
          return;

        String url = tab.getURL();
        if (url == null)
            return;

        if (ReaderModeUtils.isAboutReader(url))
            url = ReaderModeUtils.getUrlFromAboutReader(url);

        GeckoAppShell.openUriExternal(url, "text/plain", "", "",
                                      Intent.ACTION_SEND, tab.getDisplayTitle());
    }

    @Override
    protected void loadStartupTab(String url) {
        
        if (url != null || mRestoreMode != RESTORE_NONE) {
            mAboutHomeStartupTimer.cancel();
        }

        super.loadStartupTab(url);
    }

    private void setToolbarMargin(int margin) {
        ((RelativeLayout.LayoutParams) mGeckoLayout.getLayoutParams()).topMargin = margin;
        mGeckoLayout.requestLayout();
    }

    @Override
    public void onMetricsChanged(ImmutableViewportMetrics aMetrics) {
        if (mHomePager.isVisible() || mBrowserToolbar == null) {
            return;
        }

        
        
        if (aMetrics.getPageHeight() <= aMetrics.getHeight()) {
            if (mDynamicToolbarCanScroll) {
                mDynamicToolbarCanScroll = false;
                if (!mBrowserToolbar.isVisible()) {
                    ThreadUtils.postToUiThread(new Runnable() {
                        public void run() {
                            mLayerView.getLayerMarginsAnimator().showMargins(false);
                        }
                    });
                }
            }
        } else {
            mDynamicToolbarCanScroll = true;
        }

        final View toolbarLayout = mBrowserToolbar;
        final int marginTop = Math.round(aMetrics.marginTop);
        ThreadUtils.postToUiThread(new Runnable() {
            public void run() {
                toolbarLayout.scrollTo(0, toolbarLayout.getHeight() - marginTop);
            }
        });

        if (mFormAssistPopup != null)
            mFormAssistPopup.onMetricsChanged(aMetrics);
    }

    @Override
    public void onPanZoomStopped() {
        if (!isDynamicToolbarEnabled() || mHomePager.isVisible()) {
            return;
        }

        
        
        
        ImmutableViewportMetrics metrics = mLayerView.getViewportMetrics();
        if (metrics.getPageHeight() < metrics.getHeight()
              || metrics.marginTop >= mToolbarHeight / 2) {
            mLayerView.getLayerMarginsAnimator().showMargins(false);
        } else {
            mLayerView.getLayerMarginsAnimator().hideMargins(false);
        }
    }

    public void refreshToolbarHeight() {
        int height = 0;
        if (mBrowserToolbar != null) {
            height = mBrowserToolbar.getHeight();
        }

        if (!isDynamicToolbarEnabled() || mHomePager.isVisible()) {
            
            
            
            if (isDynamicToolbarEnabled()) {
                
                
                
                mHomePagerContainer.setPadding(0, height, 0, 0);
            } else {
                setToolbarMargin(height);
                height = 0;
            }
        } else {
            setToolbarMargin(0);
        }

        if (mLayerView != null && height != mToolbarHeight) {
            mToolbarHeight = height;
            mLayerView.getLayerMarginsAnimator().setMaxMargins(0, height, 0, 0);
            mLayerView.getLayerMarginsAnimator().showMargins(true);
        }
    }

    @Override
    void toggleChrome(final boolean aShow) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
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
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                mBrowserToolbar.show();
                mBrowserToolbar.requestFocusFromTouch();
            }
        });
    }

    @Override
    public void refreshChrome() {
        invalidateOptionsMenu();
        updateSideBarState();
        mTabsPanel.refresh();
        if (mSiteIdentityPopup != null) {
            mSiteIdentityPopup.dismiss();
        }
    }

    public View getActionBarLayout() {
        RelativeLayout actionBar = (RelativeLayout) LayoutInflater.from(this).inflate(R.layout.browser_toolbar, null);
        actionBar.setLayoutParams(new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.FILL_PARENT,
                                                                  (int) getResources().getDimension(R.dimen.browser_toolbar_height)));
        return actionBar;
    }

    @Override
    public boolean hasTabsSideBar() {
        return (mTabsPanel != null && mTabsPanel.isSideBar());
    }

    private void updateSideBarState() {
        if (mMainLayoutAnimator != null)
            mMainLayoutAnimator.stop();

        boolean isSideBar = (HardwareUtils.isTablet() && mOrientation == Configuration.ORIENTATION_LANDSCAPE);
        final int sidebarWidth = getResources().getDimensionPixelSize(R.dimen.tabs_sidebar_width);

        ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mTabsPanel.getLayoutParams();
        lp.width = (isSideBar ? sidebarWidth : ViewGroup.LayoutParams.FILL_PARENT);
        mTabsPanel.requestLayout();

        final boolean sidebarIsShown = (isSideBar && mTabsPanel.isShown());
        final int mainLayoutScrollX = (sidebarIsShown ? -sidebarWidth : 0);
        mMainLayout.scrollTo(mainLayoutScrollX, 0);

        mTabsPanel.setIsSideBar(isSideBar);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Menu:Add")) {
                MenuItemInfo info = new MenuItemInfo();
                info.label = message.getString("name");
                info.id = message.getInt("id") + ADDON_MENU_OFFSET;
                info.icon = message.optString("icon", null);
                info.checked = message.optBoolean("checked", false);
                info.enabled = message.optBoolean("enabled", true);
                info.visible = message.optBoolean("visible", true);
                info.checkable = message.optBoolean("checkable", false);
                int parent = message.optInt("parent", 0);
                info.parent = parent <= 0 ? parent : parent + ADDON_MENU_OFFSET;
                final MenuItemInfo menuItemInfo = info;
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        addAddonMenuItem(menuItemInfo);
                    }
                });
            } else if (event.equals("Menu:Remove")) {
                final int id = message.getInt("id") + ADDON_MENU_OFFSET;
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        removeAddonMenuItem(id);
                    }
                });
            } else if (event.equals("Menu:Update")) {
                final int id = message.getInt("id") + ADDON_MENU_OFFSET;
                final JSONObject options = message.getJSONObject("options");
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
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
                    @Override
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
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        dialogBuilder.show();
                    }
                });
            } else if (event.equals("CharEncoding:State")) {
                final boolean visible = message.getString("visible").equals("true");
                GeckoPreferences.setCharEncodingState(visible);
                final Menu menu = mMenu;
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
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
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (menu != null)
                            menu.findItem(R.id.settings).setEnabled(true);
                    }
                });

                
                if (AppConstants.MOZ_DATA_REPORTING) {
                    DataReportingNotification.checkAndNotifyPolicy(GeckoAppShell.getContext());
                }

            } else if (event.equals("Telemetry:Gather")) {
                Telemetry.HistogramAdd("PLACES_PAGES_COUNT", BrowserDB.getCount(getContentResolver(), "history"));
                Telemetry.HistogramAdd("PLACES_BOOKMARKS_COUNT", BrowserDB.getCount(getContentResolver(), "bookmarks"));
                Telemetry.HistogramAdd("FENNEC_FAVICONS_COUNT", BrowserDB.getCount(getContentResolver(), "favicons"));
                Telemetry.HistogramAdd("FENNEC_THUMBNAILS_COUNT", BrowserDB.getCount(getContentResolver(), "thumbnails"));
            } else if (event.equals("Reader:ListCountRequest")) {
                handleReaderListCountRequest();
            } else if (event.equals("Reader:Added")) {
                final int result = message.getInt("result");
                final String title = message.getString("title");
                final String url = message.getString("url");
                handleReaderAdded(result, title, url);
            } else if (event.equals("Reader:Removed")) {
                final String url = message.getString("url");
                handleReaderRemoved(url);
            } else if (event.equals("Reader:Share")) {
                final String title = message.getString("title");
                final String url = message.getString("url");
                GeckoAppShell.openUriExternal(url, "text/plain", "", "",
                                              Intent.ACTION_SEND, title);
            } else if (event.equals("Settings:Show")) {
                
                String resource = null;
                if (!message.isNull(GeckoPreferences.INTENT_EXTRA_RESOURCES)) {
                    resource = message.getString(GeckoPreferences.INTENT_EXTRA_RESOURCES);
                }
                Intent settingsIntent = new Intent(this, GeckoPreferences.class);
                GeckoPreferences.setResourceToOpen(settingsIntent, resource);
                startActivity(settingsIntent);
            } else if (event.equals("Updater:Launch")) {
                handleUpdaterLaunch();
            } else if (event.equals("Reader:GoToReadingList")) {
                openReadingList();
            } else {
                super.handleMessage(event, message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    @Override
    public void addTab() {
        Tabs.getInstance().loadUrl("about:home", Tabs.LOADURL_NEW_TAB);
    }

    @Override
    public void addPrivateTab() {
        Tabs.getInstance().loadUrl("about:privatebrowsing", Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_PRIVATE);
    }

    @Override
    public void showNormalTabs() {
        showTabs(TabsPanel.Panel.NORMAL_TABS);
    }

    @Override
    public void showPrivateTabs() {
        showTabs(TabsPanel.Panel.PRIVATE_TABS);
    }

    @Override
    public void showRemoteTabs() {
        showTabs(TabsPanel.Panel.REMOTE_TABS);
    }

    private void showTabs(TabsPanel.Panel panel) {
        if (Tabs.getInstance().getDisplayCount() == 0)
            return;

        mTabsPanel.show(panel);
    }

    @Override
    public void hideTabs() {
        mTabsPanel.hide();
    }

    @Override
    public boolean autoHideTabs() {
        if (areTabsShown()) {
            hideTabs();
            return true;
        }
        return false;
    }

    @Override
    public boolean areTabsShown() {
        return mTabsPanel.isShown();
    }

    @Override
    public void onTabsLayoutChange(int width, int height) {
        int animationLength = TABS_ANIMATION_DURATION;

        if (mMainLayoutAnimator != null) {
            animationLength = Math.max(1, animationLength - (int)mMainLayoutAnimator.getRemainingTime());
            mMainLayoutAnimator.stop(false);
        }

        if (areTabsShown()) {
            mTabsPanel.setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
        }

        mMainLayoutAnimator = new PropertyAnimator(animationLength, sTabsInterpolator);
        mMainLayoutAnimator.setPropertyAnimationListener(this);

        if (hasTabsSideBar()) {
            mMainLayoutAnimator.attach(mMainLayout,
                                       PropertyAnimator.Property.SCROLL_X,
                                       -width);
        } else {
            mMainLayoutAnimator.attach(mMainLayout,
                                       PropertyAnimator.Property.SCROLL_Y,
                                       -height);
        }

        mTabsPanel.prepareTabsAnimation(mMainLayoutAnimator);
        mBrowserToolbar.prepareTabsAnimation(mMainLayoutAnimator, areTabsShown());

        
        
        if (mLayerView != null && isDynamicToolbarEnabled()) {
            if (width > 0 && height > 0) {
                mLayerView.getLayerMarginsAnimator().setMarginsPinned(true);
                mLayerView.getLayerMarginsAnimator().showMargins(false);
            } else {
                mLayerView.getLayerMarginsAnimator().setMarginsPinned(false);
            }
        }

        mMainLayoutAnimator.start();
    }

    @Override
    public void onPropertyAnimationStart() {
    }

    @Override
    public void onPropertyAnimationEnd() {
        if (!areTabsShown()) {
            mTabsPanel.setVisibility(View.INVISIBLE);
            mTabsPanel.setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);
        }

        mTabsPanel.finishTabsAnimation();

        mMainLayoutAnimator = null;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        mToast.onSaveInstanceState(outState);
        outState.putBoolean(STATE_DYNAMIC_TOOLBAR_ENABLED, mDynamicToolbarEnabled);
        
        
    }

    private void openUrl(String url) {
        openUrl(url, null, mBrowserToolbar.getEditingTarget());
    }

    private void openUrl(String url, EditingTarget target) {
        openUrl(url, null, target);
    }

    private void openUrl(String url, String searchEngine) {
        openUrl(url, searchEngine, mBrowserToolbar.getEditingTarget());
    }

    private void openUrl(String url, String searchEngine, EditingTarget target) {
        mBrowserToolbar.setProgressVisibility(true);

        int flags = Tabs.LOADURL_NONE;
        if (target == EditingTarget.NEW_TAB) {
            flags |= Tabs.LOADURL_NEW_TAB;
        }

        Tabs.getInstance().loadUrl(url, searchEngine, -1, flags);

        hideBrowserSearch();
        mBrowserToolbar.cancelEdit();
    }

    private void openReadingList() {
        Tabs.getInstance().loadUrl(ABOUT_HOME, Tabs.LOADURL_READING_LIST);
    }

    
    private void loadFavicon(final Tab tab) {
        maybeCancelFaviconLoad(tab);

        int flags = Favicons.FLAG_SCALE | (tab.isPrivate() ? 0 : Favicons.FLAG_PERSIST);
        long id = Favicons.getInstance().loadFavicon(tab.getURL(), tab.getFaviconURL(), flags,
                        new Favicons.OnFaviconLoadedListener() {

            @Override
            public void onFaviconLoaded(String pageUrl, Bitmap favicon) {
                
                
                if (favicon == null)
                    return;

                
                
                if (!tab.getURL().equals(pageUrl))
                    return;

                tab.updateFavicon(favicon);
                tab.setFaviconLoadId(Favicons.NOT_LOADING);

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

    public boolean enterEditingMode(EditingTarget target) {
        return enterEditingMode(target, null, HomePager.Page.BOOKMARKS);
    }

    public boolean enterEditingMode(EditingTarget target, String url) {
        return enterEditingMode(target, url, HomePager.Page.BOOKMARKS);
    }

    public boolean enterEditingMode(EditingTarget target, HomePager.Page page) {
        return enterEditingMode(target, null, page);
    }

    boolean enterEditingMode(EditingTarget target, String url, HomePager.Page page) {
        
        if (TextUtils.isEmpty(url) && target == EditingTarget.CURRENT_TAB) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                
                
                url = tab.getUserSearch();
                if (TextUtils.isEmpty(url)) {
                    url = tab.getURL();
                }
            }
        }

        mBrowserToolbar.startEditing(target, url);
        animateShowHomePager(page);

        return true;
    }

    void commitEditingMode(EditingTarget target) {
        if (!mBrowserToolbar.isEditing()) {
            return;
        }

        final String url = mBrowserToolbar.commitEdit();
        animateHideHomePager();
        hideBrowserSearch();

        int flags = Tabs.LOADURL_USER_ENTERED;
        if (target == EditingTarget.NEW_TAB) {
            flags |= Tabs.LOADURL_NEW_TAB;
        }

        if (!TextUtils.isEmpty(url)) {
            Tabs.getInstance().loadUrl(url, flags);
        }
    }

    boolean dismissEditingMode() {
        if (!mBrowserToolbar.isEditing()) {
            return false;
        }

        mBrowserToolbar.cancelEdit();
        animateHideHomePager();
        hideBrowserSearch();

        return true;
    }

    void filterEditingMode(String searchTerm, AutocompleteHandler handler) {
        if (TextUtils.isEmpty(searchTerm)) {
            hideBrowserSearch();
        } else {
            showBrowserSearch();
            mBrowserSearch.filter(searchTerm, handler);
        }
    }

    private void animateShowHomePager(HomePager.Page page) {
        showHomePagerWithAnimation(true, page);
    }

    private void showHomePager(HomePager.Page page) {
        showHomePagerWithAnimation(false, page);
    }

    private void showHomePagerWithAnimation(boolean animate, HomePager.Page page) {
        if (mHomePager.isVisible()) {
            return;
        }

        
        refreshToolbarHeight();

        
        
        if (isDynamicToolbarEnabled() && mLayerView != null) {
            mLayerView.getLayerMarginsAnimator().showMargins(true);
        }

        
        mHomePager.show(getSupportFragmentManager(), page);
    }

    private void animateHideHomePager() {
        hideHomePagerWithAnimation(true);
    }

    private void hideHomePager() {
        hideHomePagerWithAnimation(false);
    }

    private void hideHomePagerWithAnimation(boolean animate) {
        if (!mHomePager.isVisible()) {
            return;
        }

        final Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null && isAboutHome(tab)) {
            return;
        }

        
        final FragmentManager fm = getSupportFragmentManager();
        final Fragment subPage = fm.findFragmentByTag(HomePager.SUBPAGE_TAG);
        if (subPage != null) {
            fm.beginTransaction().remove(subPage).commitAllowingStateLoss();
            fm.popBackStack();
        }

        
        mHomePager.hide();

        mBrowserToolbar.setShadowVisibility(true);
        mBrowserToolbar.setNextFocusDownId(R.id.layer_view);

        
        refreshToolbarHeight();
    }

    private void showBrowserSearch() {
        if (mBrowserSearch.getUserVisibleHint()) {
            return;
        }

        mBrowserSearchContainer.setVisibility(View.VISIBLE);

        getSupportFragmentManager().beginTransaction()
                .add(R.id.search_container, mBrowserSearch, BROWSER_SEARCH_TAG).commitAllowingStateLoss();
        mBrowserSearch.setUserVisibleHint(true);
    }

    private void hideBrowserSearch() {
        if (!mBrowserSearch.getUserVisibleHint()) {
            return;
        }

        mBrowserSearchContainer.setVisibility(View.INVISIBLE);

        getSupportFragmentManager().beginTransaction()
                .remove(mBrowserSearch).commitAllowingStateLoss();
        mBrowserSearch.setUserVisibleHint(false);
    }

    private class HideTabsTouchListener implements TouchEventInterceptor {
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

    private Menu findParentMenu(Menu menu, MenuItem item) {
        final int itemId = item.getItemId();

        final int count = (menu != null) ? menu.size() : 0;
        for (int i = 0; i < count; i++) {
            MenuItem menuItem = menu.getItem(i);
            if (menuItem.getItemId() == itemId) {
                return menu;
            }
            if (menuItem.hasSubMenu()) {
                Menu parent = findParentMenu(menuItem.getSubMenu(), item);
                if (parent != null) {
                    return parent;
                }
            }
        }

        return null;
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
        } else if (info.parent == GECKO_TOOLS_MENU) {
            MenuItem tools = mMenu.findItem(R.id.tools);
            menu = tools != null ? tools.getSubMenu() : mMenu;
        } else {
            MenuItem parent = mMenu.findItem(info.parent);
            if (parent == null)
                return;

            Menu parentMenu = findParentMenu(mMenu, parent);

            if (!parent.hasSubMenu()) {
                parentMenu.removeItem(parent.getItemId());
                menu = parentMenu.addSubMenu(Menu.NONE, parent.getItemId(), Menu.NONE, parent.getTitle());
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
            BitmapUtils.getDrawable(this, info.icon, new BitmapUtils.BitmapLoader() {
                @Override
                public void onBitmapFound(Drawable d) {
                    if (d == null) {
                        item.setIcon(R.drawable.ic_menu_addons_filler);
                        return;
                    }

                    item.setIcon(d);
                }
            });
        } else {
            item.setIcon(R.drawable.ic_menu_addons_filler);
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
                     item.checkable = options.optBoolean("checkable", item.checkable);
                     item.checked = options.optBoolean("checked", item.checked);
                     item.enabled = options.optBoolean("enabled", item.enabled);
                     item.visible = options.optBoolean("visible", item.visible);
                     break;
                 }
            }
        }

        if (mMenu == null)
            return;

        MenuItem menuItem = mMenu.findItem(id);
        if (menuItem != null) {
            menuItem.setCheckable(options.optBoolean("checkable", menuItem.isCheckable()));
            menuItem.setChecked(options.optBoolean("checked", menuItem.isChecked()));
            menuItem.setEnabled(options.optBoolean("enabled", menuItem.isEnabled()));
            menuItem.setVisible(options.optBoolean("visible", menuItem.isVisible()));
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        
        if (menu instanceof GeckoMenu && HardwareUtils.isTablet())
            ((GeckoMenu) menu).setActionItemBarPresenter(mBrowserToolbar);

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.browser_app_menu, mMenu);

        
        if (mAddonMenuItemsCache != null && !mAddonMenuItemsCache.isEmpty()) {
            for (MenuItemInfo item : mAddonMenuItemsCache) {
                 addAddonMenuItem(item);
            }

            mAddonMenuItemsCache.clear();
        }

        
        if (Build.VERSION.SDK_INT >= 14) {
            MenuItem share = mMenu.findItem(R.id.share);
            GeckoActionProvider provider = new GeckoActionProvider(this);
            share.setActionProvider(provider);
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

        if (isDynamicToolbarEnabled() && mLayerView != null)
            mLayerView.getLayerMarginsAnimator().showMargins(false);
    }

    @Override
    public void closeOptionsMenu() {
        if (!mBrowserToolbar.closeOptionsMenu())
            super.closeOptionsMenu();
    }

    @Override
    public void setFullScreen(final boolean fullscreen) {
        super.setFullScreen(fullscreen);
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (fullscreen) {
                    mBrowserToolbar.hide();
                    if (isDynamicToolbarEnabled()) {
                        mLayerView.getLayerMarginsAnimator().hideMargins(true);
                        mLayerView.getLayerMarginsAnimator().setMaxMargins(0, 0, 0, 0);
                    }
                } else {
                    mBrowserToolbar.show();
                    if (isDynamicToolbarEnabled()) {
                        mLayerView.getLayerMarginsAnimator().showMargins(true);
                        mLayerView.getLayerMarginsAnimator().setMaxMargins(0, mToolbarHeight, 0, 0);
                    }
                }
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
        MenuItem enterGuestMode = aMenu.findItem(R.id.enter_guest_mode);
        MenuItem exitGuestMode = aMenu.findItem(R.id.exit_guest_mode);

        
        
        aMenu.findItem(R.id.quit).setVisible(Build.VERSION.SDK_INT < 14 || HardwareUtils.isTelevision());

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

        
        if (Build.VERSION.SDK_INT >= 14) {
            GeckoActionProvider provider = (GeckoActionProvider) share.getActionProvider();
            if (provider != null) {
                Intent shareIntent = provider.getIntent();

                if (shareIntent == null) {
                    shareIntent = GeckoAppShell.getShareIntent(this, url,
                                                               "text/plain", tab.getDisplayTitle());
                    provider.setIntent(shareIntent);
                } else {
                    shareIntent.putExtra(Intent.EXTRA_TEXT, url);
                    shareIntent.putExtra(Intent.EXTRA_SUBJECT, tab.getDisplayTitle());
                }
            }
        }

        
        saveAsPDF.setEnabled(!(tab.getURL().equals("about:home") ||
                               tab.getContentType().equals("application/vnd.mozilla.xul+xml")));

        
        findInPage.setEnabled(!isAboutHome(tab));

        charEncoding.setVisible(GeckoPreferences.getCharEncodingState());

        if (mProfile.inGuestMode())
            exitGuestMode.setVisible(true);
        else
            enterGuestMode.setVisible(true);

        return true;
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
                        mToast.show(false,
                            getResources().getString(R.string.bookmark_added),
                            getResources().getString(R.string.bookmark_options),
                            null,
                            new ButtonToast.ToastListener() {
                                @Override
                                public void onButtonClicked() {
                                    showBookmarkDialog();
                                }

                                @Override
                                public void onToastHidden(ButtonToast.ReasonHidden reason) { }
                            });
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
            case R.id.enter_guest_mode:
                doRestart("--guest-mode");
                System.exit(0);
                return true;
            case R.id.exit_guest_mode:
                doRestart();
                System.exit(0);
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

        String action = intent.getAction();

        if (AppConstants.MOZ_ANDROID_BEAM && Build.VERSION.SDK_INT >= 10 && NfcAdapter.ACTION_NDEF_DISCOVERED.equals(action)) {
            String uri = intent.getDataString();
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(uri));
        }

        if (!Intent.ACTION_MAIN.equals(action) || !mInitialized) {
            return;
        }

        (new UiAsyncTask<Void, Void, Boolean>(ThreadUtils.getBackgroundHandler()) {
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
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feedback:Show", null));
            }
        }).execute();
    }

    @Override
    protected NotificationClient makeNotificationClient() {
        
        
        return new ServiceNotificationClient(getApplicationContext());
    }

    private void resetFeedbackLaunchCount() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public synchronized void run() {
                SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
                settings.edit().putInt(getPackageName() + ".feedback_launch_count", 0).commit();
            }
        });
    }

    private void getLastUrl() {
        (new UiAsyncTask<Void, Void, String>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public synchronized String doInBackground(Void... params) {
                
                String url = "";
                Cursor c = null;
                try {
                    c = BrowserDB.getRecentHistory(getContentResolver(), 1);
                    if (c.moveToFirst()) {
                        url = c.getString(c.getColumnIndexOrThrow(Combined.URL));
                    }
                } finally {
                    if (c != null)
                        c.close();
                }
                return url;
            }

            @Override
            public void onPostExecute(String url) {
                
                if (url.length() > 0)
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feedback:LastUrl", url));
            }
        }).execute();
    }

    
    @Override
    public void onNewTabs(String[] urls) {
        for (String url : urls) {
            openUrl(url, EditingTarget.NEW_TAB);
        }
    }

    
    @Override
    public void onUrlOpen(String url) {
        openUrl(url);
    }
 
    
    @Override
    public void onSearch(String engineId, String text) {
        openUrl(text, engineId);
    }

    
    @Override
    public void onEditSuggestion(String suggestion) {
        mBrowserToolbar.onEditSuggestion(suggestion);
    }

    @Override
    public int getLayout() { return R.layout.gecko_app; }

    @Override
    protected String getDefaultProfileName() {
        String profile = GeckoProfile.findDefaultProfile(this);
        return (profile != null ? profile : "default");
    }

    










    protected boolean handleUpdaterLaunch() {
        if ("release".equals(AppConstants.MOZ_UPDATE_CHANNEL) ||
            "beta".equals(AppConstants.MOZ_UPDATE_CHANNEL)) {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse("market://details?id=" + getPackageName()));
            startActivity(intent);
            return true;
        }

        if (AppConstants.MOZ_UPDATER) {
            Tabs.getInstance().loadUrlInTab("about:");
            return true;
        }

        Log.w(LOGTAG, "No candidate updater found; ignoring launch request.");
        return false;
    }
}
