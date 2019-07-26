




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.GeckoApplication;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.LightweightTheme;
import org.mozilla.gecko.R;
import org.mozilla.gecko.SiteIdentity;
import org.mozilla.gecko.SiteIdentity.SecurityMode;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.PropertyAnimationListener;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.MenuPopup;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.widget.GeckoImageButton;
import org.mozilla.gecko.widget.GeckoImageView;
import org.mozilla.gecko.widget.GeckoRelativeLayout;
import org.mozilla.gecko.widget.GeckoTextView;

import org.json.JSONObject;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.os.SystemClock;
import android.text.style.ForegroundColorSpan;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.AlphaAnimation;
import android.view.animation.Interpolator;
import android.view.animation.TranslateAnimation;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import java.util.Arrays;
import java.util.List;

public class BrowserToolbar extends GeckoRelativeLayout
                            implements Tabs.OnTabsChangedListener,
                                       GeckoMenu.ActionItemBarPresenter,
                                       Animation.AnimationListener,
                                       GeckoEventListener {
    private static final String LOGTAG = "GeckoToolbar";
    public static final String PREF_TITLEBAR_MODE = "browser.chrome.titlebarMode";
    public static final String PREF_TRIM_URLS = "browser.urlbar.trimURLs";

    public interface OnActivateListener {
        public void onActivate();
    }

    public interface OnCommitListener {
        public void onCommit();
    }

    public interface OnDismissListener {
        public void onDismiss();
    }

    public interface OnFilterListener {
        public void onFilter(String searchText, AutocompleteHandler handler);
    }

    public interface OnStartEditingListener {
        public void onStartEditing();
    }

    public interface OnStopEditingListener {
        public void onStopEditing();
    }

    private enum ForwardButtonAnimation {
        SHOW,
        HIDE
    }

    private View mUrlDisplayContainer;
    private ToolbarEditLayout mUrlEditLayout;
    private View mUrlBarEntry;
    private ImageView mUrlBarRightEdge;
    private GeckoTextView mTitle;
    private int mTitlePadding;
    private boolean mSiteSecurityVisible;
    private boolean mSwitchingTabs;
    private ShapedButton mTabs;
    private ImageButton mBack;
    private ImageButton mForward;
    private ImageButton mStop;

    
    private Bitmap mLastFavicon;
    private ImageButton mFavicon;

    private ImageButton mSiteSecurity;
    private PageActionLayout mPageActionLayout;
    private Animation mProgressSpinner;
    private TabCounter mTabsCounter;
    private GeckoImageButton mMenu;
    private GeckoImageView mMenuIcon;
    private LinearLayout mActionItemBar;
    private MenuPopup mMenuPopup;
    private List<? extends View> mFocusOrder;

    private OnActivateListener mActivateListener;
    private OnCommitListener mCommitListener;
    private OnDismissListener mDismissListener;
    private OnFilterListener mFilterListener;
    private OnStartEditingListener mStartEditingListener;
    private OnStopEditingListener mStopEditingListener;

    private SiteIdentityPopup mSiteIdentityPopup;

    final private BrowserApp mActivity;
    private boolean mHasSoftMenuButton;

    private boolean mShowSiteSecurity;
    private boolean mSpinnerVisible;

    private boolean mIsEditing;
    private boolean mAnimatingEntry;

    private AlphaAnimation mLockFadeIn;
    private TranslateAnimation mTitleSlideLeft;
    private TranslateAnimation mTitleSlideRight;

    private int mUrlBarViewOffset;
    private int mDefaultForwardMargin;
    private PropertyAnimator mForwardAnim = null;

    private int mFaviconSize;

    private PropertyAnimator mVisibilityAnimator;
    private static final Interpolator sButtonsInterpolator = new AccelerateInterpolator();

    private static final int TABS_CONTRACTED = 1;
    private static final int TABS_EXPANDED = 2;

    private static final int FORWARD_ANIMATION_DURATION = 450;
    private final ForegroundColorSpan mUrlColor;
    private final ForegroundColorSpan mBlockedColor;
    private final ForegroundColorSpan mDomainColor;
    private final ForegroundColorSpan mPrivateDomainColor;

    private final LightweightTheme mTheme;

    private boolean mShowUrl;
    private boolean mTrimURLs;

    private Integer mPrefObserverId;

    public BrowserToolbar(Context context) {
        this(context, null);
    }

    public BrowserToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);
        mTheme = ((GeckoApplication) context.getApplicationContext()).getLightweightTheme();

        
        mActivity = (BrowserApp) context;

        
        LayoutInflater.from(context).inflate(R.layout.browser_toolbar, this);

        Tabs.registerOnTabsChangedListener(this);
        mSwitchingTabs = true;

        mAnimatingEntry = false;
        mShowUrl = false;
        mTrimURLs = true;

        final String[] prefs = {
            PREF_TITLEBAR_MODE,
            PREF_TRIM_URLS
        };
        
        mPrefObserverId = PrefsHelper.getPrefs(prefs, new PrefsHelper.PrefHandlerBase() {
            @Override
            public void prefValue(String pref, String str) {
                
                int value = Integer.parseInt(str);
                boolean shouldShowUrl = (value == 1);

                if (shouldShowUrl == mShowUrl) {
                    return;
                }
                mShowUrl = shouldShowUrl;

                triggerTitleUpdate();
            }

            @Override
            public void prefValue(String pref, boolean value) {
                
                if (value == mTrimURLs) {
                    return;
                }
                mTrimURLs = value;

                triggerTitleUpdate();
            }

            @Override
            public boolean isObserver() {
                
                
                return true;
            }

            private void triggerTitleUpdate() {
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateTitle();
                    }
                });
            }
        });

        Resources res = getResources();
        mUrlColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_urltext));
        mBlockedColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_blockedtext));
        mDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext));
        mPrivateDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext_private));

        registerEventListener("Reader:Click");
        registerEventListener("Reader:LongClick");

        mShowSiteSecurity = false;

        mAnimatingEntry = false;

        mUrlBarViewOffset = res.getDimensionPixelSize(R.dimen.url_bar_offset_left);
        mDefaultForwardMargin = res.getDimensionPixelSize(R.dimen.forward_default_offset);
        mUrlDisplayContainer = findViewById(R.id.url_display_container);
        mUrlBarEntry = findViewById(R.id.url_bar_entry);
        mUrlEditLayout = (ToolbarEditLayout) findViewById(R.id.edit_layout);

        
        mUrlBarRightEdge = (ImageView) findViewById(R.id.url_bar_right_edge);
        if (mUrlBarRightEdge != null) {
            mUrlBarRightEdge.getDrawable().setLevel(6000);
        }

        mTitle = (GeckoTextView) findViewById(R.id.url_bar_title);
        mTitlePadding = mTitle.getPaddingRight();

        mTabs = (ShapedButton) findViewById(R.id.tabs);
        mTabsCounter = (TabCounter) findViewById(R.id.tabs_counter);

        mBack = (ImageButton) findViewById(R.id.back);
        setButtonEnabled(mBack, false);
        mForward = (ImageButton) findViewById(R.id.forward);
        setButtonEnabled(mForward, false);

        mFavicon = (ImageButton) findViewById(R.id.favicon);
        if (Build.VERSION.SDK_INT >= 11) {
            if (Build.VERSION.SDK_INT >= 16) {
                mFavicon.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
            mFavicon.setLayerType(View.LAYER_TYPE_HARDWARE, null);
        }
        mFaviconSize = Math.round(res.getDimension(R.dimen.browser_toolbar_favicon_size));

        mSiteSecurity = (ImageButton) findViewById(R.id.site_security);
        mSiteSecurityVisible = (mSiteSecurity.getVisibility() == View.VISIBLE);

        mSiteIdentityPopup = new SiteIdentityPopup(mActivity);
        mSiteIdentityPopup.setAnchor(mSiteSecurity);

        mProgressSpinner = AnimationUtils.loadAnimation(mActivity, R.anim.progress_spinner);

        mStop = (ImageButton) findViewById(R.id.stop);
        mPageActionLayout = (PageActionLayout) findViewById(R.id.page_action_layout);

        mMenu = (GeckoImageButton) findViewById(R.id.menu);
        mMenuIcon = (GeckoImageView) findViewById(R.id.menu_icon);
        mActionItemBar = (LinearLayout) findViewById(R.id.menu_items);
        mHasSoftMenuButton = !HardwareUtils.hasMenuButton();

        
        
        if (HardwareUtils.isTablet()) {
            mFocusOrder = Arrays.asList(mTabs, mBack, mForward, this,
                    mSiteSecurity, mPageActionLayout, mStop, mActionItemBar, mMenu);
        } else {
            mFocusOrder = Arrays.asList(this, mSiteSecurity, mPageActionLayout, mStop,
                    mTabs, mMenu);
        }

        setIsEditing(false);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mActivateListener != null) {
                    mActivateListener.onActivate();
                }
            }
        });

        setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                
                if (isEditing()) {
                    return;
                }

                MenuInflater inflater = mActivity.getMenuInflater();
                inflater.inflate(R.menu.titlebar_contextmenu, menu);

                String clipboard = Clipboard.getText();
                if (TextUtils.isEmpty(clipboard)) {
                    menu.findItem(R.id.pasteandgo).setVisible(false);
                    menu.findItem(R.id.paste).setVisible(false);
                }

                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    String url = tab.getURL();
                    if (url == null) {
                        menu.findItem(R.id.copyurl).setVisible(false);
                        menu.findItem(R.id.share).setVisible(false);
                        menu.findItem(R.id.add_to_launcher).setVisible(false);
                    }

                    if (!tab.hasFeeds()) {
                        menu.findItem(R.id.subscribe).setVisible(false);
                    }

                    menu.findItem(R.id.add_search_engine).setVisible(tab.hasOpenSearch());
                } else {
                    
                    menu.findItem(R.id.copyurl).setVisible(false);
                    menu.findItem(R.id.share).setVisible(false);
                    menu.findItem(R.id.add_to_launcher).setVisible(false);
                    menu.findItem(R.id.subscribe).setVisible(false);
                    menu.findItem(R.id.add_search_engine).setVisible(false);
                }

                menu.findItem(R.id.share).setVisible(!GeckoProfile.get(getContext()).inGuestMode());
            }
        });

        mUrlEditLayout.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                setSelected(hasFocus);
            }
        });

        mTabs.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleTabs();
            }
        });
        mTabs.setImageLevel(0);

        mBack.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                Tabs.getInstance().getSelectedTab().doBack();
            }
        });
        mBack.setOnLongClickListener(new Button.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                return Tabs.getInstance().getSelectedTab().showBackHistory();
            }
        });

        mForward.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                Tabs.getInstance().getSelectedTab().doForward();
            }
        });
        mForward.setOnLongClickListener(new Button.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                return Tabs.getInstance().getSelectedTab().showForwardHistory();
            }
        });

        Button.OnClickListener faviconListener = new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mSiteSecurity.getVisibility() != View.VISIBLE)
                    return;

                final Tab tab = Tabs.getInstance().getSelectedTab();

                final SiteIdentity siteIdentity = tab.getSiteIdentity();
                if (siteIdentity.getSecurityMode() == SecurityMode.UNKNOWN) {
                    Log.e(LOGTAG, "Selected tab has no identity data");
                    return;
                }

                mSiteIdentityPopup.updateIdentity(siteIdentity);
                mSiteIdentityPopup.show();
            }
        };

        mFavicon.setOnClickListener(faviconListener);
        mSiteSecurity.setOnClickListener(faviconListener);

        mStop.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null)
                    tab.doStop();
                setProgressVisibility(false);
            }
        });

        float slideWidth = getResources().getDimension(R.dimen.browser_toolbar_lock_width);

        LinearLayout.LayoutParams siteSecParams = (LinearLayout.LayoutParams) mSiteSecurity.getLayoutParams();
        final float scale = getResources().getDisplayMetrics().density;
        slideWidth += (siteSecParams.leftMargin + siteSecParams.rightMargin) * scale + 0.5f;

        mLockFadeIn = new AlphaAnimation(0.0f, 1.0f);
        mLockFadeIn.setAnimationListener(this);

        mTitleSlideLeft = new TranslateAnimation(slideWidth, 0, 0, 0);
        mTitleSlideLeft.setAnimationListener(this);

        mTitleSlideRight = new TranslateAnimation(-slideWidth, 0, 0, 0);
        mTitleSlideRight.setAnimationListener(this);

        final int lockAnimDuration = 300;
        mLockFadeIn.setDuration(lockAnimDuration);
        mTitleSlideLeft.setDuration(lockAnimDuration);
        mTitleSlideRight.setDuration(lockAnimDuration);

        if (mHasSoftMenuButton) {
            mMenu.setVisibility(View.VISIBLE);
            mMenuIcon.setVisibility(View.VISIBLE);

            mMenu.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mActivity.openOptionsMenu();
                }
            });
        }
    }

    public void refresh() {
        dismissSiteIdentityPopup();
    }

    public boolean onBackPressed() {
        return dismissSiteIdentityPopup();
    }

    public boolean onKey(int keyCode, KeyEvent event) {
        if (event.getAction() != KeyEvent.ACTION_DOWN) {
            return false;
        }

        
        
        if (keyCode > KeyEvent.getMaxKeyCode()) {
            return true;
        }

        
        
        if (keyCode == KeyEvent.KEYCODE_BACK ||
            keyCode == KeyEvent.KEYCODE_MENU ||
            keyCode == KeyEvent.KEYCODE_DPAD_UP ||
            keyCode == KeyEvent.KEYCODE_DPAD_DOWN ||
            keyCode == KeyEvent.KEYCODE_DPAD_LEFT ||
            keyCode == KeyEvent.KEYCODE_DPAD_RIGHT ||
            keyCode == KeyEvent.KEYCODE_DPAD_CENTER ||
            keyCode == KeyEvent.KEYCODE_DEL ||
            keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
            keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            return false;
        } else if (isEditing()) {
            return mUrlEditLayout.onKey(keyCode, event);
        }

        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        
        
        if (event != null && event.getY() > getHeight() + ViewHelper.getTranslationY(this)) {
            return false;
        }

        return super.onTouchEvent(event);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        if (h != oldh) {
            
            
            post(new Runnable() {
                @Override
                public void run() {
                    mActivity.refreshToolbarHeight();
                }
            });
        }
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        Log.d(LOGTAG, "onTabChanged: " + msg);
        final Tabs tabs = Tabs.getInstance();

        
        
        
        

        switch (msg) {
            case ADDED:
            case CLOSED:
                updateTabCount(tabs.getDisplayCount());
                break;
            case RESTORED:
                
            case SELECTED:
                dismissSiteIdentityPopup();
                updateTabCount(tabs.getDisplayCount());
                mSwitchingTabs = true;
                
        }

        if (tabs.isSelectedTab(tab)) {
            switch (msg) {
                case TITLE:
                    updateTitle();
                    break;

                case START:
                    updateBackButton(tab);
                    updateForwardButton(tab);
                    if (tab.getState() == Tab.STATE_LOADING) {
                        setProgressVisibility(true);
                    }
                    setSecurityMode(tab.getSecurityMode());
                    setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
                    break;

                case STOP:
                    updateBackButton(tab);
                    updateForwardButton(tab);
                    setProgressVisibility(false);
                    
                    updateTitle();
                    break;

                case SELECTED:
                case LOAD_ERROR:
                    updateTitle();
                    
                case LOCATION_CHANGE:
                    
                    
                    refreshState();
                    break;

                case CLOSED:
                case ADDED:
                    updateBackButton(tab);
                    updateForwardButton(tab);
                    break;

                case FAVICON:
                    setFavicon(tab.getFavicon());
                    break;

                case SECURITY_CHANGE:
                    setSecurityMode(tab.getSecurityMode());
                    break;

                case READER_ENABLED:
                    setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
                    break;
            }
        }

        switch (msg) {
            case SELECTED:
            case LOAD_ERROR:
            case LOCATION_CHANGE:
                mSwitchingTabs = false;
        }
    }

    public boolean isVisible() {
        return ViewHelper.getTranslationY(this) == 0;
    }

    public void setNextFocusDownId(int nextId) {
        super.setNextFocusDownId(nextId);
        mTabs.setNextFocusDownId(nextId);
        mBack.setNextFocusDownId(nextId);
        mForward.setNextFocusDownId(nextId);
        mFavicon.setNextFocusDownId(nextId);
        mStop.setNextFocusDownId(nextId);
        mSiteSecurity.setNextFocusDownId(nextId);
        mPageActionLayout.setNextFocusDownId(nextId);
        mMenu.setNextFocusDownId(nextId);
    }

    @Override
    public void onAnimationStart(Animation animation) {
        if (animation.equals(mLockFadeIn)) {
            if (mSiteSecurityVisible)
                mSiteSecurity.setVisibility(View.VISIBLE);
        } else if (animation.equals(mTitleSlideLeft)) {
            
            
            
            mSiteSecurity.setVisibility(View.GONE);
        } else if (animation.equals(mTitleSlideRight)) {
            
            
            mSiteSecurity.setVisibility(View.INVISIBLE);
        }
    }

    @Override
    public void onAnimationRepeat(Animation animation) {
    }

    @Override
    public void onAnimationEnd(Animation animation) {
        if (animation.equals(mTitleSlideRight)) {
            mSiteSecurity.startAnimation(mLockFadeIn);
        }
    }

    private boolean dismissSiteIdentityPopup() {
        if (mSiteIdentityPopup != null && mSiteIdentityPopup.isShowing()) {
            mSiteIdentityPopup.dismiss();
            return true;
        }

        return false;
    }

    private int getUrlBarEntryTranslation() {
        return getWidth() - mUrlBarEntry.getRight();
    }

    private int getUrlBarCurveTranslation() {
        return getWidth() - mTabs.getLeft();
    }

    private boolean canDoBack(Tab tab) {
        return (tab.canDoBack() && !mIsEditing);
    }

    private boolean canDoForward(Tab tab) {
        return (tab.canDoForward() && !mIsEditing);
    }

    private void addTab() {
        mActivity.addTab();
    }

    private void toggleTabs() {
        if (mActivity.areTabsShown()) {
            if (mActivity.hasTabsSideBar())
                mActivity.hideTabs();
        } else {
            
            InputMethodManager imm =
                    (InputMethodManager) mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(mTabs.getWindowToken(), 0);

            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                if (!tab.isPrivate())
                    mActivity.showNormalTabs();
                else
                    mActivity.showPrivateTabs();
            }
        }
    }

    public void updateTabCountAndAnimate(int count) {
        
        if (!isVisible()) {
            updateTabCount(count);
            return;
        }

        
        
        
        
        if (!isEditing() || HardwareUtils.isTablet()) {
            mTabsCounter.setCount(count);

            mTabs.setContentDescription((count > 1) ?
                                        mActivity.getString(R.string.num_tabs, count) :
                                        mActivity.getString(R.string.one_tab));
        }
    }

    public void updateTabCount(int count) {
        
        
        
        
        if (isEditing() && !HardwareUtils.isTablet()) {
            return;
        }

        
        if (isVisible() && ViewHelper.getAlpha(mTabsCounter) != 0) {
            mTabsCounter.setCountWithAnimation(count);
        } else {
            mTabsCounter.setCount(count);
        }

        
        mTabs.setContentDescription((count > 1) ?
                                    mActivity.getString(R.string.num_tabs, count) :
                                    mActivity.getString(R.string.one_tab));
    }

    public void setProgressVisibility(boolean visible) {
        Log.d(LOGTAG, "setProgressVisibility: " + visible);
        
        
        
        if (visible) {
            mFavicon.setImageResource(R.drawable.progress_spinner);
            mLastFavicon = null;

            
            if (!mSpinnerVisible) {
                setPageActionVisibility(true);
                mFavicon.setAnimation(mProgressSpinner);
                mProgressSpinner.start();
                mSpinnerVisible = true;
            }
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber start");
        } else {
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null) {
                setFavicon(selectedTab.getFavicon());
            }

            if (mSpinnerVisible) {
                setPageActionVisibility(false);
                mFavicon.setAnimation(null);
                mProgressSpinner.cancel();
                mSpinnerVisible = false;
            }
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber stop");
        }
    }

    public void setPageActionVisibility(boolean isLoading) {
        
        mStop.setVisibility(isLoading ? View.VISIBLE : View.GONE);

        
        setSiteSecurityVisibility(mShowSiteSecurity && !isLoading);
        mPageActionLayout.setVisibility(!isLoading ? View.VISIBLE : View.GONE);

        
        
        
        mTitle.setPadding(0, 0, (!isLoading ? mTitlePadding : 0), 0);
        updateFocusOrder();
    }

    private void setSiteSecurityVisibility(final boolean visible) {
        if (visible == mSiteSecurityVisible)
            return;

        mSiteSecurityVisible = visible;

        if (mSwitchingTabs) {
            mSiteSecurity.setVisibility(visible ? View.VISIBLE : View.GONE);
            return;
        }

        mTitle.clearAnimation();
        mSiteSecurity.clearAnimation();

        
        
        mLockFadeIn.reset();
        mTitleSlideLeft.reset();
        mTitleSlideRight.reset();

        if (mForwardAnim != null) {
            long delay = mForwardAnim.getRemainingTime();
            mTitleSlideRight.setStartOffset(delay);
            mTitleSlideLeft.setStartOffset(delay);
        } else {
            mTitleSlideRight.setStartOffset(0);
            mTitleSlideLeft.setStartOffset(0);
        }

        mTitle.startAnimation(visible ? mTitleSlideRight : mTitleSlideLeft);
    }

    private void updateFocusOrder() {
        View prevView = null;

        
        
        boolean needsNewFocus = false;

        for (View view : mFocusOrder) {
            if (view.getVisibility() != View.VISIBLE || !view.isEnabled()) {
                if (view.hasFocus()) {
                    needsNewFocus = true;
                }
                continue;
            }

            if (view == mActionItemBar) {
                final int childCount = mActionItemBar.getChildCount();
                for (int child = 0; child < childCount; child++) {
                    View childView = mActionItemBar.getChildAt(child);
                    if (prevView != null) {
                        childView.setNextFocusLeftId(prevView.getId());
                        prevView.setNextFocusRightId(childView.getId());
                    }
                    prevView = childView;
                }
            } else {
                if (prevView != null) {
                    view.setNextFocusLeftId(prevView.getId());
                    prevView.setNextFocusRightId(view.getId());
                }
                prevView = view;
            }
        }

        if (needsNewFocus) {
            requestFocus();
        }
    }

    public void onEditSuggestion(String suggestion) {
        if (!isEditing()) {
            return;
        }

        mUrlEditLayout.onEditSuggestion(suggestion);
    }

    public void setTitle(CharSequence title) {
        mTitle.setText(title);
        setContentDescription(title != null ? title : mTitle.getHint());
    }

    
    private void updateTitle() {
        final Tab tab = Tabs.getInstance().getSelectedTab();
        
        if (tab == null || tab.isEnteringReaderMode()) {
            return;
        }

        final String url = tab.getURL();

        if (!isEditing()) {
            mUrlEditLayout.setText(url);
        }

        
        if (AboutPages.isTitlelessAboutPage(url)) {
            setTitle(null);
            return;
        }

        
        if (tab.getErrorType() == Tab.ErrorType.BLOCKED) {
            String title = tab.getDisplayTitle();
            SpannableStringBuilder builder = new SpannableStringBuilder(title);
            builder.setSpan(mBlockedColor, 0, title.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
            setTitle(builder);
            return;
        }

        
        if (!mShowUrl || url == null) {
            setTitle(tab.getDisplayTitle());
            return;
        }

        CharSequence title = url;
        if (mTrimURLs) {
            title = StringUtils.stripCommonSubdomains(StringUtils.stripScheme(url));
        }

        String baseDomain = tab.getBaseDomain();
        if (!TextUtils.isEmpty(baseDomain)) {
            SpannableStringBuilder builder = new SpannableStringBuilder(title);
            int index = title.toString().indexOf(baseDomain);
            if (index > -1) {
                builder.setSpan(mUrlColor, 0, title.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
                builder.setSpan(tab.isPrivate() ? mPrivateDomainColor : mDomainColor, index, index+baseDomain.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
                title = builder;
            }
        }

        setTitle(title);
    }

    private void setFavicon(Bitmap image) {
        Log.d(LOGTAG, "setFavicon(" + image + ")");
        if (Tabs.getInstance().getSelectedTab().getState() == Tab.STATE_LOADING) {
            return;
        }

        if (image == mLastFavicon) {
            Log.d(LOGTAG, "Ignoring favicon set: new favicon is identical to previous favicon.");
            return;
        }

        mLastFavicon = image;     

        if (image != null) {
            image = Bitmap.createScaledBitmap(image, mFaviconSize, mFaviconSize, false);
            mFavicon.setImageBitmap(image);
        } else {
            mFavicon.setImageDrawable(null);
        }
    }
    
    private void setSecurityMode(SecurityMode mode) {
        mSiteSecurity.setImageLevel(mode.ordinal());
        mShowSiteSecurity = (mode != SecurityMode.UNKNOWN);

        setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
    }

    public void prepareTabsAnimation(PropertyAnimator animator, boolean tabsAreShown) {
        if (!tabsAreShown) {
            PropertyAnimator buttonsAnimator =
                    new PropertyAnimator(animator.getDuration(), sButtonsInterpolator);

            buttonsAnimator.attach(mTabsCounter,
                                   PropertyAnimator.Property.ALPHA,
                                   1.0f);

            if (mHasSoftMenuButton && !HardwareUtils.isTablet()) {
                buttonsAnimator.attach(mMenuIcon,
                                       PropertyAnimator.Property.ALPHA,
                                       1.0f);
            }

            buttonsAnimator.start();

            return;
        }

        ViewHelper.setAlpha(mTabsCounter, 0.0f);

        if (mHasSoftMenuButton && !HardwareUtils.isTablet()) {
            ViewHelper.setAlpha(mMenuIcon, 0.0f);
        }
    }

    public void finishTabsAnimation(boolean tabsAreShown) {
        if (tabsAreShown) {
            return;
        }

        PropertyAnimator animator = new PropertyAnimator(150);

        animator.attach(mTabsCounter,
                        PropertyAnimator.Property.ALPHA,
                        1.0f);

        if (mHasSoftMenuButton && !HardwareUtils.isTablet()) {
            animator.attach(mMenuIcon,
                            PropertyAnimator.Property.ALPHA,
                            1.0f);
        }

        animator.start();
    }

    public void setOnActivateListener(OnActivateListener listener) {
        mActivateListener = listener;
    }

    public void setOnCommitListener(OnCommitListener listener) {
        mCommitListener = listener;
        mUrlEditLayout.setOnCommitListener(listener);
    }

    public void setOnDismissListener(OnDismissListener listener) {
        mDismissListener = listener;
        mUrlEditLayout.setOnDismissListener(listener);
    }

    public void setOnFilterListener(OnFilterListener listener) {
        mFilterListener = listener;
        mUrlEditLayout.setOnFilterListener(listener);
    }

    public void setOnStartEditingListener(OnStartEditingListener listener) {
        mStartEditingListener = listener;
    }

    public void setOnStopEditingListener(OnStopEditingListener listener) {
        mStopEditingListener = listener;
    }

    private void showUrlEditLayout() {
        setUrlEditLayoutVisibility(true, null);
    }

    private void showUrlEditLayout(PropertyAnimator animator) {
        setUrlEditLayoutVisibility(true, animator);
    }

    private void hideUrlEditLayout() {
        setUrlEditLayoutVisibility(false, null);
    }

    private void hideUrlEditLayout(PropertyAnimator animator) {
        setUrlEditLayoutVisibility(false, animator);
    }

    private void setUrlEditLayoutVisibility(final boolean showEditLayout, PropertyAnimator animator) {
        final View viewToShow = (showEditLayout ? mUrlEditLayout : mUrlDisplayContainer);
        final View viewToHide = (showEditLayout ? mUrlDisplayContainer : mUrlEditLayout);

        if (showEditLayout) {
            mUrlEditLayout.prepareShowAnimation(animator);
        }

        if (animator == null) {
            viewToHide.setVisibility(View.GONE);
            viewToShow.setVisibility(View.VISIBLE);
            return;
        }

        ViewHelper.setAlpha(viewToShow, 0.0f);
        animator.attach(viewToShow,
                        PropertyAnimator.Property.ALPHA,
                        1.0f);

        animator.attach(viewToHide,
                        PropertyAnimator.Property.ALPHA,
                        0.0f);

        animator.addPropertyAnimationListener(new PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                viewToShow.setVisibility(View.VISIBLE);
            }

            @Override
            public void onPropertyAnimationEnd() {
                viewToHide.setVisibility(View.GONE);
                ViewHelper.setAlpha(viewToHide, 1.0f);
            }
        });
    }

    



    private void updateChildrenForEditing() {
        
        if (!HardwareUtils.isTablet()) {
            return;
        }

        
        final boolean enabled = !mIsEditing;

        
        
        final float alpha = (enabled ? 1.0f : 0.24f);

        mTabs.setEnabled(enabled);
        ViewHelper.setAlpha(mTabsCounter, alpha);
        mMenu.setEnabled(enabled);
        ViewHelper.setAlpha(mMenuIcon, alpha);

        final int actionItemsCount = mActionItemBar.getChildCount();
        for (int i = 0; i < actionItemsCount; i++) {
            mActionItemBar.getChildAt(i).setEnabled(enabled);
        }
        ViewHelper.setAlpha(mActionItemBar, alpha);

        final Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null) {
            setButtonEnabled(mBack, canDoBack(tab));
            setButtonEnabled(mForward, canDoForward(tab));

            
            
            
            
            if (!mIsEditing) {
                animateForwardButton(canDoForward(tab) ?
                                     ForwardButtonAnimation.SHOW : ForwardButtonAnimation.HIDE);
            }
        }
    }

    public void setIsEditing(boolean isEditing) {
        mIsEditing = isEditing;
        mUrlEditLayout.setEnabled(isEditing);
    }

    



    public boolean isEditing() {
        return mIsEditing;
    }

    public void startEditing(String url, PropertyAnimator animator) {
        if (isEditing()) {
            return;
        }

        mUrlEditLayout.setText(url != null ? url : "");

        setIsEditing(true);
        updateChildrenForEditing();

        if (mStartEditingListener != null) {
            mStartEditingListener.onStartEditing();
        }

        if (mUrlBarRightEdge != null) {
            mUrlBarRightEdge.setVisibility(View.VISIBLE);
        }

        final int entryTranslation = getUrlBarEntryTranslation();
        final int curveTranslation = getUrlBarCurveTranslation();

        
        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            showUrlEditLayout();

            if (!HardwareUtils.isTablet()) {
                if (mUrlBarRightEdge != null) {
                    ViewHelper.setTranslationX(mUrlBarRightEdge, entryTranslation);
                }

                ViewHelper.setTranslationX(mTabs, curveTranslation);
                ViewHelper.setTranslationX(mTabsCounter, curveTranslation);
                ViewHelper.setTranslationX(mActionItemBar, curveTranslation);

                if (mHasSoftMenuButton) {
                    ViewHelper.setTranslationX(mMenu, curveTranslation);
                    ViewHelper.setTranslationX(mMenuIcon, curveTranslation);
                }
            }

            return;
        }

        if (mAnimatingEntry)
            return;

        
        setSelected(true);

        
        ViewHelper.setAlpha(mPageActionLayout, 0);
        ViewHelper.setAlpha(mStop, 0);

        

        if (mUrlBarRightEdge != null) {
            animator.attach(mUrlBarRightEdge,
                            PropertyAnimator.Property.TRANSLATION_X,
                            entryTranslation);
        }

        animator.attach(mTabs,
                        PropertyAnimator.Property.TRANSLATION_X,
                        curveTranslation);
        animator.attach(mTabsCounter,
                        PropertyAnimator.Property.TRANSLATION_X,
                        curveTranslation);
        animator.attach(mActionItemBar,
                        PropertyAnimator.Property.TRANSLATION_X,
                        curveTranslation);

        if (mHasSoftMenuButton) {
            animator.attach(mMenu,
                            PropertyAnimator.Property.TRANSLATION_X,
                            curveTranslation);

            animator.attach(mMenuIcon,
                            PropertyAnimator.Property.TRANSLATION_X,
                            curveTranslation);
        }

        showUrlEditLayout(animator);

        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                mAnimatingEntry = false;
            }
        });

        mAnimatingEntry = true;
    }

    




    public String cancelEdit() {
        return stopEditing();
    }

    




    public String commitEdit() {
        final String url = stopEditing();
        if (!TextUtils.isEmpty(url)) {
            setTitle(url);
        }
        return url;
    }

    private String stopEditing() {
        final String url = mUrlEditLayout.getText();
        if (!isEditing()) {
            return url;
        }
        setIsEditing(false);

        updateChildrenForEditing();

        if (mStopEditingListener != null) {
            mStopEditingListener.onStopEditing();
        }

        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            hideUrlEditLayout();

            if (!HardwareUtils.isTablet()) {
                updateTabCountAndAnimate(Tabs.getInstance().getDisplayCount());

                if (mUrlBarRightEdge != null) {
                    ViewHelper.setTranslationX(mUrlBarRightEdge, 0);
                }

                ViewHelper.setTranslationX(mTabs, 0);
                ViewHelper.setTranslationX(mTabsCounter, 0);
                ViewHelper.setTranslationX(mActionItemBar, 0);

                if (mHasSoftMenuButton) {
                    ViewHelper.setTranslationX(mMenu, 0);
                    ViewHelper.setTranslationX(mMenuIcon, 0);
                }
            }

            return url;
        }

        final PropertyAnimator contentAnimator = new PropertyAnimator(250);
        contentAnimator.setUseHardwareLayer(false);

        

        if (mUrlBarRightEdge != null) {
            contentAnimator.attach(mUrlBarRightEdge,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   0);
        }

        contentAnimator.attach(mTabs,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mTabsCounter,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mActionItemBar,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);

        if (mHasSoftMenuButton) {
            contentAnimator.attach(mMenu,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   0);

            contentAnimator.attach(mMenuIcon,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   0);
        }

        hideUrlEditLayout(contentAnimator);

        contentAnimator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (mUrlBarRightEdge != null) {
                    mUrlBarRightEdge.setVisibility(View.INVISIBLE);
                }

                PropertyAnimator buttonsAnimator = new PropertyAnimator(300);

                
                
                buttonsAnimator.attach(mPageActionLayout,
                                       PropertyAnimator.Property.ALPHA,
                                       1);
                buttonsAnimator.attach(mStop,
                                       PropertyAnimator.Property.ALPHA,
                                       1);

                buttonsAnimator.start();

                mAnimatingEntry = false;

                
                
                updateTabCountAndAnimate(Tabs.getInstance().getDisplayCount());
            }
        });

        mAnimatingEntry = true;
        contentAnimator.start();

        return url;
    }

    public void setButtonEnabled(ImageButton button, boolean enabled) {
        final Drawable drawable = button.getDrawable();
        if (drawable != null) {
            
            
            drawable.setAlpha(enabled ? 255 : 61);
        }

        button.setEnabled(enabled);
    }

    public void updateBackButton(Tab tab) {
        setButtonEnabled(mBack, canDoBack(tab));
    }

    private void animateForwardButton(final ForwardButtonAnimation animation) {
        
        
        if (mForward.getVisibility() != View.VISIBLE) {
            return;
        }

        final boolean showing = (animation == ForwardButtonAnimation.SHOW);

        
        
        MarginLayoutParams fwdParams = (MarginLayoutParams) mForward.getLayoutParams();
        if ((fwdParams.leftMargin > mDefaultForwardMargin && showing) ||
            (fwdParams.leftMargin == mDefaultForwardMargin && !showing)) {
            return;
        }

        
        mForwardAnim = new PropertyAnimator(mSwitchingTabs ? 10 : FORWARD_ANIMATION_DURATION);
        final int width = mForward.getWidth() / 2;

        mForwardAnim.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                if (!showing) {
                    
                    
                    MarginLayoutParams layoutParams =
                        (MarginLayoutParams) mUrlDisplayContainer.getLayoutParams();
                    layoutParams.leftMargin = 0;

                    
                    layoutParams = (MarginLayoutParams) mUrlEditLayout.getLayoutParams();
                    layoutParams.leftMargin = 0;

                    requestLayout();
                    
                    
                    
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (showing) {
                    MarginLayoutParams layoutParams =
                        (MarginLayoutParams) mUrlDisplayContainer.getLayoutParams();
                    layoutParams.leftMargin = mUrlBarViewOffset;

                    layoutParams = (MarginLayoutParams) mUrlEditLayout.getLayoutParams();
                    layoutParams.leftMargin = mUrlBarViewOffset;

                    ViewHelper.setTranslationX(mTitle, 0);
                    ViewHelper.setTranslationX(mFavicon, 0);
                    ViewHelper.setTranslationX(mSiteSecurity, 0);
                }

                MarginLayoutParams layoutParams = (MarginLayoutParams) mForward.getLayoutParams();
                layoutParams.leftMargin = mDefaultForwardMargin + (showing ? width : 0);
                ViewHelper.setTranslationX(mForward, 0);

                requestLayout();
                mForwardAnim = null;
            }
        });

        prepareForwardAnimation(mForwardAnim, animation, width);
        mForwardAnim.start();
    }

    public void updateForwardButton(Tab tab) {
        final boolean enabled = canDoForward(tab);
        if (mForward.isEnabled() == enabled)
            return;

        
        
        setButtonEnabled(mForward, enabled);
        animateForwardButton(enabled ? ForwardButtonAnimation.SHOW : ForwardButtonAnimation.HIDE);
    }

    private void prepareForwardAnimation(PropertyAnimator anim, ForwardButtonAnimation animation, int width) {
        if (animation == ForwardButtonAnimation.HIDE) {
            anim.attach(mForward,
                      PropertyAnimator.Property.TRANSLATION_X,
                      -width);
            anim.attach(mForward,
                      PropertyAnimator.Property.ALPHA,
                      0);
            anim.attach(mTitle,
                      PropertyAnimator.Property.TRANSLATION_X,
                      0);
            anim.attach(mFavicon,
                      PropertyAnimator.Property.TRANSLATION_X,
                      0);
            anim.attach(mSiteSecurity,
                      PropertyAnimator.Property.TRANSLATION_X,
                      0);

            
            
            
            ViewHelper.setTranslationX(mTitle, mUrlBarViewOffset);
            ViewHelper.setTranslationX(mFavicon, mUrlBarViewOffset);
            ViewHelper.setTranslationX(mSiteSecurity, mUrlBarViewOffset);
        } else {
            anim.attach(mForward,
                      PropertyAnimator.Property.TRANSLATION_X,
                      width);
            anim.attach(mForward,
                      PropertyAnimator.Property.ALPHA,
                      1);
            anim.attach(mTitle,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mUrlBarViewOffset);
            anim.attach(mFavicon,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mUrlBarViewOffset);
            anim.attach(mSiteSecurity,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mUrlBarViewOffset);
        }
    }

    @Override
    public boolean addActionItem(View actionItem) {
        mActionItemBar.addView(actionItem);
        return true;
    }

    @Override
    public void removeActionItem(View actionItem) {
        mActionItemBar.removeView(actionItem);
    }

    public void show() {
        setVisibility(View.VISIBLE);
    }

    public void hide() {
        setVisibility(View.GONE);
    }

    private void refreshState() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null) {
            setFavicon(tab.getFavicon());
            setProgressVisibility(tab.getState() == Tab.STATE_LOADING);
            setSecurityMode(tab.getSecurityMode());
            setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
            updateBackButton(tab);
            updateForwardButton(tab);

            final boolean isPrivate = tab.isPrivate();
            setPrivateMode(isPrivate);
            mTabs.setPrivateMode(isPrivate);
            mTitle.setPrivateMode(isPrivate);
            mMenu.setPrivateMode(isPrivate);
            mMenuIcon.setPrivateMode(isPrivate);
            mUrlEditLayout.setPrivateMode(isPrivate);

            if (mBack instanceof BackButton)
                ((BackButton) mBack).setPrivateMode(isPrivate);

            if (mForward instanceof ForwardButton)
                ((ForwardButton) mForward).setPrivateMode(isPrivate);
        }
    }

    public View getDoorHangerAnchor() {
        return mFavicon;
    }

    public void onDestroy() {
        if (mPrefObserverId != null) {
             PrefsHelper.removeObserver(mPrefObserverId);
             mPrefObserverId = null;
        }
        Tabs.unregisterOnTabsChangedListener(this);

        unregisterEventListener("Reader:Click");
        unregisterEventListener("Reader:LongClick");
    }

    public boolean openOptionsMenu() {
        if (!mHasSoftMenuButton)
            return false;

        
        if (mMenuPopup == null) {
            View panel = mActivity.getMenuPanel();
            mMenuPopup = new MenuPopup(mActivity);
            mMenuPopup.setPanelView(panel);

            mMenuPopup.setOnDismissListener(new PopupWindow.OnDismissListener() {
                @Override
                public void onDismiss() {
                    mActivity.onOptionsMenuClosed(null);
                }
            });
        }

        GeckoAppShell.getGeckoInterface().invalidateOptionsMenu();
        if (!mMenuPopup.isShowing())
            mMenuPopup.showAsDropDown(mMenu);

        return true;
    }

    public boolean closeOptionsMenu() {
        if (!mHasSoftMenuButton)
            return false;

        if (mMenuPopup != null && mMenuPopup.isShowing())
            mMenuPopup.dismiss();

        return true;
    }

    protected void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    protected void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        Log.d(LOGTAG, "handleMessage: " + event);
        if (event.equals("Reader:Click")) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                tab.toggleReaderMode();
            }
        } else if (event.equals("Reader:LongClick")) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                tab.addToReadingList();
            }
        }
    }

    @Override
    public void onLightweightThemeChanged() {
        Drawable drawable = mTheme.getDrawable(this);
        if (drawable == null)
            return;

        StateListDrawable stateList = new StateListDrawable();
        stateList.addState(PRIVATE_STATE_SET, getColorDrawable(R.color.background_private));
        stateList.addState(EMPTY_STATE_SET, drawable);

        setBackgroundDrawable(stateList);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.url_bar_bg);
    }
}
