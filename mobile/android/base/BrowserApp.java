




package org.mozilla.gecko;

import java.io.File;
import java.io.FileNotFoundException;
import java.lang.reflect.Method;
import java.net.URLEncoder;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Vector;

import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.DynamicToolbar.PinReason;
import org.mozilla.gecko.DynamicToolbar.VisibilityTransition;
import org.mozilla.gecko.GeckoProfileDirectories.NoMozillaDirectoryException;
import org.mozilla.gecko.Tabs.TabEvents;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.SuggestedSites;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.LoadFaviconTask;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.favicons.decoders.IconDirectoryEntry;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.activities.FxAccountGetStartedActivity;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerMarginsAnimator;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.health.BrowserHealthRecorder;
import org.mozilla.gecko.health.BrowserHealthReporter;
import org.mozilla.gecko.health.HealthRecorder;
import org.mozilla.gecko.health.SessionInformation;
import org.mozilla.gecko.home.BrowserSearch;
import org.mozilla.gecko.home.HomeBanner;
import org.mozilla.gecko.home.HomePager;
import org.mozilla.gecko.home.HomePager.OnUrlOpenInBackgroundListener;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.HomePanelsManager;
import org.mozilla.gecko.home.TopSitesPanel;
import org.mozilla.gecko.home.SearchEngine;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.GeckoMenuItem;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.preferences.ClearOnShutdownPref;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.prompts.Prompt;
import org.mozilla.gecko.prompts.PromptListItem;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.tabs.TabHistoryController;
import org.mozilla.gecko.tabs.TabHistoryFragment;
import org.mozilla.gecko.tabs.TabHistoryPage;
import org.mozilla.gecko.tabs.TabsPanel;
import org.mozilla.gecko.tabs.TabHistoryController.OnShowTabHistory;
import org.mozilla.gecko.tiles.TilesRecorder;
import org.mozilla.gecko.toolbar.AutocompleteHandler;
import org.mozilla.gecko.toolbar.BrowserToolbar;
import org.mozilla.gecko.toolbar.BrowserToolbar.TabEditingState;
import org.mozilla.gecko.toolbar.ToolbarProgressView;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.MenuUtils;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.PrefUtils;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;
import org.mozilla.gecko.widget.ButtonToast;
import org.mozilla.gecko.widget.ButtonToast.ToastListener;
import org.mozilla.gecko.widget.GeckoActionProvider;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.KeyguardManager;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
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
import android.os.StrictMode;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.content.LocalBroadcastManager;
import android.text.TextUtils;
import android.util.AttributeSet;
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
import android.view.ViewStub;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.Interpolator;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.Toast;
import android.widget.ViewFlipper;

import com.readystatesoftware.systembartint.SystemBarTintManager;

public class BrowserApp extends GeckoApp
                        implements TabsPanel.TabsLayoutChangeListener,
                                   PropertyAnimator.PropertyAnimationListener,
                                   View.OnKeyListener,
                                   LayerView.OnMetricsChangedListener,
                                   BrowserSearch.OnSearchListener,
                                   BrowserSearch.OnEditSuggestionListener,
                                   HomePager.OnNewTabsListener,
                                   OnUrlOpenListener,
                                   OnUrlOpenInBackgroundListener,
                                   ActionModeCompat.Presenter,
                                   LayoutInflater.Factory,
                                   TopSitesPanel.BrowserTilesRecorderProvider {
    private static final String LOGTAG = "GeckoBrowserApp";

    private static final int TABS_ANIMATION_DURATION = 450;

    private static final String ADD_SHORTCUT_TOAST = "add_shortcut_toast";
    public static final String GUEST_BROWSING_ARG = "--guest";

    private static final String STATE_ABOUT_HOME_TOP_PADDING = "abouthome_top_padding";

    private static final String BROWSER_SEARCH_TAG = "browser_search";
    private static final String ONBOARD_STARTPANE_TAG = "startpane_dialog";

    
    private static final int ACTIVITY_REQUEST_PREFERENCES = 1001;

    public static final String PREF_STARTPANE_ENABLED = "startpane_enabled";

    private BrowserSearch mBrowserSearch;
    private View mBrowserSearchContainer;

    public ViewGroup mBrowserChrome;
    public ViewFlipper mActionBarFlipper;
    public ActionModeCompatView mActionBar;
    private BrowserToolbar mBrowserToolbar;
    private ToolbarProgressView mProgressView;
    private HomePager mHomePager;
    private TabsPanel mTabsPanel;
    private ViewGroup mHomePagerContainer;
    protected Telemetry.Timer mAboutHomeStartupTimer;
    private ActionModeCompat mActionMode;
    private boolean mShowActionModeEndAnimation;
    private TabHistoryController tabHistoryController;

    private static final int GECKO_TOOLS_MENU = -1;
    private static final int ADDON_MENU_OFFSET = 1000;
    public static final String TAB_HISTORY_FRAGMENT_TAG = "tabHistoryFragment";
    private static class MenuItemInfo {
        public int id;
        public String label;
        public String icon;
        public boolean checkable;
        public boolean checked;
        public boolean enabled = true;
        public boolean visible = true;
        public int parent;
        public boolean added;   
    }

    
    public static enum GuestModeDialog {
        ENTERING,
        LEAVING
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
    private MediaCastingBar mMediaCastingBar;

    
    private static final int FEEDBACK_LAUNCH_COUNT = 15;

    
    private int mToolbarHeight;

    
    
    private boolean mDynamicToolbarCanScroll;

    private SharedPreferencesHelper mSharedPreferencesHelper;

    private OrderedBroadcastHelper mOrderedBroadcastHelper;

    private BroadcastReceiver mOnboardingReceiver;

    private BrowserHealthReporter mBrowserHealthReporter;

    private ReadingListHelper mReadingListHelper;

    private SystemBarTintManager mTintManager;

    
    private Integer mTargetTabForEditingMode;

    private final TabEditingState mLastTabEditingState = new TabEditingState();

    
    
    
    
    private boolean mHideWebContentOnAnimationEnd;

    private final DynamicToolbar mDynamicToolbar = new DynamicToolbar();

    private TilesRecorder mTilesRecorder;

    @Override
    public View onCreateView(final String name, final Context context, final AttributeSet attrs) {
        final View view;
        if (BrowserToolbar.class.getName().equals(name)) {
            view = BrowserToolbar.create(context, attrs);
        } else if (TabsPanel.TabsLayout.class.getName().equals(name)) {
            view = TabsPanel.createTabsLayout(context, attrs);
        } else {
            view = super.onCreateView(name, context, attrs);
        }
        return view;
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        if (tab == null) {
            
            
            if (msg != Tabs.TabEvents.RESTORED) {
                throw new IllegalArgumentException("onTabChanged:" + msg + " must specify a tab.");
            }
            return;
        }

        Log.d(LOGTAG, "BrowserApp.onTabChanged: " + tab.getId() + ": " + msg);
        switch(msg) {
            case LOCATION_CHANGE:
                
            case SELECTED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    updateHomePagerForTab(tab);
                }
                break;
            case START:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    invalidateOptionsMenu();

                    if (mDynamicToolbar.isEnabled()) {
                        mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
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
                tab.loadFavicon();
                break;
            case BOOKMARK_ADDED:
                showBookmarkAddedToast();
                break;
            case BOOKMARK_REMOVED:
                showBookmarkRemovedToast();
                break;

            case UNSELECTED:
                
                
                if (tab.isEditing()) {
                    
                    tab.getEditingState().copyFrom(mLastTabEditingState);
                }
                break;
        }

        if (NewTabletUI.isEnabled(this) && msg == TabEvents.SELECTED) {
            updateEditingModeForTab(tab);
        }

        super.onTabChanged(tab, msg, data);
    }

    private void updateEditingModeForTab(final Tab selectedTab) {
        
        
        
        
        
        if (!Tabs.getInstance().isSelectedTab(selectedTab)) {
            Log.w(LOGTAG, "updateEditingModeForTab: Given tab is expected to be selected tab");
        }

        saveTabEditingState(mLastTabEditingState);

        if (selectedTab.isEditing()) {
            enterEditingMode();
            restoreTabEditingState(selectedTab.getEditingState());
        } else {
            mBrowserToolbar.cancelEdit();
        }
    }

    private void saveTabEditingState(final TabEditingState editingState) {
        mBrowserToolbar.saveTabEditingState(editingState);
        editingState.setIsBrowserSearchShown(mBrowserSearch.getUserVisibleHint());
    }

    private void restoreTabEditingState(final TabEditingState editingState) {
        mBrowserToolbar.restoreTabEditingState(editingState);

        
        
        if (editingState.isBrowserSearchShown()) {
            showBrowserSearch();
        } else {
            hideBrowserSearch();
        }
    }

    private void showBookmarkAddedToast() {
        getButtonToast().show(false,
                getResources().getString(R.string.bookmark_added),
                ButtonToast.LENGTH_SHORT,
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
    }

    private void showBookmarkRemovedToast() {
        Toast.makeText(this, R.string.bookmark_removed, Toast.LENGTH_SHORT).show();
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (AndroidGamepadManager.handleKeyEvent(event)) {
            return true;
        }

        
        
        if (event.getAction() != KeyEvent.ACTION_DOWN) {
            return false;
        }

        if ((event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_BUTTON_Y:
                    
                    if (mBrowserChrome.getVisibility() == View.VISIBLE) {
                        if (mDynamicToolbar.isEnabled() && !isHomePagerVisible()) {
                            mDynamicToolbar.setVisible(false, VisibilityTransition.ANIMATE);
                            if (mLayerView != null) {
                                mLayerView.requestFocus();
                            }
                        } else {
                            
                            
                            mBrowserToolbar.requestFocusFromTouch();
                        }
                    } else {
                        mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
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
        if (Versions.feature11Plus && tab != null && event.isCtrlPressed()) {
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
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (AndroidGamepadManager.handleKeyEvent(event)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mAboutHomeStartupTimer = new Telemetry.UptimeTimer("FENNEC_STARTUP_TIME_ABOUTHOME");

        final Intent intent = getIntent();
        final GeckoProfile p = GeckoProfile.get(this);
        if (p != null && !p.inGuestMode()) {
            
            
            
            
            GeckoProfile.maybeCleanupGuestProfile(this);
        }

        
        
        
        ((GeckoApplication) getApplication()).prepareLightweightTheme();
        super.onCreate(savedInstanceState);

        final Context appContext = getApplicationContext();

        setupSystemUITinting();

        mBrowserChrome = (ViewGroup) findViewById(R.id.browser_chrome);
        mActionBarFlipper = (ViewFlipper) findViewById(R.id.browser_actionbar);
        mActionBar = (ActionModeCompatView) findViewById(R.id.actionbar);

        mBrowserToolbar = (BrowserToolbar) findViewById(R.id.browser_toolbar);
        mProgressView = (ToolbarProgressView) findViewById(R.id.progress);
        mBrowserToolbar.setProgressBar(mProgressView);

        
        tabHistoryController = new TabHistoryController(new OnShowTabHistory() {
            @Override
            public void onShowHistory(final List<TabHistoryPage> historyPageList, final int toIndex) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        final TabHistoryFragment fragment = TabHistoryFragment.newInstance(historyPageList, toIndex);
                        final FragmentManager fragmentManager = getSupportFragmentManager();
                        GeckoAppShell.vibrateOnHapticFeedbackEnabled(getResources().getInteger(R.integer.long_press_vibrate_msec));
                        fragment.show(R.id.tab_history_panel, fragmentManager.beginTransaction(), TAB_HISTORY_FRAGMENT_TAG);
                    }
                });
            }
        });
        mBrowserToolbar.setTabHistoryController(tabHistoryController);

        final String action = intent.getAction();
        if (Intent.ACTION_VIEW.equals(action)) {
            
            mBrowserToolbar.setTitle(intent.getDataString());

            Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.INTENT);
        } else if (GuestSession.NOTIFICATION_INTENT.equals(action)) {
            GuestSession.handleIntent(this, intent);
        }

        if (NewTabletUI.isEnabled(this)) {
            findViewById(R.id.new_tablet_tab_strip).setVisibility(View.VISIBLE);
        }

        ((GeckoApp.MainLayout) mMainLayout).setTouchEventInterceptor(new HideOnTouchListener());
        ((GeckoApp.MainLayout) mMainLayout).setMotionEventInterceptor(new MotionEventInterceptor() {
            @Override
            public boolean onInterceptMotionEvent(View view, MotionEvent event) {
                
                
                if (mLayerView != null && !mLayerView.hasFocus() && GamepadUtils.isPanningControl(event)) {
                    if (mHomePager == null) {
                        return false;
                    }

                    if (isHomePagerVisible()) {
                        mLayerView.requestFocus();
                    } else {
                        mHomePager.requestFocus();
                    }
                }
                return false;
            }
        });

        mHomePagerContainer = (ViewGroup) findViewById(R.id.home_pager_container);

        mBrowserSearchContainer = findViewById(R.id.search_container);
        mBrowserSearch = (BrowserSearch) getSupportFragmentManager().findFragmentByTag(BROWSER_SEARCH_TAG);
        if (mBrowserSearch == null) {
            mBrowserSearch = BrowserSearch.newInstance();
            mBrowserSearch.setUserVisibleHint(false);
        }

        setBrowserToolbarListeners();

        mFindInPageBar = (FindInPageBar) findViewById(R.id.find_in_page);
        mMediaCastingBar = (MediaCastingBar) findViewById(R.id.media_casting);

        EventDispatcher.getInstance().registerGeckoThreadListener((GeckoEventListener)this,
            "Menu:Open",
            "Menu:Update",
            "Search:Keyword",
            "Prompt:ShowTop",
            "Accounts:Exist");

        EventDispatcher.getInstance().registerGeckoThreadListener((NativeEventListener)this,
            "Accounts:Create",
            "CharEncoding:Data",
            "CharEncoding:State",
            "Feedback:LastUrl",
            "Feedback:MaybeLater",
            "Feedback:OpenPlayStore",
            "Menu:Add",
            "Menu:Remove",
            "Reader:Share",
            "Settings:Show",
            "Telemetry:Gather",
            "Updater:Launch",
            "BrowserToolbar:Visibility");

        Distribution distribution = Distribution.init(this);

        
        final SuggestedSites suggestedSites = new SuggestedSites(appContext, distribution);
        BrowserDB.setSuggestedSites(suggestedSites);

        JavaAddonManager.getInstance().init(appContext);
        mSharedPreferencesHelper = new SharedPreferencesHelper(appContext);
        mOrderedBroadcastHelper = new OrderedBroadcastHelper(appContext);
        mBrowserHealthReporter = new BrowserHealthReporter();
        mReadingListHelper = new ReadingListHelper(appContext);

        if (AppConstants.MOZ_ANDROID_BEAM) {
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
            mDynamicToolbar.onRestoreInstanceState(savedInstanceState);
            mHomePagerContainer.setPadding(0, savedInstanceState.getInt(STATE_ABOUT_HOME_TOP_PADDING), 0, 0);
        }

        mDynamicToolbar.setEnabledChangedListener(new DynamicToolbar.OnEnabledChangedListener() {
            @Override
            public void onEnabledChanged(boolean enabled) {
                setDynamicToolbarEnabled(enabled);
            }
        });

        
        IconDirectoryEntry.setMaxBPP(GeckoAppShell.getScreenDepth());

        Class<?> mediaManagerClass = getMediaPlayerManager();
        if (mediaManagerClass != null) {
            try {
                Method init = mediaManagerClass.getMethod("init", Context.class);
                init.invoke(null, this);
            } catch(Exception ex) {
                Log.e(LOGTAG, "Error initializing media manager", ex);
            }
        }

        mTilesRecorder = new TilesRecorder();
    }

    private void setupSystemUITinting() {
        if (!Versions.feature19Plus) {
            return;
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);

        mTintManager = new SystemBarTintManager(this);
        mTintManager.setTintColor(getResources().getColor(R.color.background_tabs));
        updateSystemUITinting(mRootLayout.getSystemUiVisibility());

        mRootLayout.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener() {
            @Override
            public void onSystemUiVisibilityChange(int visibility) {
                updateSystemUITinting(visibility);
            }
        });
    }

    private void updateSystemUITinting(int visibility) {
        final boolean shouldTint = (visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0 &&
                                   (visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0;
        mTintManager.setStatusBarTintEnabled(shouldTint);
    }

    






    private void checkStartPane(Context context, String intentAction) {
        final StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();

        try {
            final SharedPreferences prefs = GeckoSharedPrefs.forProfile(this);

            if (prefs.getBoolean(PREF_STARTPANE_ENABLED, false)) {
                if (!Intent.ACTION_VIEW.equals(intentAction)) {
                    final DialogFragment dialog = new StartPane();
                    dialog.show(getSupportFragmentManager(), ONBOARD_STARTPANE_TAG);
                }
                
                prefs.edit().putBoolean(PREF_STARTPANE_ENABLED, false).apply();
            }
        } finally {
            StrictMode.setThreadPolicy(savedPolicy);
        }
    }

    private Class<?> getMediaPlayerManager() {
        if (AppConstants.MOZ_MEDIA_PLAYER) {
            try {
                return Class.forName("org.mozilla.gecko.MediaPlayerManager");
            } catch(Exception ex) {
                
                Log.e(LOGTAG, "No native casting support", ex);
            }
        }

        return null;
    }

    @Override
    public void onBackPressed() {
        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            super.onBackPressed();
            return;
        }

        if (mBrowserToolbar.onBackPressed()) {
            return;
        }

        if (mActionMode != null) {
            endActionModeCompat();
            return;
        }

        super.onBackPressed();
    }

    @Override
    public void onAttachedToWindow() {
        
        checkStartPane(this, getIntent().getAction());
    }

    @Override
    public void onResume() {
        super.onResume();

        final String args = ContextUtils.getStringExtra(getIntent(), "args");
        
        
        
        final boolean enableGuestSession = GuestSession.shouldUse(this, args);
        final boolean inGuestSession = GeckoProfile.get(this).inGuestMode();
        if (enableGuestSession != inGuestSession) {
            doRestart(getIntent());
            GeckoAppShell.systemExit();
            return;
        }

        final KeyguardManager manager = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
        
        if (Versions.feature11Plus && manager != null) {
            
            
            
            if (manager.isKeyguardLocked() && (GeckoProfile.get(this).inGuestMode() || !manager.isKeyguardSecure())) {
                GuestSession.configureWindow(getWindow());
            } else {
                GuestSession.unconfigureWindow(getWindow());
            }
        }

        EventDispatcher.getInstance().unregisterGeckoThreadListener((GeckoEventListener)this,
            "Prompt:ShowTop");
    }

    @Override
    public void onPause() {
        super.onPause();
        
        EventDispatcher.getInstance().registerGeckoThreadListener((GeckoEventListener)this,
            "Prompt:ShowTop");

        final LocalBroadcastManager lbm = LocalBroadcastManager.getInstance(this);
        lbm.unregisterReceiver(mOnboardingReceiver);
    }

    @Override
    public void onStart() {
        super.onStart();

        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                if (getProfile().inGuestMode()) {
                    GuestSession.showNotification(BrowserApp.this);
                } else {
                    
                    
                    
                    GuestSession.hideNotification(BrowserApp.this);
                }
            }
        });
    }

    @Override
    public void onStop() {
        super.onStop();

        
        GuestSession.hideNotification(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        
        
        
        
        if (mInitialized && hasFocus &&
            Versions.preHC && isHomePagerVisible() &&
            mLayerView.getVisibility() != View.VISIBLE){
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    mLayerView.showSurface();
                }
            });
        }
    }

    private void setBrowserToolbarListeners() {
        mBrowserToolbar.setOnActivateListener(new BrowserToolbar.OnActivateListener() {
            @Override
            public void onActivate() {
                enterEditingMode();
            }
        });

        mBrowserToolbar.setOnCommitListener(new BrowserToolbar.OnCommitListener() {
            @Override
            public void onCommit() {
                commitEditingMode();
            }
        });

        mBrowserToolbar.setOnDismissListener(new BrowserToolbar.OnDismissListener() {
            @Override
            public void onDismiss() {
                mBrowserToolbar.cancelEdit();
            }
        });

        mBrowserToolbar.setOnFilterListener(new BrowserToolbar.OnFilterListener() {
            @Override
            public void onFilter(String searchText, AutocompleteHandler handler) {
                filterEditingMode(searchText, handler);
            }
        });

        mBrowserToolbar.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (isHomePagerVisible()) {
                    mHomePager.onToolbarFocusChange(hasFocus);
                }
            }
        });

        mBrowserToolbar.setOnStartEditingListener(new BrowserToolbar.OnStartEditingListener() {
            @Override
            public void onStartEditing() {
                final Tab selectedTab = Tabs.getInstance().getSelectedTab();
                if (selectedTab != null) {
                    selectedTab.setIsEditing(true);
                }

                
                mDoorHangerPopup.disable();
            }
        });

        mBrowserToolbar.setOnStopEditingListener(new BrowserToolbar.OnStopEditingListener() {
            @Override
            public void onStopEditing() {
                final Tab selectedTab = Tabs.getInstance().getSelectedTab();
                if (selectedTab != null) {
                    selectedTab.setIsEditing(false);
                }

                selectTargetTabForEditingMode();

                
                
                
                
                
                hideBrowserSearch();
                hideHomePager();

                
                mDoorHangerPopup.enable();
            }
        });

        
        mBrowserToolbar.setOnKeyListener(this);
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
                        GeckoAppShell.createShortcut(title, url, favicon);
                    }
                }
            }
        });

        final PromptListItem[] items = new PromptListItem[2];
        Resources res = getResources();
        items[0] = new PromptListItem(res.getString(R.string.contextmenu_edit_bookmark));
        items[1] = new PromptListItem(res.getString(R.string.contextmenu_add_to_launcher));

        ps.show("", "", items, ListView.CHOICE_MODE_NONE);
    }

    private void setDynamicToolbarEnabled(boolean enabled) {
        ThreadUtils.assertOnUiThread();

        if (enabled) {
            if (mLayerView != null) {
                mLayerView.setOnMetricsChangedListener(this);
            }
            setToolbarMargin(0);
            mHomePagerContainer.setPadding(0, mBrowserChrome.getHeight(), 0, 0);
        } else {
            
            
            if (mLayerView != null) {
                mLayerView.setOnMetricsChangedListener(null);
            }
            mHomePagerContainer.setPadding(0, 0, 0, 0);
            if (mBrowserChrome != null) {
                ViewHelper.setTranslationY(mBrowserChrome, 0);
            }
        }

        refreshToolbarHeight();
    }

    private static boolean isAboutHome(final Tab tab) {
        return AboutPages.isAboutHome(tab.getURL());
    }

    @Override
    public boolean onSearchRequested() {
        enterEditingMode();
        return true;
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == R.id.pasteandgo) {
            String text = Clipboard.getText();
            if (!TextUtils.isEmpty(text)) {
                loadUrlOrKeywordSearch(text);
                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.CONTEXT_MENU);
                Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, "pasteandgo");
            }
            return true;
        }

        if (itemId == R.id.site_settings) {
            
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Permissions:Get", null));
            if (Versions.preHC) {
                Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, "site_settings");
            }
            return true;
        }

        if (itemId == R.id.paste) {
            String text = Clipboard.getText();
            if (!TextUtils.isEmpty(text)) {
                enterEditingMode(text);
                showBrowserSearch();
                mBrowserSearch.filter(text, null);
                Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, "paste");
            }
            return true;
        }

        if (itemId == R.id.subscribe) {
            
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null && tab.hasFeeds()) {
                JSONObject args = new JSONObject();
                try {
                    args.put("tabId", tab.getId());
                } catch (JSONException e) {
                    Log.e(LOGTAG, "error building json arguments", e);
                }
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feeds:Subscribe", args.toString()));
                if (Versions.preHC) {
                    Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, "subscribe");
                }
            }
            return true;
        }

        if (itemId == R.id.add_search_engine) {
            
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null && tab.hasOpenSearch()) {
                JSONObject args = new JSONObject();
                try {
                    args.put("tabId", tab.getId());
                } catch (JSONException e) {
                    Log.e(LOGTAG, "error building json arguments", e);
                    return true;
                }
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:Add", args.toString()));

                if (Versions.preHC) {
                    Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, "add_search_engine");
                }
            }
            return true;
        }

        if (itemId == R.id.copyurl) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                String url = tab.getURL();
                if (url != null) {
                    Clipboard.setText(url);
                    Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, "copyurl");
                }
            }
            return true;
        }

        if (itemId == R.id.add_to_launcher) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab == null) {
                return true;
            }

            final String url = tab.getURL();
            final String title = tab.getDisplayTitle();
            if (url == null || title == null) {
                return true;
            }

            final OnFaviconLoadedListener listener = new GeckoAppShell.CreateShortcutFaviconLoadedListener(url, title);
            Favicons.getSizedFavicon(getContext(),
                                     url,
                                     tab.getFaviconURL(),
                                     Integer.MAX_VALUE,
                                     LoadFaviconTask.FLAG_PERSIST,
                                     listener);

            return true;
        }

        return false;
    }

    @Override
    public void setAccessibilityEnabled(boolean enabled) {
        mDynamicToolbar.setAccessibilityEnabled(enabled);
    }

    @Override
    public void onDestroy() {
        mDynamicToolbar.destroy();

        if (mBrowserToolbar != null)
            mBrowserToolbar.onDestroy();

        if (mFindInPageBar != null) {
            mFindInPageBar.onDestroy();
            mFindInPageBar = null;
        }

        if (mMediaCastingBar != null) {
            mMediaCastingBar.onDestroy();
            mMediaCastingBar = null;
        }

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

        if (mReadingListHelper != null) {
            mReadingListHelper.uninit();
            mReadingListHelper = null;
        }

        EventDispatcher.getInstance().unregisterGeckoThreadListener((GeckoEventListener)this,
            "Menu:Open",
            "Menu:Update",
            "Search:Keyword",
            "Prompt:ShowTop",
            "Accounts:Exist");

        EventDispatcher.getInstance().unregisterGeckoThreadListener((NativeEventListener)this,
            "Accounts:Create",
            "CharEncoding:Data",
            "CharEncoding:State",
            "Feedback:LastUrl",
            "Feedback:MaybeLater",
            "Feedback:OpenPlayStore",
            "Menu:Add",
            "Menu:Remove",
            "Reader:Share",
            "Settings:Show",
            "Telemetry:Gather",
            "Updater:Launch",
            "BrowserToolbar:Visibility");

        if (AppConstants.MOZ_ANDROID_BEAM) {
            NfcAdapter nfc = NfcAdapter.getDefaultAdapter(this);
            if (nfc != null) {
                
                
                
                nfc.setNdefPushMessageCallback(null, this);
            }
        }

        Class<?> mediaManagerClass = getMediaPlayerManager();
        if (mediaManagerClass != null) {
            try {
                Method destroy = mediaManagerClass.getMethod("onDestroy",  (Class[]) null);
                destroy.invoke(null);
            } catch(Exception ex) {
                Log.e(LOGTAG, "Error destroying media manager", ex);
            }
        }

        super.onDestroy();
    }

    @Override
    protected void initializeChrome() {
        super.initializeChrome();

        mDoorHangerPopup.setAnchor(mBrowserToolbar.getDoorHangerAnchor());

        mDynamicToolbar.setLayerView(mLayerView);
        setDynamicToolbarEnabled(mDynamicToolbar.isEnabled());

        
        mLayerView.setOnKeyListener(this);

        
        if (HardwareUtils.isTablet()) {
            onCreatePanelMenu(Window.FEATURE_OPTIONS_PANEL, null);
            invalidateOptionsMenu();
        }
    }

    private void shareCurrentUrl() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null) {
            return;
        }

        String url = tab.getURL();
        if (url == null) {
            return;
        }

        if (AboutPages.isAboutReader(url)) {
            url = ReaderModeUtils.getUrlFromAboutReader(url);
        }

        GeckoAppShell.openUriExternal(url, "text/plain", "", "",
                                      Intent.ACTION_SEND, tab.getDisplayTitle());

        
        Telemetry.sendUIEvent(TelemetryContract.Event.SHARE, TelemetryContract.Method.LIST);
    }

    @Override
    protected void loadStartupTab(String url) {
        
        if (url != null || mShouldRestore) {
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
        if (isHomePagerVisible() || mBrowserChrome == null) {
            return;
        }

        
        
        if (aMetrics.getPageHeight() <= aMetrics.getHeight()) {
            if (mDynamicToolbarCanScroll) {
                mDynamicToolbarCanScroll = false;
                if (mBrowserChrome.getVisibility() != View.VISIBLE) {
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
                        }
                    });
                }
            }
        } else {
            mDynamicToolbarCanScroll = true;
        }

        final View browserChrome = mBrowserChrome;
        final ToolbarProgressView progressView = mProgressView;
        final int marginTop = Math.round(aMetrics.marginTop);
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                final float translationY = marginTop - browserChrome.getHeight();
                ViewHelper.setTranslationY(browserChrome, translationY);
                ViewHelper.setTranslationY(progressView, translationY);

                if (mDoorHangerPopup.isShowing()) {
                    mDoorHangerPopup.updatePopup();
                }
            }
        });

        if (mFormAssistPopup != null)
            mFormAssistPopup.onMetricsChanged(aMetrics);
    }

    @Override
    public void onPanZoomStopped() {
        if (!mDynamicToolbar.isEnabled() || isHomePagerVisible()) {
            return;
        }

        
        
        
        
        ImmutableViewportMetrics metrics = mLayerView.getViewportMetrics();
        final float height = metrics.viewportRectBottom - metrics.viewportRectTop;
        if (metrics.getPageHeight() < metrics.getHeight()
              || metrics.marginTop >= mToolbarHeight / 2
              || (metrics.pageRectBottom == metrics.viewportRectBottom && metrics.pageRectBottom > 2*height)) {
            mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
        } else {
            mDynamicToolbar.setVisible(false, VisibilityTransition.ANIMATE);
        }
    }

    public void refreshToolbarHeight() {
        ThreadUtils.assertOnUiThread();

        int height = 0;
        if (mBrowserChrome != null) {
            height = mBrowserChrome.getHeight();
        }

        if (!mDynamicToolbar.isEnabled() || isHomePagerVisible()) {
            
            
            
            if (mDynamicToolbar.isEnabled()) {
                
                
                
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
            mDynamicToolbar.setVisible(true, VisibilityTransition.IMMEDIATE);
        }
    }

    @Override
    void toggleChrome(final boolean aShow) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (aShow) {
                    mBrowserChrome.setVisibility(View.VISIBLE);
                } else {
                    mBrowserChrome.setVisibility(View.GONE);
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
                mBrowserChrome.setVisibility(View.VISIBLE);
                mActionBarFlipper.requestFocusFromTouch();
            }
        });
    }

    @Override
    public void refreshChrome() {
        invalidateOptionsMenu();

        if (mTabsPanel != null) {
            updateSideBarState();
            mTabsPanel.refresh();
        }

        mBrowserToolbar.refresh();
    }

    @Override
    public boolean hasTabsSideBar() {
        return (mTabsPanel != null && mTabsPanel.isSideBar());
    }

    private void setBrowserToolbarVisible(final boolean visible) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (mDynamicToolbar.isEnabled()) {
                    mDynamicToolbar.setVisible(visible, VisibilityTransition.IMMEDIATE);
                }
            }
        });
    }

    private void updateSideBarState() {
        if (NewTabletUI.isEnabled(this)) {
            return;
        }

        if (mMainLayoutAnimator != null)
            mMainLayoutAnimator.stop();

        boolean isSideBar = (HardwareUtils.isTablet() && getOrientation() == Configuration.ORIENTATION_LANDSCAPE);
        final int sidebarWidth = getResources().getDimensionPixelSize(R.dimen.tabs_sidebar_width);

        ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mTabsPanel.getLayoutParams();
        lp.width = (isSideBar ? sidebarWidth : ViewGroup.LayoutParams.MATCH_PARENT);
        mTabsPanel.requestLayout();

        final boolean sidebarIsShown = (isSideBar && mTabsPanel.isShown());
        final int mainLayoutScrollX = (sidebarIsShown ? -sidebarWidth : 0);
        mMainLayout.scrollTo(mainLayoutScrollX, 0);

        mTabsPanel.setIsSideBar(isSideBar);
    }

    @Override
    public void handleMessage(final String event, final NativeJSObject message,
                              final EventCallback callback) {
        if ("Accounts:Create".equals(event)) {
            
            final Intent intent = new Intent(getContext(), FxAccountGetStartedActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            final NativeJSObject extras = message.optObject("extras", null);
            if (extras != null) {
                intent.putExtra("extras", extras.toString());
            }
            getContext().startActivity(intent);

        } else if ("CharEncoding:Data".equals(event)) {
            final NativeJSObject[] charsets = message.getObjectArray("charsets");
            final int selected = message.getInt("selected");

            final String[] titleArray = new String[charsets.length];
            final String[] codeArray = new String[charsets.length];
            for (int i = 0; i < charsets.length; i++) {
                final NativeJSObject charset = charsets[i];
                titleArray[i] = charset.getString("title");
                codeArray[i] = charset.getString("code");
            }

            final AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);
            dialogBuilder.setSingleChoiceItems(titleArray, selected,
                    new AlertDialog.OnClickListener() {
                @Override
                public void onClick(final DialogInterface dialog, final int which) {
                    GeckoAppShell.sendEventToGecko(
                        GeckoEvent.createBroadcastEvent("CharEncoding:Set", codeArray[which]));
                    dialog.dismiss();
                }
            });
            dialogBuilder.setNegativeButton(R.string.button_cancel,
                    new AlertDialog.OnClickListener() {
                @Override
                public void onClick(final DialogInterface dialog, final int which) {
                    dialog.dismiss();
                }
            });
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    dialogBuilder.show();
                }
            });

        } else if ("CharEncoding:State".equals(event)) {
            final boolean visible = message.getString("visible").equals("true");
            GeckoPreferences.setCharEncodingState(visible);
            final Menu menu = mMenu;
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    if (menu != null) {
                        menu.findItem(R.id.char_encoding).setVisible(visible);
                    }
                }
            });

        } else if ("Feedback:LastUrl".equals(event)) {
            getLastUrl(callback);

        } else if ("Feedback:MaybeLater".equals(event)) {
            resetFeedbackLaunchCount();

        } else if ("Feedback:OpenPlayStore".equals(event)) {
            final Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse("market://details?id=" + getPackageName()));
            startActivity(intent);

        } else if ("Menu:Add".equals(event)) {
            final MenuItemInfo info = new MenuItemInfo();
            info.label = message.getString("name");
            info.id = message.getInt("id") + ADDON_MENU_OFFSET;
            info.icon = message.optString("icon", null);
            info.checked = message.optBoolean("checked", false);
            info.enabled = message.optBoolean("enabled", true);
            info.visible = message.optBoolean("visible", true);
            info.checkable = message.optBoolean("checkable", false);
            final int parent = message.optInt("parent", 0);
            info.parent = parent <= 0 ? parent : parent + ADDON_MENU_OFFSET;
            final MenuItemInfo menuItemInfo = info;
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    addAddonMenuItem(menuItemInfo);
                }
            });

        } else if ("Menu:Remove".equals(event)) {
            final int id = message.getInt("id") + ADDON_MENU_OFFSET;
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    removeAddonMenuItem(id);
                }
            });

        } else if ("Reader:Share".equals(event)) {
            final String title = message.getString("title");
            final String url = message.getString("url");
            GeckoAppShell.openUriExternal(url, "text/plain", "", "", Intent.ACTION_SEND, title);

        } else if ("Settings:Show".equals(event)) {
            final String resource =
                    message.optString(GeckoPreferences.INTENT_EXTRA_RESOURCES, null);
            final Intent settingsIntent = new Intent(this, GeckoPreferences.class);
            GeckoPreferences.setResourceToOpen(settingsIntent, resource);
            startActivityForResult(settingsIntent, ACTIVITY_REQUEST_PREFERENCES);

            
            
            if (HardwareUtils.IS_KINDLE_DEVICE) {
                overridePendingTransition(0, 0);
            }

        } else if ("Telemetry:Gather".equals(event)) {
            Telemetry.HistogramAdd("PLACES_PAGES_COUNT",
                    BrowserDB.getCount(getContentResolver(), "history"));
            Telemetry.HistogramAdd("PLACES_BOOKMARKS_COUNT",
                    BrowserDB.getCount(getContentResolver(), "bookmarks"));
            Telemetry.HistogramAdd("FENNEC_FAVICONS_COUNT",
                    BrowserDB.getCount(getContentResolver(), "favicons"));
            Telemetry.HistogramAdd("FENNEC_THUMBNAILS_COUNT",
                    BrowserDB.getCount(getContentResolver(), "thumbnails"));
            Telemetry.HistogramAdd("FENNEC_READING_LIST_COUNT",
                    BrowserDB.getCount(getContentResolver(), "readinglist"));
            Telemetry.HistogramAdd("BROWSER_IS_USER_DEFAULT", (isDefaultBrowser(Intent.ACTION_VIEW) ? 1 : 0));
            if (Versions.feature16Plus) {
                Telemetry.HistogramAdd("BROWSER_IS_ASSIST_DEFAULT", (isDefaultBrowser(Intent.ACTION_ASSIST) ? 1 : 0));
            }
        } else if ("Updater:Launch".equals(event)) {
            handleUpdaterLaunch();

        } else if ("BrowserToolbar:Visibility".equals(event)) {
            setBrowserToolbarVisible(message.getBoolean("visible"));

        } else {
            super.handleMessage(event, message, callback);
        }
    }

    




    private boolean isDefaultBrowser(String action) {
        final Intent viewIntent = new Intent(action, Uri.parse("http://www.mozilla.org"));
        final ResolveInfo info = getPackageManager().resolveActivity(viewIntent, PackageManager.MATCH_DEFAULT_ONLY);
        if (info == null) {
            
            return false;
        }

        final String packageName = info.activityInfo.packageName;
        return (TextUtils.equals(packageName, getPackageName()));
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Menu:Open")) {
                if (mBrowserToolbar.isEditing()) {
                    mBrowserToolbar.cancelEdit();
                }

                openOptionsMenu();
            } else if (event.equals("Menu:Update")) {
                final int id = message.getInt("id") + ADDON_MENU_OFFSET;
                final JSONObject options = message.getJSONObject("options");
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateAddonMenuItem(id, options);
                    }
                });
            } else if (event.equals("Gecko:DelayedStartup")) {
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        
                        
                        ensureTabsPanelExists();
                    }
                });

                if (AppConstants.MOZ_STUMBLER_BUILD_TIME_ENABLED) {
                    
                    
                    
                    final long oneSecondInMillis = 1000;
                    ThreadUtils.getBackgroundHandler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                             GeckoPreferences.broadcastStumblerPref(BrowserApp.this);
                        }
                    }, oneSecondInMillis);
                }
                super.handleMessage(event, message);
            } else if (event.equals("Gecko:Ready")) {
                
                
                super.handleMessage(event, message);
                final Menu menu = mMenu;
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (menu != null) {
                            menu.findItem(R.id.settings).setEnabled(true);
                            menu.findItem(R.id.help).setEnabled(true);
                        }
                    }
                });

                
                if (AppConstants.MOZ_DATA_REPORTING) {
                    DataReportingNotification.checkAndNotifyPolicy(GeckoAppShell.getContext());
                }

            } else if (event.equals("Search:Keyword")) {
                storeSearchQuery(message.getString("query"));
            } else if (event.equals("Prompt:ShowTop")) {
                
                Intent bringToFrontIntent = new Intent();
                bringToFrontIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME, AppConstants.BROWSER_INTENT_CLASS_NAME);
                bringToFrontIntent.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
                startActivity(bringToFrontIntent);
            } else if (event.equals("Accounts:Exist")) {
                final String kind = message.getString("kind");
                final JSONObject response = new JSONObject();

                if ("any".equals(kind)) {
                    response.put("exists", SyncAccounts.syncAccountsExist(getContext()) ||
                                           FirefoxAccounts.firefoxAccountsExist(getContext()));
                    EventDispatcher.sendResponse(message, response);
                } else if ("fxa".equals(kind)) {
                    response.put("exists", FirefoxAccounts.firefoxAccountsExist(getContext()));
                    EventDispatcher.sendResponse(message, response);
                } else if ("sync11".equals(kind)) {
                    response.put("exists", SyncAccounts.syncAccountsExist(getContext()));
                    EventDispatcher.sendResponse(message, response);
                } else {
                    response.put("error", "Unknown kind");
                    EventDispatcher.sendError(message, response);
                }
            } else {
                super.handleMessage(event, message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    @Override
    public void addTab() {
        Tabs.getInstance().addTab();
    }

    @Override
    public void addPrivateTab() {
        Tabs.getInstance().addPrivateTab();
    }

    @Override
    public void showNormalTabs() {
        showTabs(TabsPanel.Panel.NORMAL_TABS);
    }

    @Override
    public void showPrivateTabs() {
        showTabs(TabsPanel.Panel.PRIVATE_TABS);
    }
    



    private boolean ensureTabsPanelExists() {
        if (mTabsPanel != null) {
            return false;
        }

        ViewStub tabsPanelStub = (ViewStub) findViewById(R.id.tabs_panel);
        mTabsPanel = (TabsPanel) tabsPanelStub.inflate();

        mTabsPanel.setTabsLayoutChangeListener(this);
        updateSideBarState();

        return true;
    }

    private void showTabs(final TabsPanel.Panel panel) {
        if (Tabs.getInstance().getDisplayCount() == 0)
            return;

        if (ensureTabsPanelExists()) {
            
            
            
            ViewTreeObserver vto = mTabsPanel.getViewTreeObserver();
            if (vto.isAlive()) {
                vto.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        mTabsPanel.getViewTreeObserver().removeGlobalOnLayoutListener(this);
                        mTabsPanel.show(panel);
                    }
                });
            }
        } else {
            mTabsPanel.show(panel);
        }
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
        return (mTabsPanel != null && mTabsPanel.isShown());
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
            
            
            if (Versions.feature16Plus) {
                mLayerView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
        } else {
            if (Versions.feature16Plus) {
                mLayerView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
            }
        }

        mMainLayoutAnimator = new PropertyAnimator(animationLength, sTabsInterpolator);
        mMainLayoutAnimator.addPropertyAnimationListener(this);

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
        mBrowserToolbar.triggerTabsPanelTransition(mMainLayoutAnimator, areTabsShown());

        
        
        if (mDynamicToolbar.isEnabled()) {
            if (width > 0 && height > 0) {
                mDynamicToolbar.setPinned(true, PinReason.RELAYOUT);
                mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
            } else {
                mDynamicToolbar.setPinned(false, PinReason.RELAYOUT);
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
        } else {
            
            
            mBrowserToolbar.cancelEdit();
        }

        mTabsPanel.finishTabsAnimation();

        mMainLayoutAnimator = null;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        mDynamicToolbar.onSaveInstanceState(outState);
        outState.putInt(STATE_ABOUT_HOME_TOP_PADDING, mHomePagerContainer.getPaddingTop());
    }

    










    private boolean maybeSwitchToTab(String url, EnumSet<OnUrlOpenListener.Flags> flags) {
        if (!flags.contains(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB)) {
            return false;
        }

        final Tabs tabs = Tabs.getInstance();
        final Tab tab;

        if (AboutPages.isAboutReader(url)) {
            tab = tabs.getFirstReaderTabForUrl(url, tabs.getSelectedTab().isPrivate());
        } else {
            tab = tabs.getFirstTabForUrl(url, tabs.getSelectedTab().isPrivate());
        }

        if (tab == null) {
            return false;
        }

        return maybeSwitchToTab(tab.getId());
    }

    








    private boolean maybeSwitchToTab(int id) {
        final Tabs tabs = Tabs.getInstance();
        final Tab tab = tabs.getTab(id);

        if (tab == null) {
            return false;
        }

        final Tab oldTab = tabs.getSelectedTab();
        if (oldTab != null) {
            oldTab.setIsEditing(false);
        }

        
        
        mTargetTabForEditingMode = null;
        tabs.selectTab(tab.getId());

        mBrowserToolbar.cancelEdit();

        return true;
    }

    private void openUrlAndStopEditing(String url) {
        openUrlAndStopEditing(url, null, false);
    }

    private void openUrlAndStopEditing(String url, boolean newTab) {
        openUrlAndStopEditing(url, null, newTab);
    }

    private void openUrlAndStopEditing(String url, String searchEngine) {
        openUrlAndStopEditing(url, searchEngine, false);
    }

    private void openUrlAndStopEditing(String url, String searchEngine, boolean newTab) {
        int flags = Tabs.LOADURL_NONE;
        if (newTab) {
            flags |= Tabs.LOADURL_NEW_TAB;
            if (Tabs.getInstance().getSelectedTab().isPrivate()) {
                flags |= Tabs.LOADURL_PRIVATE;
            }
        }

        Tabs.getInstance().loadUrl(url, searchEngine, -1, flags);

        mBrowserToolbar.cancelEdit();
    }

    private boolean isHomePagerVisible() {
        return (mHomePager != null && mHomePager.isVisible()
            && mHomePagerContainer != null && mHomePagerContainer.getVisibility() == View.VISIBLE);
    }

    




    private void enterEditingMode() {
        String url = "";

        final Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null) {
            final String userRequested = tab.getUserRequested();

            
            
            url = (TextUtils.isEmpty(userRequested) ? tab.getURL() : userRequested);
        }

        enterEditingMode(url);
    }

    



    private void enterEditingMode(String url) {
        if (url == null) {
            url = "";
        }

        if (mBrowserToolbar.isEditing() || mBrowserToolbar.isAnimating()) {
            return;
        }

        final Tab selectedTab = Tabs.getInstance().getSelectedTab();
        mTargetTabForEditingMode = (selectedTab != null ? selectedTab.getId() : null);

        final PropertyAnimator animator = new PropertyAnimator(250);
        animator.setUseHardwareLayer(false);

        mBrowserToolbar.startEditing(url, animator);

        final String panelId = selectedTab.getMostRecentHomePanel();
        showHomePagerWithAnimator(panelId, animator);

        animator.start();
        Telemetry.startUISession(TelemetryContract.Session.AWESOMESCREEN);
    }

    private void commitEditingMode() {
        if (!mBrowserToolbar.isEditing()) {
            return;
        }

        Telemetry.stopUISession(TelemetryContract.Session.AWESOMESCREEN,
                                TelemetryContract.Reason.COMMIT);

        final String url = mBrowserToolbar.commitEdit();

        
        
        
        
        
        
        
        
        
        
        
        hideHomePager(url);
        loadUrlOrKeywordSearch(url);
    }

    private void loadUrlOrKeywordSearch(final String url) {
        
        if (TextUtils.isEmpty(url)) {
            return;
        }

        
        if (!StringUtils.isSearchQuery(url, true)) {
            Tabs.getInstance().loadUrl(url, Tabs.LOADURL_USER_ENTERED);
            Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.ACTIONBAR, "user");
            return;
        }

        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final String keyword;
                final String keywordSearch;

                final int index = url.indexOf(" ");
                if (index == -1) {
                    keyword = url;
                    keywordSearch = "";
                } else {
                    keyword = url.substring(0, index);
                    keywordSearch = url.substring(index + 1);
                }

                final String keywordUrl = BrowserDB.getUrlForKeyword(getContentResolver(), keyword);

                
                
                if (TextUtils.isEmpty(keywordUrl)) {
                    Tabs.getInstance().loadUrl(url, Tabs.LOADURL_USER_ENTERED);
                    Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.ACTIONBAR, "user");
                    return;
                }

                recordSearch(null, "barkeyword");

                
                final String searchUrl = keywordUrl.replace("%s", URLEncoder.encode(keywordSearch));
                Tabs.getInstance().loadUrl(searchUrl, Tabs.LOADURL_USER_ENTERED);
                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL,
                                      TelemetryContract.Method.ACTIONBAR,
                                      "keyword");
            }
        });
    }

    








    private static void recordSearch(SearchEngine engine, String where) {
        try {
            String identifier = (engine == null) ? "other" : engine.getEngineIdentifier();
            JSONObject message = new JSONObject();
            message.put("type", BrowserHealthRecorder.EVENT_SEARCH);
            message.put("location", where);
            message.put("identifier", identifier);
            EventDispatcher.getInstance().dispatchEvent(message, null);
        } catch (Exception e) {
            Log.e(LOGTAG, "Error recording search.", e);
        }
    }

    





    private void storeSearchQuery(String query) {
        if (TextUtils.isEmpty(query)) {
            return;
        }

        final ContentValues values = new ContentValues();
        values.put(SearchHistory.QUERY, query);
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                getContentResolver().insert(SearchHistory.CONTENT_URI, values);
            }
        });
    }

    void filterEditingMode(String searchTerm, AutocompleteHandler handler) {
        if (TextUtils.isEmpty(searchTerm)) {
            hideBrowserSearch();
        } else {
            showBrowserSearch();
            mBrowserSearch.filter(searchTerm, handler);
        }
    }

    











    private void selectTargetTabForEditingMode() {
        if (NewTabletUI.isEnabled(this)) {
            return;
        }

        if (mTargetTabForEditingMode != null) {
            Tabs.getInstance().selectTab(mTargetTabForEditingMode);
        }

        mTargetTabForEditingMode = null;
    }

    


    private void updateHomePagerForTab(Tab tab) {
        
        if (mBrowserToolbar.isEditing()) {
            return;
        }

        if (isAboutHome(tab)) {
            String panelId = AboutPages.getPanelIdFromAboutHomeUrl(tab.getURL());
            if (panelId == null) {
                
                
                panelId = tab.getMostRecentHomePanel();
            }
            showHomePager(panelId);

            if (mDynamicToolbar.isEnabled()) {
                mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
            }
        } else {
            hideHomePager();
        }
    }

    @Override
    public void onLocaleReady(final String locale) {
        Log.d(LOGTAG, "onLocaleReady: " + locale);
        super.onLocaleReady(locale);

        HomePanelsManager.getInstance().onLocaleReady(locale);

        if (mMenu != null) {
            mMenu.clear();
            onCreateOptionsMenu(mMenu);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(LOGTAG, "onActivityResult: " + requestCode + ", " + resultCode + ", " + data);
        switch (requestCode) {
            case ACTIVITY_REQUEST_PREFERENCES:
                
                
                
                if (resultCode != GeckoPreferences.RESULT_CODE_LOCALE_DID_CHANGE) {
                    Log.d(LOGTAG, "No locale change returning from preferences; nothing to do.");
                    return;
                }

                ThreadUtils.postToBackgroundThread(new Runnable() {
                    @Override
                    public void run() {
                        final LocaleManager localeManager = BrowserLocaleManager.getInstance();
                        final Locale locale = localeManager.getCurrentLocale(getApplicationContext());
                        Log.d(LOGTAG, "Read persisted locale " + locale);
                        if (locale == null) {
                            return;
                        }
                        onLocaleChanged(BrowserLocaleManager.getLanguageTag(locale));
                    }
                });
                break;
            default:
                super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private void showHomePager(String panelId) {
        showHomePagerWithAnimator(panelId, null);
    }

    private void showHomePagerWithAnimator(String panelId, PropertyAnimator animator) {
        if (isHomePagerVisible()) {
            
            mHomePager.showPanel(panelId);
            return;
        }

        
        refreshToolbarHeight();

        
        
        if (mDynamicToolbar.isEnabled()) {
            mDynamicToolbar.setVisible(true, VisibilityTransition.IMMEDIATE);
        }

        if (mHomePager == null) {
            final ViewStub homePagerStub = (ViewStub) findViewById(R.id.home_pager_stub);
            mHomePager = (HomePager) homePagerStub.inflate();

            mHomePager.setOnPanelChangeListener(new HomePager.OnPanelChangeListener() {
                @Override
                public void onPanelSelected(String panelId) {
                    final Tab currentTab = Tabs.getInstance().getSelectedTab();
                    if (currentTab != null) {
                        currentTab.setMostRecentHomePanel(panelId);
                    }
                }
            });

            
            if (!getProfile().inGuestMode()) {
                final ViewStub homeBannerStub = (ViewStub) findViewById(R.id.home_banner_stub);
                final HomeBanner homeBanner = (HomeBanner) homeBannerStub.inflate();
                mHomePager.setBanner(homeBanner);

                
                homeBanner.setOnDismissListener(new HomeBanner.OnDismissListener() {
                    @Override
                    public void onDismiss() {
                        mHomePager.setBanner(null);
                        mHomePagerContainer.removeView(homeBanner);
                    }
                });
            }
        }

        mHomePagerContainer.setVisibility(View.VISIBLE);
        mHomePager.load(getSupportLoaderManager(),
                        getSupportFragmentManager(),
                        panelId, animator);

        
        hideWebContentOnPropertyAnimationEnd(animator);
    }

    private void hideWebContentOnPropertyAnimationEnd(final PropertyAnimator animator) {
        if (animator == null) {
            hideWebContent();
            return;
        }

        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                mHideWebContentOnAnimationEnd = true;
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (mHideWebContentOnAnimationEnd) {
                    hideWebContent();
                }
            }
        });
    }

    private void hideWebContent() {
        
        
        mLayerView.setVisibility(View.INVISIBLE);
    }

    



    private void hideHomePager() {
        final Tab selectedTab = Tabs.getInstance().getSelectedTab();
        final String url = (selectedTab != null) ? selectedTab.getURL() : null;

        hideHomePager(url);
    }

    



    private void hideHomePager(final String url) {
        if (!isHomePagerVisible() || AboutPages.isAboutHome(url)) {
            return;
        }

        
        mHideWebContentOnAnimationEnd = false;

        
        mLayerView.setVisibility(View.VISIBLE);
        mHomePagerContainer.setVisibility(View.GONE);

        if (mHomePager != null) {
            mHomePager.unload();
        }

        mBrowserToolbar.setNextFocusDownId(R.id.layer_view);

        
        refreshToolbarHeight();
    }

    private void showBrowserSearch() {
        if (mBrowserSearch.getUserVisibleHint()) {
            return;
        }

        mBrowserSearchContainer.setVisibility(View.VISIBLE);

        
        mHomePager.setVisibility(View.INVISIBLE);

        final FragmentManager fm = getSupportFragmentManager();

        
        
        
        
        
        fm.executePendingTransactions();

        fm.beginTransaction().add(R.id.search_container, mBrowserSearch, BROWSER_SEARCH_TAG).commitAllowingStateLoss();
        mBrowserSearch.setUserVisibleHint(true);
    }

    private void hideBrowserSearch() {
        if (!mBrowserSearch.getUserVisibleHint()) {
            return;
        }

        
        
        mHomePager.setVisibility(View.VISIBLE);

        mBrowserSearchContainer.setVisibility(View.INVISIBLE);

        getSupportFragmentManager().beginTransaction()
                .remove(mBrowserSearch).commitAllowingStateLoss();
        mBrowserSearch.setUserVisibleHint(false);
    }

    



    private class HideOnTouchListener implements TouchEventInterceptor {
        private boolean mIsHidingTabs;
        private final Rect mTempRect = new Rect();

        @Override
        public boolean onInterceptTouchEvent(View view, MotionEvent event) {
            
            if (mToast != null) {
                mToast.hide(false, ButtonToast.ReasonHidden.TOUCH_OUTSIDE);
            }

            
            
            
            if (view.getScrollX() != 0 || view.getScrollY() != 0) {
                view.getHitRect(mTempRect);
                mTempRect.offset(-view.getScrollX(), -view.getScrollY());

                if (mTintManager != null) {
                    SystemBarTintManager.SystemBarConfig config = mTintManager.getConfig();
                    mTempRect.offset(0, -config.getPixelInsetTop(false));
                }

                int[] viewCoords = new int[2];
                view.getLocationOnScreen(viewCoords);

                int x = (int) event.getRawX() - viewCoords[0];
                int y = (int) event.getRawY() - viewCoords[1];

                if (!mTempRect.contains(x, y))
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

    private static Menu findParentMenu(Menu menu, MenuItem item) {
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

    



    private void addAddonMenuItemToMenu(final Menu menu, final MenuItemInfo info) {
        info.added = true;

        final Menu destination;
        if (info.parent == 0) {
            destination = menu;
        } else if (info.parent == GECKO_TOOLS_MENU) {
            
            if (Versions.feature11Plus) {
                MenuItem tools = menu.findItem(R.id.tools);
                destination = tools != null ? tools.getSubMenu() : menu;
            } else {
                destination = menu;
            }
        } else {
            MenuItem parent = menu.findItem(info.parent);
            if (parent == null) {
                return;
            }

            Menu parentMenu = findParentMenu(menu, parent);

            if (!parent.hasSubMenu()) {
                parentMenu.removeItem(parent.getItemId());
                destination = parentMenu.addSubMenu(Menu.NONE, parent.getItemId(), Menu.NONE, parent.getTitle());
                if (parent.getIcon() != null) {
                    ((SubMenu) destination).getItem().setIcon(parent.getIcon());
                }
            } else {
                destination = parent.getSubMenu();
            }
        }

        MenuItem item = destination.add(Menu.NONE, info.id, Menu.NONE, info.label);

        item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Menu:Clicked", Integer.toString(info.id - ADDON_MENU_OFFSET)));
                return true;
            }
        });

        if (info.icon == null) {
            item.setIcon(R.drawable.ic_menu_addons_filler);
        } else {
            final int id = info.id;
            BitmapUtils.getDrawable(this, info.icon, new BitmapUtils.BitmapLoader() {
                @Override
                public void onBitmapFound(Drawable d) {
                    
                    MenuItem item = destination.findItem(id);
                    if (item == null) {
                        return;
                    }
                    if (d == null) {
                        item.setIcon(R.drawable.ic_menu_addons_filler);
                        return;
                    }
                    item.setIcon(d);
                }
            });
        }

        item.setCheckable(info.checkable);
        item.setChecked(info.checked);
        item.setEnabled(info.enabled);
        item.setVisible(info.visible);
    }

    private void addAddonMenuItem(final MenuItemInfo info) {
        if (mAddonMenuItemsCache == null) {
            mAddonMenuItemsCache = new Vector<MenuItemInfo>();
        }

        
        info.added = (mMenu != null);

        
        mAddonMenuItemsCache.add(info);

        if (mMenu == null) {
            return;
        }

        addAddonMenuItemToMenu(mMenu, info);
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
                    item.label = options.optString("name", item.label);
                    item.checkable = options.optBoolean("checkable", item.checkable);
                    item.checked = options.optBoolean("checked", item.checked);
                    item.enabled = options.optBoolean("enabled", item.enabled);
                    item.visible = options.optBoolean("visible", item.visible);
                    item.added = (mMenu != null);
                    break;
                }
            }
        }

        if (mMenu == null) {
            return;
        }

        MenuItem menuItem = mMenu.findItem(id);
        if (menuItem != null) {
            menuItem.setTitle(options.optString("name", menuItem.getTitle().toString()));
            menuItem.setCheckable(options.optBoolean("checkable", menuItem.isCheckable()));
            menuItem.setChecked(options.optBoolean("checked", menuItem.isChecked()));
            menuItem.setEnabled(options.optBoolean("enabled", menuItem.isEnabled()));
            menuItem.setVisible(options.optBoolean("visible", menuItem.isVisible()));
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        
        super.onCreateOptionsMenu(menu);

        
        if (menu instanceof GeckoMenu &&
            HardwareUtils.isTablet()) {
            ((GeckoMenu) menu).setActionItemBarPresenter(mBrowserToolbar);
        }

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.browser_app_menu, mMenu);

        
        if (mAddonMenuItemsCache != null && !mAddonMenuItemsCache.isEmpty()) {
            for (MenuItemInfo item : mAddonMenuItemsCache) {
                addAddonMenuItemToMenu(mMenu, item);
            }
        }

        
        if (Versions.feature14Plus) {
            GeckoMenuItem share = (GeckoMenuItem) mMenu.findItem(R.id.share);
            GeckoActionProvider provider = GeckoActionProvider.getForType(GeckoActionProvider.DEFAULT_MIME_TYPE, this);
            share.setActionProvider(provider);
        }

        return true;
    }

    @Override
    public void openOptionsMenu() {
        
        
        if (mBrowserToolbar.isEditing() && !NewTabletUI.isEnabled(this)) {
            return;
        }

        if (areTabsShown()) {
            mTabsPanel.showMenu();
            return;
        }

        
        if (mMenuPanel != null)
            mMenuPanel.scrollTo(0, 0);

        if (!mBrowserToolbar.openOptionsMenu())
            super.openOptionsMenu();

        if (mDynamicToolbar.isEnabled()) {
            mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
        }
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
                    mBrowserChrome.setVisibility(View.GONE);
                    if (mDynamicToolbar.isEnabled()) {
                        mDynamicToolbar.setVisible(false, VisibilityTransition.IMMEDIATE);
                        mLayerView.getLayerMarginsAnimator().setMaxMargins(0, 0, 0, 0);
                    } else {
                        setToolbarMargin(0);
                    }
                } else {
                    mBrowserChrome.setVisibility(View.VISIBLE);
                    if (mDynamicToolbar.isEnabled()) {
                        mDynamicToolbar.setVisible(true, VisibilityTransition.IMMEDIATE);
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

        
        TabHistoryFragment frag = (TabHistoryFragment) getSupportFragmentManager().findFragmentByTag(TAB_HISTORY_FRAGMENT_TAG);
        if (frag != null) {
            frag.dismiss();
        }

        if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            aMenu.findItem(R.id.settings).setEnabled(false);
            aMenu.findItem(R.id.help).setEnabled(false);
        }

        Tab tab = Tabs.getInstance().getSelectedTab();
        MenuItem bookmark = aMenu.findItem(R.id.bookmark);
        MenuItem back = aMenu.findItem(R.id.back);
        MenuItem forward = aMenu.findItem(R.id.forward);
        MenuItem share = aMenu.findItem(R.id.share);
        MenuItem saveAsPDF = aMenu.findItem(R.id.save_as_pdf);
        MenuItem charEncoding = aMenu.findItem(R.id.char_encoding);
        MenuItem findInPage = aMenu.findItem(R.id.find_in_page);
        MenuItem desktopMode = aMenu.findItem(R.id.desktop_mode);
        MenuItem enterGuestMode = aMenu.findItem(R.id.new_guest_session);
        MenuItem exitGuestMode = aMenu.findItem(R.id.exit_guest_session);

        
        
        
        
        final boolean visible = Versions.preICS ||
                                HardwareUtils.isTelevision() ||
                                !PrefUtils.getStringSet(GeckoSharedPrefs.forProfile(this),
                                                        ClearOnShutdownPref.PREF,
                                                        new HashSet<String>()).isEmpty();
        aMenu.findItem(R.id.quit).setVisible(visible);

        if (tab == null || tab.getURL() == null) {
            bookmark.setEnabled(false);
            back.setEnabled(false);
            forward.setEnabled(false);
            share.setEnabled(false);
            saveAsPDF.setEnabled(false);
            findInPage.setEnabled(false);

            
            
            if (Versions.feature11Plus) {
                
                MenuUtils.safeSetEnabled(aMenu, R.id.page, false);
            }
            MenuUtils.safeSetEnabled(aMenu, R.id.subscribe, false);
            MenuUtils.safeSetEnabled(aMenu, R.id.add_search_engine, false);
            MenuUtils.safeSetEnabled(aMenu, R.id.site_settings, false);
            MenuUtils.safeSetEnabled(aMenu, R.id.add_to_launcher, false);

            return true;
        }

        bookmark.setEnabled(!AboutPages.isAboutReader(tab.getURL()));
        bookmark.setVisible(!GeckoProfile.get(this).inGuestMode());
        bookmark.setCheckable(true);
        bookmark.setChecked(tab.isBookmark());
        bookmark.setIcon(resolveBookmarkIconID(tab.isBookmark()));

        back.setEnabled(tab.canDoBack());
        forward.setEnabled(tab.canDoForward());
        desktopMode.setChecked(tab.getDesktopMode());
        desktopMode.setIcon(tab.getDesktopMode() ? R.drawable.ic_menu_desktop_mode_on : R.drawable.ic_menu_desktop_mode_off);

        String url = tab.getURL();
        if (AboutPages.isAboutReader(url)) {
            String urlFromReader = ReaderModeUtils.getUrlFromAboutReader(url);
            if (urlFromReader != null) {
                url = urlFromReader;
            }
        }

        
        final boolean shareEnabled = RestrictedProfiles.isAllowed(this, RestrictedProfiles.Restriction.DISALLOW_SHARE);
        share.setVisible(shareEnabled);
        share.setEnabled(StringUtils.isShareableUrl(url) && shareEnabled);
        MenuUtils.safeSetEnabled(aMenu, R.id.apps, RestrictedProfiles.isAllowed(this, RestrictedProfiles.Restriction.DISALLOW_INSTALL_APPS));
        MenuUtils.safeSetEnabled(aMenu, R.id.addons, RestrictedProfiles.isAllowed(this, RestrictedProfiles.Restriction.DISALLOW_INSTALL_EXTENSION));
        MenuUtils.safeSetEnabled(aMenu, R.id.downloads, RestrictedProfiles.isAllowed(this, RestrictedProfiles.Restriction.DISALLOW_DOWNLOADS));

        
        
        if (Versions.feature11Plus) {
            MenuUtils.safeSetEnabled(aMenu, R.id.page, !isAboutHome(tab));
        }
        MenuUtils.safeSetEnabled(aMenu, R.id.subscribe, tab.hasFeeds());
        MenuUtils.safeSetEnabled(aMenu, R.id.add_search_engine, tab.hasOpenSearch());
        MenuUtils.safeSetEnabled(aMenu, R.id.site_settings, !isAboutHome(tab));
        MenuUtils.safeSetEnabled(aMenu, R.id.add_to_launcher, !isAboutHome(tab));

        
        if (Versions.feature14Plus) {
            final GeckoActionProvider provider = ((GeckoMenuItem) share).getGeckoActionProvider();
            if (provider != null) {
                Intent shareIntent = provider.getIntent();

                
                if (shareIntent == null) {
                    shareIntent = new Intent(Intent.ACTION_SEND);
                    shareIntent.setType("text/plain");
                    provider.setIntent(shareIntent);
                }

                
                shareIntent.putExtra(Intent.EXTRA_TEXT, url);
                shareIntent.putExtra(Intent.EXTRA_SUBJECT, tab.getDisplayTitle());
                shareIntent.putExtra(Intent.EXTRA_TITLE, tab.getDisplayTitle());

                
                shareIntent.removeExtra("share_screenshot_uri");

                
                BitmapDrawable drawable = tab.getThumbnail();
                if (drawable != null) {
                    Bitmap thumbnail = drawable.getBitmap();

                    
                    if (Build.MANUFACTURER.equals("Kobo") && thumbnail != null) {
                        File cacheDir = getExternalCacheDir();

                        if (cacheDir != null) {
                            File outFile = new File(cacheDir, "thumbnail.png");

                            try {
                                java.io.FileOutputStream out = new java.io.FileOutputStream(outFile);
                                thumbnail.compress(Bitmap.CompressFormat.PNG, 90, out);
                            } catch (FileNotFoundException e) {
                                Log.e(LOGTAG, "File not found", e);
                            }

                            shareIntent.putExtra("share_screenshot_uri", Uri.parse(outFile.getPath()));
                        }
                    }
                }
            }
        }

        
        saveAsPDF.setEnabled(!(isAboutHome(tab) ||
                               tab.getContentType().equals("application/vnd.mozilla.xul+xml") ||
                               tab.getContentType().startsWith("video/")));

        
        findInPage.setEnabled(!isAboutHome(tab));

        charEncoding.setVisible(GeckoPreferences.getCharEncodingState());

        if (mProfile.inGuestMode()) {
            exitGuestMode.setVisible(true);
        } else {
            enterGuestMode.setVisible(true);
        }

        return true;
    }

    private int resolveBookmarkIconID(final boolean isBookmark) {
        if (NewTabletUI.isEnabled(this) && HardwareUtils.isLargeTablet()) {
            if (isBookmark) {
                return R.drawable.new_tablet_ic_menu_bookmark_remove;
            } else {
                return R.drawable.new_tablet_ic_menu_bookmark_add;
            }
        }

        if (isBookmark) {
            return R.drawable.ic_menu_bookmark_remove;
        } else {
            return R.drawable.ic_menu_bookmark_add;
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Tab tab = null;
        Intent intent = null;

        final int itemId = item.getItemId();

        
        
        Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.MENU, getResources().getResourceEntryName(itemId));

        if (NewTabletUI.isEnabled(this)) {
            mBrowserToolbar.cancelEdit();
        }

        if (itemId == R.id.bookmark) {
            tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                if (item.isChecked()) {
                    Telemetry.sendUIEvent(TelemetryContract.Event.UNSAVE, TelemetryContract.Method.MENU, "bookmark");
                    tab.removeBookmark();
                    item.setIcon(resolveBookmarkIconID(false));
                } else {
                    Telemetry.sendUIEvent(TelemetryContract.Event.SAVE, TelemetryContract.Method.MENU, "bookmark");
                    tab.addBookmark();
                    item.setIcon(resolveBookmarkIconID(true));
                }
            }
            return true;
        }

        if (itemId == R.id.share) {
            shareCurrentUrl();
            return true;
        }

        if (itemId == R.id.reload) {
            tab = Tabs.getInstance().getSelectedTab();
            if (tab != null)
                tab.doReload();
            return true;
        }

        if (itemId == R.id.back) {
            tab = Tabs.getInstance().getSelectedTab();
            if (tab != null)
                tab.doBack();
            return true;
        }

        if (itemId == R.id.forward) {
            tab = Tabs.getInstance().getSelectedTab();
            if (tab != null)
                tab.doForward();
            return true;
        }

        if (itemId == R.id.save_as_pdf) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SaveAs:PDF", null));
            return true;
        }

        if (itemId == R.id.settings) {
            intent = new Intent(this, GeckoPreferences.class);

            
            
            startActivityForResult(intent, ACTIVITY_REQUEST_PREFERENCES);
            return true;
        }

        if (itemId == R.id.help) {
            final String VERSION = AppConstants.MOZ_APP_VERSION;
            final String OS = AppConstants.OS_TARGET;
            final String LOCALE = BrowserLocaleManager.getLanguageTag(Locale.getDefault());

            final String URL = getResources().getString(R.string.help_link, VERSION, OS, LOCALE);
            Tabs.getInstance().loadUrlInTab(URL);
            return true;
        }

        if (itemId == R.id.addons) {
            Tabs.getInstance().loadUrlInTab(AboutPages.ADDONS);
            return true;
        }

        if (itemId == R.id.apps) {
            Tabs.getInstance().loadUrlInTab(AboutPages.APPS);
            return true;
        }

        if (itemId == R.id.downloads) {
            Tabs.getInstance().loadUrlInTab(AboutPages.DOWNLOADS);
            return true;
        }

        if (itemId == R.id.char_encoding) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("CharEncoding:Get", null));
            return true;
        }

        if (itemId == R.id.find_in_page) {
            mFindInPageBar.show();
            return true;
        }

        if (itemId == R.id.desktop_mode) {
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab == null)
                return true;
            JSONObject args = new JSONObject();
            try {
                args.put("desktopMode", !item.isChecked());
                args.put("tabId", selectedTab.getId());
            } catch (JSONException e) {
                Log.e(LOGTAG, "error building json arguments", e);
            }
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("DesktopMode:Change", args.toString()));
            return true;
        }

        if (itemId == R.id.new_tab) {
            addTab();
            return true;
        }

        if (itemId == R.id.new_private_tab) {
            addPrivateTab();
            return true;
        }

        if (itemId == R.id.new_guest_session) {
            showGuestModeDialog(GuestModeDialog.ENTERING);
            return true;
        }

        if (itemId == R.id.exit_guest_session) {
            showGuestModeDialog(GuestModeDialog.LEAVING);
            return true;
        }

        
        
        
        if (onContextItemSelected(item)) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public void showGuestModeDialog(final GuestModeDialog type) {
        final Prompt ps = new Prompt(this, new Prompt.PromptCallback() {
            @Override
            public void onPromptFinished(String result) {
                try {
                    int itemId = new JSONObject(result).getInt("button");
                    if (itemId == 0) {
                        String args = "";
                        if (type == GuestModeDialog.ENTERING) {
                            args = GUEST_BROWSING_ARG;
                        } else {
                            GeckoProfile.leaveGuestSession(BrowserApp.this);

                            
                            GuestSession.hideNotification(BrowserApp.this);
                        }

                        if (!GuestSession.isSecureKeyguardLocked(BrowserApp.this)) {
                            doRestart(args);
                        } else {
                            
                            
                            GeckoProfile.maybeCleanupGuestProfile(BrowserApp.this);
                        }
                        GeckoAppShell.systemExit();
                    }
                } catch(JSONException ex) {
                    Log.e(LOGTAG, "Exception reading guest mode prompt result", ex);
                }
            }
        });

        Resources res = getResources();
        ps.setButtons(new String[] {
            res.getString(R.string.guest_session_dialog_continue),
            res.getString(R.string.guest_session_dialog_cancel)
        });

        int titleString = 0;
        int msgString = 0;
        if (type == GuestModeDialog.ENTERING) {
            titleString = R.string.new_guest_session_title;
            msgString = R.string.new_guest_session_text;
        } else {
            titleString = R.string.exit_guest_session_title;
            msgString = R.string.exit_guest_session_text;
        }

        ps.show(res.getString(titleString), res.getString(msgString), null, ListView.CHOICE_MODE_NONE);
    }

    


    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            
            TabHistoryFragment frag = (TabHistoryFragment) getSupportFragmentManager().findFragmentByTag(TAB_HISTORY_FRAGMENT_TAG);
            if (frag != null) {
                return false;
            }

            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null  && !tab.isEditing()) {
                return tabHistoryController.showTabHistory(tab, TabHistoryController.HistoryAction.ALL);
            }
        }
        return super.onKeyLongPress(keyCode, event);
    }

    



    @Override
    protected void onNewIntent(Intent intent) {
        String action = intent.getAction();

        final boolean isViewAction = Intent.ACTION_VIEW.equals(action);
        final boolean isBookmarkAction = GeckoApp.ACTION_HOMESCREEN_SHORTCUT.equals(action);

        if (mInitialized && (isViewAction || isBookmarkAction)) {
            
            mBrowserToolbar.cancelEdit();

            
            
            final TelemetryContract.Method method =
                (isViewAction ? TelemetryContract.Method.INTENT : TelemetryContract.Method.HOMESCREEN);

            Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, method);
        }

        super.onNewIntent(intent);

        if (AppConstants.MOZ_ANDROID_BEAM && NfcAdapter.ACTION_NDEF_DISCOVERED.equals(action)) {
            String uri = intent.getDataString();
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(uri));
        }

        
        if (GuestSession.NOTIFICATION_INTENT.equals(action)) {
            GuestSession.handleIntent(this, intent);
        }

        if (!mInitialized || !Intent.ACTION_MAIN.equals(action)) {
            return;
        }

        
        final String keyName = getPackageName() + ".feedback_launch_count";
        final StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();

        
        try {
            SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
            int launchCount = settings.getInt(keyName, 0);
            if (launchCount < FEEDBACK_LAUNCH_COUNT) {
                
                launchCount++;
                settings.edit().putInt(keyName, launchCount).apply();

                
                if (launchCount == FEEDBACK_LAUNCH_COUNT) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Feedback:Show", null));
                }
            }
        } finally {
            StrictMode.setThreadPolicy(savedPolicy);
        }
    }

    @Override
    protected NotificationClient makeNotificationClient() {
        
        
        return new ServiceNotificationClient(getApplicationContext());
    }

    private void resetFeedbackLaunchCount() {
        SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
        settings.edit().putInt(getPackageName() + ".feedback_launch_count", 0).apply();
    }

    private void getLastUrl(final EventCallback callback) {
        (new UIAsyncTask.WithoutParams<String>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public synchronized String doInBackground() {
                
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
                callback.sendSuccess(url);
            }
        }).execute();
    }

    
    @Override
    public void onNewTabs(List<String> urls) {
        final EnumSet<OnUrlOpenListener.Flags> flags = EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB);

        for (String url : urls) {
            if (!maybeSwitchToTab(url, flags)) {
                openUrlAndStopEditing(url, true);
            }
        }
    }

    
    @Override
    public void onUrlOpen(String url, EnumSet<OnUrlOpenListener.Flags> flags) {
        if (flags.contains(OnUrlOpenListener.Flags.OPEN_WITH_INTENT)) {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse(url));
            startActivity(intent);
        } else if (!maybeSwitchToTab(url, flags)) {
            openUrlAndStopEditing(url);
        }
    }

    
    @Override
    public void onUrlOpenInBackground(final String url, EnumSet<OnUrlOpenInBackgroundListener.Flags> flags) {
        if (url == null) {
            throw new IllegalArgumentException("url must not be null");
        }
        if (flags == null) {
            throw new IllegalArgumentException("flags must not be null");
        }

        final boolean isPrivate = flags.contains(OnUrlOpenInBackgroundListener.Flags.PRIVATE);

        int loadFlags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_BACKGROUND;
        if (isPrivate) {
            loadFlags |= Tabs.LOADURL_PRIVATE;
        }

        final Tab newTab = Tabs.getInstance().loadUrl(url, loadFlags);

        
        
        
        
        final int newTabId = newTab.getId();

        final ToastListener listener = new ButtonToast.ToastListener() {
            @Override
            public void onButtonClicked() {
                maybeSwitchToTab(newTabId);
            }

            @Override
            public void onToastHidden(ButtonToast.ReasonHidden reason) { }
        };

        final String message = isPrivate ?
                getResources().getString(R.string.new_private_tab_opened) :
                getResources().getString(R.string.new_tab_opened);
        final String buttonMessage = getResources().getString(R.string.switch_button_message);
        getButtonToast().show(false,
                              message,
                              ButtonToast.LENGTH_SHORT,
                              buttonMessage,
                              R.drawable.switch_button_icon,
                              listener);
    }

    
    @Override
    public void onSearch(SearchEngine engine, String text) {
        
        
        
        if (!Tabs.getInstance().getSelectedTab().isPrivate()) {
            storeSearchQuery(text);
        }
        recordSearch(engine, "barsuggest");
        openUrlAndStopEditing(text, engine.name);
    }

    
    @Override
    public void onEditSuggestion(String suggestion) {
        mBrowserToolbar.onEditSuggestion(suggestion);
    }

    @Override
    public int getLayout() { return R.layout.gecko_app; }

    @Override
    protected String getDefaultProfileName() throws NoMozillaDirectoryException {
        return GeckoProfile.getDefaultProfileName(this);
    }

    










    protected boolean handleUpdaterLaunch() {
        if (AppConstants.RELEASE_BUILD) {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse("market://details?id=" + getPackageName()));
            startActivity(intent);
            return true;
        }

        if (AppConstants.MOZ_UPDATER) {
            Tabs.getInstance().loadUrlInTab(AboutPages.UPDATER);
            return true;
        }

        Log.w(LOGTAG, "No candidate updater found; ignoring launch request.");
        return false;
    }

    
    @Override
    public void startActionModeCompat(final ActionModeCompat.Callback callback) {
        
        if (mActionMode == null) {
            mActionBarFlipper.showNext();
            LayerMarginsAnimator margins = mLayerView.getLayerMarginsAnimator();

            
            if (mDynamicToolbar.isEnabled() && !margins.areMarginsShown()) {
                margins.setMaxMargins(0, mBrowserChrome.getHeight(), 0, 0);
                mDynamicToolbar.setVisible(true, VisibilityTransition.ANIMATE);
                mShowActionModeEndAnimation = true;
            } else {
                
                mActionBar.animateIn();
            }

            mDynamicToolbar.setPinned(true, PinReason.ACTION_MODE);
        } else {
            
            mActionMode.finish();
        }

        mActionMode = new ActionModeCompat(BrowserApp.this, callback, mActionBar);
        if (callback.onCreateActionMode(mActionMode, mActionMode.getMenu())) {
            mActionMode.invalidate();
        }
    }

    
    @Override
    public void endActionModeCompat() {
        if (mActionMode == null) {
            return;
        }

        mActionMode.finish();
        mActionMode = null;
        mDynamicToolbar.setPinned(false, PinReason.ACTION_MODE);

        mActionBarFlipper.showPrevious();

        
        
        if (mShowActionModeEndAnimation) {
            mDynamicToolbar.setVisible(false, VisibilityTransition.IMMEDIATE);
            mShowActionModeEndAnimation = false;
        }
    }

    @Override
    protected HealthRecorder createHealthRecorder(final Context context,
                                                  final String profilePath,
                                                  final EventDispatcher dispatcher,
                                                  final String osLocale,
                                                  final String appLocale,
                                                  final SessionInformation previousSession) {
        return new BrowserHealthRecorder(context,
                                         GeckoSharedPrefs.forApp(context),
                                         profilePath,
                                         dispatcher,
                                         osLocale,
                                         appLocale,
                                         previousSession);
    }

    @Override
    public TilesRecorder getTilesRecorder() {
        return mTilesRecorder;
    }
}
