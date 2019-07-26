




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.util.UiAsyncTask;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.Intent;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.Rect;
import android.net.Uri;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.NfcEvent;
import android.os.Build;
import android.os.Bundle;
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

import java.io.InputStream;
import java.net.URL;
import java.util.EnumSet;
import java.util.Vector;

abstract public class BrowserApp extends GeckoApp
                                 implements TabsPanel.TabsLayoutChangeListener,
                                            PropertyAnimator.PropertyAnimationListener,
                                            View.OnKeyListener {
    private static final String LOGTAG = "GeckoBrowserApp";

    private static final String PREF_CHROME_DYNAMICTOOLBAR = "browser.chrome.dynamictoolbar";

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
        @Override
        public float getInterpolation(float t) {
            t -= 1.0f;
            return t * t * t * t * t + 1.0f;
        }
    };

    private FindInPageBar mFindInPageBar;

    
    private static final int FEEDBACK_LAUNCH_COUNT = 15;

    
    private static final int TOOLBAR_ONLOAD_HIDE_DELAY = 2000;
    private static final float TOOLBAR_MOVEMENT_THRESHOLD = 0.3f;
    private boolean mDynamicToolbarEnabled = false;
    private View mToolbarSpacer = null;
    private float mLastTouchX = 0.0f;
    private float mLastTouchY = 0.0f;
    private float mToolbarSubpixelAccumulation = 0.0f;
    private boolean mToolbarLocked = false;
    private boolean mToolbarThresholdPassed = false;

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case LOCATION_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    maybeCancelFaviconLoad(tab);
                }
                
            case SELECTED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    if ("about:home".equals(tab.getURL())) {
                        showAboutHome();

                        if (mDynamicToolbarEnabled) {
                            
                            mBrowserToolbar.animateVisibility(true, 0);
                        }
                    } else {
                        hideAboutHome();
                    }

                    
                    SiteIdentityPopup.getInstance().dismiss();

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

                    if (mDynamicToolbarEnabled) {
                        
                        mBrowserToolbar.animateVisibility(true, 0);
                    }
                }
                break;
            case LOAD_ERROR:
            case STOP:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    if (!mAboutHomeShowing) {
                        if (mDynamicToolbarEnabled) {
                            
                            mBrowserToolbar.animateVisibility(false, TOOLBAR_ONLOAD_HIDE_DELAY);
                        }
                    }
                }
                
            case MENU_UPDATED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    invalidateOptionsMenu();
                }
                break;
            case PAGE_SHOW:
                loadFavicon(tab);
                break;
            case LINK_ADDED:
                
                
                
                
                if (tab.getState() != Tab.STATE_LOADING) {
                    loadFavicon(tab);
                }
                break;
        }
        super.onTabChanged(tab, msg, data);
    }

    @Override
    void handleClearHistory() {
        super.handleClearHistory();
        updateAboutHomeTopSites();
    }

    @Override
    public boolean onInterceptTouchEvent(View view, MotionEvent event) {
        if (!mDynamicToolbarEnabled) {
            return super.onInterceptTouchEvent(view, event);
        }

        int action = event.getActionMasked();
        int pointerCount = event.getPointerCount();

        
        
        
        
        
        mLayerView.getLayerClient().setClampOnFixedLayerMarginsChange(
            pointerCount == 0 || action == MotionEvent.ACTION_CANCEL ||
            action == MotionEvent.ACTION_UP);

        View toolbarView = mBrowserToolbar.getLayout();
        if (action == MotionEvent.ACTION_DOWN ||
            action == MotionEvent.ACTION_POINTER_DOWN) {
            if (pointerCount == 1) {
                mToolbarLocked = mToolbarThresholdPassed = false;
                mToolbarSubpixelAccumulation = 0.0f;
                mLastTouchX = event.getX();
                mLastTouchY = event.getY();
                return super.onInterceptTouchEvent(view, event);
            }

            
            mBrowserToolbar.animateVisibility(
                toolbarView.getScrollY() > toolbarView.getHeight() / 2 ?
                    false : true, 0);
        }

        
        
        
        if (pointerCount > 1 || mToolbarLocked) {
            return super.onInterceptTouchEvent(view, event);
        }

        
        
        if (pointerCount == 1 && action == MotionEvent.ACTION_POINTER_UP) {
            mLastTouchY = event.getY(1 - event.getActionIndex());
            return super.onInterceptTouchEvent(view, event);
        }

        
        
        float eventX = event.getX();
        float eventY = event.getY();
        if (Tabs.getInstance().getSelectedTab().getState() != Tab.STATE_LOADING) {
            int toolbarHeight = toolbarView.getHeight();
            float deltaX = mLastTouchX - eventX;
            float deltaY = mLastTouchY - eventY;
            int toolbarY = toolbarView.getScrollY();

            
            if (!mToolbarThresholdPassed) {
                float threshold = toolbarHeight * TOOLBAR_MOVEMENT_THRESHOLD;
                if (Math.abs(deltaY) > threshold) {
                    mToolbarThresholdPassed = true;
                    
                    
                    if (deltaY > 0 && toolbarY == toolbarHeight) {
                        mToolbarLocked = true;
                        return super.onInterceptTouchEvent(view, event);
                    }
                } else if (Math.abs(deltaX) > threshold) {
                    
                    
                    mToolbarLocked = true;
                    mToolbarThresholdPassed = true;
                    return super.onInterceptTouchEvent(view, event);
                } else {
                    
                    
                    return super.onInterceptTouchEvent(view, event);
                }
            } else if (action == MotionEvent.ACTION_MOVE) {
                
                mBrowserToolbar.cancelVisibilityAnimation();

                
                

                
                
                ImmutableViewportMetrics metrics =
                    mLayerView.getLayerClient().getViewportMetrics();
                float toolbarMaxY = Math.min(toolbarHeight,
                    Math.max(0, toolbarHeight - (metrics.pageRectTop -
                                                 metrics.viewportRectTop)));

                float newToolbarYf = Math.max(0, Math.min(toolbarMaxY,
                    toolbarY + deltaY + mToolbarSubpixelAccumulation));
                int newToolbarY = Math.round(newToolbarYf);
                mToolbarSubpixelAccumulation = (newToolbarYf - newToolbarY);

                toolbarView.scrollTo(0, newToolbarY);

                
                if (newToolbarY == 0 || newToolbarY == toolbarHeight) {
                    mLastTouchY = eventY;
                }
            } else if (action == MotionEvent.ACTION_UP ||
                       action == MotionEvent.ACTION_CANCEL) {
                
                
                mBrowserToolbar.animateVisibilityWithVelocityBias(
                    toolbarY > toolbarHeight / 2 ? false : true,
                    mLayerView.getPanZoomController().getVelocityVector().y);
            }
        }

        
        mLastTouchX = eventX;
        mLastTouchY = eventY;

        return super.onInterceptTouchEvent(view, event);
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
                        if (mDynamicToolbarEnabled &&
                            Boolean.FALSE.equals(mAboutHomeShowing)) {
                            mBrowserToolbar.animateVisibility(false, 0);
                            mLayerView.requestFocus();
                        } else {
                            
                            
                            mBrowserToolbar.requestFocusFromTouch();
                        }
                    } else {
                        mBrowserToolbar.animateVisibility(true, 0);
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

        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (onKey(null, keyCode, event)) {
            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    void handleReaderAdded(boolean success, final String title, final String url) {
        if (!success) {
            showToast(R.string.reading_list_failed, Toast.LENGTH_SHORT);
            return;
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                BrowserDB.addReadingListItem(getContentResolver(), title, url);
                showToast(R.string.reading_list_added, Toast.LENGTH_SHORT);
            }
        });
    }

    void handleReaderRemoved(final String url) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                BrowserDB.removeReadingListItemWithURL(getContentResolver(), url);
                showToast(R.string.reading_list_removed, Toast.LENGTH_SHORT);
            }
        });
    }

    @Override
    void onStatePurged() {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
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

        mToolbarSpacer = findViewById(R.id.toolbar_spacer);

        LinearLayout actionBar = (LinearLayout) getActionBarLayout();
        mMainLayout.addView(actionBar, 2);

        ((GeckoApp.MainLayout) mMainLayout).setOnInterceptTouchListener(new HideTabsTouchListener());

        mBrowserToolbar = new BrowserToolbar(this);
        mBrowserToolbar.from(actionBar);

        
        actionBar.setOnKeyListener(this);

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

        Distribution.init(this, getPackageResourcePath());
        JavaAddonManager.getInstance().init(getApplicationContext());

        if (Build.VERSION.SDK_INT >= 14) {
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

        
        PrefsHelper.getPref(PREF_CHROME_DYNAMICTOOLBAR, new PrefsHelper.PrefHandlerBase() {
            @Override
            public void prefValue(String pref, boolean value) {
                if (value == mDynamicToolbarEnabled) {
                    return;
                }
                mDynamicToolbarEnabled = value;

                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mDynamicToolbarEnabled) {
                            mToolbarSpacer.setPadding(0, 0, 0, 0);
                        } else {
                            
                            
                            mAboutHomeContent.setPadding(0, 0, 0, 0);
                            mBrowserToolbar.cancelVisibilityAnimation();
                            mBrowserToolbar.getLayout().scrollTo(0, 0);
                        }

                        
                        
                        ((BrowserToolbarLayout)mBrowserToolbar.getLayout()).refreshMargins();
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

        if (Build.VERSION.SDK_INT >= 14) {
            NfcAdapter nfc = NfcAdapter.getDefaultAdapter(this);
            if (nfc != null) {
                
                
                
                nfc.setNdefPushMessageCallback(null, this);
            }
        }
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
        ((BrowserToolbarLayout)mBrowserToolbar.getLayout()).refreshMargins();

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

        
        mLayerView.setOnKeyListener(this);
    }

    public void setToolbarHeight(int aHeight, int aVisibleHeight) {
        if (!mDynamicToolbarEnabled || Boolean.TRUE.equals(mAboutHomeShowing)) {
            
            
            
            if (mDynamicToolbarEnabled) {
                
                
                
                mAboutHomeContent.setPadding(0, aVisibleHeight, 0, 0);
            } else {
                mToolbarSpacer.setPadding(0, aVisibleHeight, 0, 0);
            }
            aHeight = aVisibleHeight = 0;
        } else {
            mToolbarSpacer.setPadding(0, 0, 0, 0);
        }

        
        
        
        GeckoAppShell.sendEventToGecko(
            GeckoEvent.createBroadcastEvent("Viewport:FixedMarginsChanged",
                "{ \"top\" : " + aHeight + ", \"right\" : 0, \"bottom\" : 0, \"left\" : 0 }"));

        if (mLayerView != null) {
            mLayerView.getLayerClient().setFixedLayerMargins(0, aVisibleHeight, 0, 0);
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
        updateSideBarState();
        mTabsPanel.refresh();

        if (mAboutHomeContent != null)
            mAboutHomeContent.refresh();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        String url = resultCode == Activity.RESULT_OK ? data.getStringExtra(AwesomeBar.URL_KEY) : null;
        mBrowserToolbar.fromAwesomeBarSearch(url);
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

    private void updateSideBarState() {
        if (mMainLayoutAnimator != null)
            mMainLayoutAnimator.stop();

        boolean isSideBar = (GeckoAppShell.isTablet() && mOrientation == Configuration.ORIENTATION_LANDSCAPE);

        ViewGroup.LayoutParams lp = mTabsPanel.getLayoutParams();
        if (isSideBar) {
            lp.width = getResources().getDimensionPixelSize(R.dimen.tabs_sidebar_width);
        } else {
            lp.width = ViewGroup.LayoutParams.FILL_PARENT;
        }
        mTabsPanel.requestLayout();

        final boolean changed = (mTabsPanel.isSideBar() != isSideBar);
        final boolean needsRelayout = (changed && mTabsPanel.isShown());

        if (needsRelayout) {
            final int width;
            final int scrollY;

            if (isSideBar) {
                width = lp.width;
                mMainLayout.scrollTo(0, 0);
            } else {
                width = 0;
            }

            mBrowserToolbar.adjustForTabsLayout(width);

            ((RelativeLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(width, 0, 0, 0);
            mGeckoLayout.requestLayout();
        }

        if (changed) {
            
            mBrowserToolbar.updateTabs(false);

            mTabsPanel.setIsSideBar(isSideBar);
            mBrowserToolbar.setIsSideBar(isSideBar);

            
            mBrowserToolbar.updateTabs(mTabsPanel.isShown());
        }
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
                try {
                    info.checkable = message.getBoolean("checkable");
                } catch (Exception ex) { }
                try { 
                    info.parent = message.getInt("parent") + ADDON_MENU_OFFSET;
                } catch (Exception ex) { }
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
            } else if (event.equals("Telemetry:Gather")) {
                Telemetry.HistogramAdd("PLACES_PAGES_COUNT", BrowserDB.getCount(getContentResolver(), "history"));
                Telemetry.HistogramAdd("PLACES_BOOKMARKS_COUNT", BrowserDB.getCount(getContentResolver(), "bookmarks"));
                Telemetry.HistogramAdd("FENNEC_FAVICONS_COUNT", BrowserDB.getCount(getContentResolver(), "favicons"));
                Telemetry.HistogramAdd("FENNEC_THUMBNAILS_COUNT", BrowserDB.getCount(getContentResolver(), "thumbnails"));
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
            } else {
                super.handleMessage(event, message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    @Override
    public void addTab() {
        showAwesomebar(AwesomeBar.Target.NEW_TAB);
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
        if (!hasTabsSideBar() && areTabsShown()) {
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
                ((RelativeLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(0, 0, 0, 0);
                mGeckoLayout.scrollTo(mTabsPanel.getWidth() * -1, 0);
                mGeckoLayout.requestLayout();
            }

            mMainLayoutAnimator.attach(mGeckoLayout,
                                       PropertyAnimator.Property.SCROLL_X,
                                       -width);
        } else {
            mMainLayoutAnimator.attach(mMainLayout,
                                       PropertyAnimator.Property.SCROLL_Y,
                                       -height);
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
                ((RelativeLayout.LayoutParams) mGeckoLayout.getLayoutParams()).setMargins(mTabsPanel.getWidth(), 0, 0, 0);
                mGeckoLayout.scrollTo(0, 0);
            }

            mGeckoLayout.requestLayout();
        } else {
            mTabsPanel.setVisibility(View.INVISIBLE);
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
        ThreadUtils.getUiHandler().postAtFrontOfQueue(r);
    }

    private void hideAboutHome() {
        
        
        if (mAboutHomeShowing != null && !mAboutHomeShowing)
            return;

        mBrowserToolbar.setShadowVisibility(true);
        mAboutHomeShowing = false;
        Runnable r = new AboutHomeRunnable(false);
        ThreadUtils.getUiHandler().postAtFrontOfQueue(r);
    }

    private class AboutHomeRunnable implements Runnable {
        boolean mShow;
        AboutHomeRunnable(boolean show) {
            mShow = show;
        }

        @Override
        public void run() {
            if (mShow) {
                if (mAboutHomeContent == null) {
                    mAboutHomeContent = (AboutHomeContent) findViewById(R.id.abouthome_content);
                    mAboutHomeContent.init();
                    mAboutHomeContent.update(AboutHomeContent.UpdateFlags.ALL);
                    mAboutHomeContent.setUriLoadCallback(new AboutHomeContent.UriLoadCallback() {
                        @Override
                        public void callback(String url) {
                            mBrowserToolbar.setProgressVisibility(true);
                            Tabs.getInstance().loadUrl(url);
                        }
                    });
                    mAboutHomeContent.setLoadCompleteCallback(new AboutHomeContent.VoidCallback() {
                        @Override
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

            
            ((BrowserToolbarLayout)mBrowserToolbar.getLayout()).refreshMargins();
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
                ThreadUtils.postToBackgroundThread(new Runnable() {
                    @Override
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
            } else {
                item.setIcon(R.drawable.ic_menu_addons_filler);
            }
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
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
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

        
        
        aMenu.findItem(R.id.quit).setVisible(Build.VERSION.SDK_INT < 14 || !isTouchDevice());

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

        String action = intent.getAction();

        if (Build.VERSION.SDK_INT >= 10 && NfcAdapter.ACTION_NDEF_DISCOVERED.equals(action)) {
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
                    Tabs.getInstance().loadUrlInTab("about:feedback");
            }
        }).execute();
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

    @Override
    public void onResume()
    {
        super.onResume();
        if (mAboutHomeContent != null) {
            mAboutHomeContent.refresh();
        }

    }
}
