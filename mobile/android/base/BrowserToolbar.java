




package org.mozilla.gecko;

import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.MenuPopup;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.HardwareUtils;

import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import org.mozilla.gecko.PrefsHelper;

import org.json.JSONObject;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.os.SystemClock;
import android.text.style.ForegroundColorSpan;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.Window;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.AlphaAnimation;
import android.view.animation.Interpolator;
import android.view.animation.TranslateAnimation;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.ViewSwitcher;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class BrowserToolbar extends GeckoRelativeLayout
                            implements Tabs.OnTabsChangedListener,
                                       GeckoMenu.ActionItemBarPresenter,
                                       Animation.AnimationListener {
    private static final String LOGTAG = "GeckoToolbar";
    public static final String PREF_TITLEBAR_MODE = "browser.chrome.titlebarMode";
    private LayoutParams mAwesomeBarParams;
    private View mUrlDisplayContainer;
    private View mAwesomeBarEntry;
    private ImageView mAwesomeBarRightEdge;
    private BrowserToolbarBackground mAddressBarBg;
    private GeckoTextView mTitle;
    private int mTitlePadding;
    private boolean mSiteSecurityVisible;
    private boolean mSwitchingTabs;
    private ShapedButton mTabs;
    private ImageButton mBack;
    private ImageButton mForward;
    public ImageButton mFavicon;
    public ImageButton mStop;
    public ImageButton mSiteSecurity;
    public ImageButton mReader;
    private AnimationDrawable mProgressSpinner;
    private TabCounter mTabsCounter;
    private ImageView mShadow;
    private GeckoImageButton mMenu;
    private GeckoImageView mMenuIcon;
    private LinearLayout mActionItemBar;
    private MenuPopup mMenuPopup;
    private List<? extends View> mFocusOrder;

    final private BrowserApp mActivity;
    private boolean mHasSoftMenuButton;

    private boolean mShowSiteSecurity;
    private boolean mShowReader;

    private boolean mAnimatingEntry;

    private AlphaAnimation mLockFadeIn;
    private TranslateAnimation mTitleSlideLeft;
    private TranslateAnimation mTitleSlideRight;

    private int mAddressBarViewOffset;
    private int mDefaultForwardMargin;
    private PropertyAnimator mForwardAnim = null;

    private int mFaviconSize;

    private PropertyAnimator mVisibilityAnimator;
    private static final Interpolator sButtonsInterpolator = new AccelerateInterpolator();

    private static final int TABS_CONTRACTED = 1;
    private static final int TABS_EXPANDED = 2;

    private static final int FORWARD_ANIMATION_DURATION = 450;
    private final ForegroundColorSpan mUrlColor;
    private final ForegroundColorSpan mDomainColor;
    private final ForegroundColorSpan mPrivateDomainColor;

    private boolean mShowUrl;

    private Integer mPrefObserverId;

    public BrowserToolbar(Context context) {
        this(context, null);
    }

    public BrowserToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        mActivity = (BrowserApp) context;

        
        LayoutInflater.from(context).inflate(R.layout.browser_toolbar, this);

        Tabs.registerOnTabsChangedListener(this);
        mSwitchingTabs = true;

        mAnimatingEntry = false;
        mShowUrl = false;

        
        mPrefObserverId = PrefsHelper.getPref(PREF_TITLEBAR_MODE, new PrefsHelper.PrefHandlerBase() {
            @Override
            public void prefValue(String pref, String str) {
                int value = Integer.parseInt(str);
                boolean shouldShowUrl = (value == 1);

                if (shouldShowUrl == mShowUrl) {
                    return;
                }
                mShowUrl = shouldShowUrl;

                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateTitle();
                    }
                });
            }

            @Override
            public boolean isObserver() {
                
                
                return true;
            }
        });

        Resources res = getResources();
        mUrlColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_urltext));
        mDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext));
        mPrivateDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext_private));

        mShowSiteSecurity = false;
        mShowReader = false;

        mAnimatingEntry = false;

        mAddressBarBg = (BrowserToolbarBackground) findViewById(R.id.address_bar_bg);
        mAddressBarViewOffset = res.getDimensionPixelSize(R.dimen.addressbar_offset_left);
        mDefaultForwardMargin = res.getDimensionPixelSize(R.dimen.forward_default_offset);
        mUrlDisplayContainer = findViewById(R.id.awesome_bar_display_container);
        mAwesomeBarEntry = findViewById(R.id.awesome_bar_entry);

        
        mAwesomeBarRightEdge = (ImageView) findViewById(R.id.awesome_bar_right_edge);
        if (mAwesomeBarRightEdge != null) {
            mAwesomeBarRightEdge.getDrawable().setLevel(5000);
        }

        mTitle = (GeckoTextView) findViewById(R.id.awesome_bar_title);
        mTitlePadding = mTitle.getPaddingRight();

        mTabs = (ShapedButton) findViewById(R.id.tabs);
        mTabsCounter = (TabCounter) findViewById(R.id.tabs_counter);
        mBack = (ImageButton) findViewById(R.id.back);
        mForward = (ImageButton) findViewById(R.id.forward);
        mForward.setEnabled(false); 

        mFavicon = (ImageButton) findViewById(R.id.favicon);
        if (Build.VERSION.SDK_INT >= 16)
            mFavicon.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        mFaviconSize = Math.round(res.getDimension(R.dimen.browser_toolbar_favicon_size));

        mSiteSecurity = (ImageButton) findViewById(R.id.site_security);
        mSiteSecurityVisible = (mSiteSecurity.getVisibility() == View.VISIBLE);
        mActivity.getSiteIdentityPopup().setAnchor(mSiteSecurity);

        mProgressSpinner = (AnimationDrawable) res.getDrawable(R.drawable.progress_spinner);

        mStop = (ImageButton) findViewById(R.id.stop);
        mReader = (ImageButton) findViewById(R.id.reader);
        mShadow = (ImageView) findViewById(R.id.shadow);

        if (Build.VERSION.SDK_INT >= 16) {
            mShadow.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        }

        mMenu = (GeckoImageButton) findViewById(R.id.menu);
        mMenuIcon = (GeckoImageView) findViewById(R.id.menu_icon);
        mActionItemBar = (LinearLayout) findViewById(R.id.menu_items);
        mHasSoftMenuButton = !HardwareUtils.hasMenuButton();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                mActivity.autoHideTabs();
                onAwesomeBarSearch();
            }
        });

        setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
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
                    if (!tab.getFeedsEnabled()) {
                        menu.findItem(R.id.subscribe).setVisible(false);
                    }
                } else {
                    
                    menu.findItem(R.id.copyurl).setVisible(false);
                    menu.findItem(R.id.share).setVisible(false);
                    menu.findItem(R.id.add_to_launcher).setVisible(false);
                    menu.findItem(R.id.subscribe).setVisible(false);
                }
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

                JSONObject identityData = Tabs.getInstance().getSelectedTab().getIdentityData();
                if (identityData == null) {
                    Log.e(LOGTAG, "Selected tab has no identity data");
                    return;
                }
                SiteIdentityPopup siteIdentityPopup = mActivity.getSiteIdentityPopup();
                siteIdentityPopup.updateIdentity(identityData);
                siteIdentityPopup.show();
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

        mReader.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    tab.toggleReaderMode();
                }
            }
        });

        mReader.setOnLongClickListener(new Button.OnLongClickListener() {
            public boolean onLongClick(View v) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    tab.addToReadingList();
                    return true;
                }

                return false;
            }
        });

        mShadow.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
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

        if (!HardwareUtils.isTablet()) {
            
            
            post(new Runnable() {
                @Override
                public void run() {
                    int height = mTabs.getHeight();
                    int width = mTabs.getWidth();
                    int tail = (width - height) / 2;

                    Rect bounds = new Rect(0, 0, tail, height);
                    TailTouchDelegate delegate = new TailTouchDelegate(bounds, mShadow);
                    mTabs.setTouchDelegate(delegate);
                }
            });
        }

        
        
        if (HardwareUtils.isTablet()) {
            mFocusOrder = Arrays.asList(mTabs, mBack, mForward, this,
                    mSiteSecurity, mReader, mStop, mActionItemBar, mMenu);
        } else {
            mFocusOrder = Arrays.asList(this, mSiteSecurity, mReader, mStop,
                    mTabs, mMenu);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        
        
        if (event != null && event.getY() > getHeight() - getScrollY()) {
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
        switch(msg) {
            case TITLE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    updateTitle();
                }
                break;
            case START:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    updateBackButton(tab.canDoBack());
                    updateForwardButton(tab.canDoForward());
                    Boolean showProgress = (Boolean)data;
                    if (showProgress && tab.getState() == Tab.STATE_LOADING)
                        setProgressVisibility(true);
                    setSecurityMode(tab.getSecurityMode());
                    setReaderMode(tab.getReaderEnabled());
                }
                break;
            case STOP:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    updateBackButton(tab.canDoBack());
                    updateForwardButton(tab.canDoForward());
                    setProgressVisibility(false);
                    
                    updateTitle();
                }
                break;
            case RESTORED:
                
            case SELECTED:
                updateTabCount(Tabs.getInstance().getDisplayCount());
                mSwitchingTabs = true;
                
            case LOCATION_CHANGE:
            case LOAD_ERROR:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    refresh();
                }
                mSwitchingTabs = false;
                break;
            case CLOSED:
            case ADDED:
                updateTabCount(Tabs.getInstance().getDisplayCount());
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    updateBackButton(tab.canDoBack());
                    updateForwardButton(tab.canDoForward());
                }
                break;
            case FAVICON:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    setFavicon(tab.getFavicon());
                }
                break;
            case SECURITY_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    setSecurityMode(tab.getSecurityMode());
                }
                break;
            case READER_ENABLED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    setReaderMode(tab.getReaderEnabled());
                }
                break;
        }
    }

    public boolean isVisible() {
        return getScrollY() == 0;
    }

    public void setNextFocusDownId(int nextId) {
        super.setNextFocusDownId(nextId);
        mTabs.setNextFocusDownId(nextId);
        mBack.setNextFocusDownId(nextId);
        mForward.setNextFocusDownId(nextId);
        mFavicon.setNextFocusDownId(nextId);
        mStop.setNextFocusDownId(nextId);
        mSiteSecurity.setNextFocusDownId(nextId);
        mReader.setNextFocusDownId(nextId);
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

    private int getAwesomeBarEntryTranslation() {
        return getWidth() - mAwesomeBarEntry.getRight();
    }

    private int getAwesomeBarCurveTranslation() {
        return getWidth() - mTabs.getLeft();
    }

    public void fromAwesomeBarSearch(String url) {
        
        
        if (url != null && url.length() > 0) {
            setTitle(url);
        }

        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            return;
        }

        
        
        
        
        
        if (!isSelected()) {
            
            setSelected(true);

            final int entryTranslation = getAwesomeBarEntryTranslation();
            final int curveTranslation = getAwesomeBarCurveTranslation();

            if (mAwesomeBarRightEdge != null) {
                ViewHelper.setTranslationX(mAwesomeBarRightEdge, entryTranslation);
            }

            ViewHelper.setTranslationX(mTabs, curveTranslation);
            ViewHelper.setTranslationX(mTabsCounter, curveTranslation);
            ViewHelper.setTranslationX(mActionItemBar, curveTranslation);

            if (mHasSoftMenuButton) {
                ViewHelper.setTranslationX(mMenu, curveTranslation);
                ViewHelper.setTranslationX(mMenuIcon, curveTranslation);
            }

            ViewHelper.setAlpha(mReader, 0);
            ViewHelper.setAlpha(mStop, 0);
        }

        final PropertyAnimator contentAnimator = new PropertyAnimator(250);
        contentAnimator.setUseHardwareLayer(false);

        

        if (mAwesomeBarRightEdge != null) {
            contentAnimator.attach(mAwesomeBarRightEdge,
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

        contentAnimator.setPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                
                setSelected(false);

                PropertyAnimator buttonsAnimator = new PropertyAnimator(300);

                
                
                buttonsAnimator.attach(mReader,
                                       PropertyAnimator.Property.ALPHA,
                                       1);
                buttonsAnimator.attach(mStop,
                                       PropertyAnimator.Property.ALPHA,
                                       1);

                buttonsAnimator.start();

                mAnimatingEntry = false;

                
                
                updateTabCount(Tabs.getInstance().getDisplayCount());
            }
        });

        mAnimatingEntry = true;

        postDelayed(new Runnable() {
            @Override
            public void run() {
                contentAnimator.start();
            }
        }, 500);
    }

    private void onAwesomeBarSearch() {
        
        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            mActivity.onSearchRequested();
            return;
        }

        if (mAnimatingEntry)
            return;

        final PropertyAnimator contentAnimator = new PropertyAnimator(250);
        contentAnimator.setUseHardwareLayer(false);

        final int entryTranslation = getAwesomeBarEntryTranslation();
        final int curveTranslation = getAwesomeBarCurveTranslation();

        
        setSelected(true);

        
        ViewHelper.setAlpha(mReader, 0);
        ViewHelper.setAlpha(mStop, 0);

        

        if (mAwesomeBarRightEdge != null) {
            contentAnimator.attach(mAwesomeBarRightEdge,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   entryTranslation);
        }

        contentAnimator.attach(mTabs,
                               PropertyAnimator.Property.TRANSLATION_X,
                               curveTranslation);
        contentAnimator.attach(mTabsCounter,
                               PropertyAnimator.Property.TRANSLATION_X,
                               curveTranslation);
        contentAnimator.attach(mActionItemBar,
                               PropertyAnimator.Property.TRANSLATION_X,
                               curveTranslation);

        if (mHasSoftMenuButton) {
            contentAnimator.attach(mMenu,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   curveTranslation);
            contentAnimator.attach(mMenuIcon,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   curveTranslation);
        }

        contentAnimator.setPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                
                mActivity.onSearchRequested();
                mAnimatingEntry = false;
            }
        });

        mAnimatingEntry = true;
        contentAnimator.start();
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

    public void updateTabCount(int count) {
        
        
        
        
        if (isSelected()) {
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
        
        
        
        if (visible) {
            mFavicon.setImageDrawable(mProgressSpinner);
            mProgressSpinner.start();
            setPageActionVisibility(true);
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber start");
        } else {
            mProgressSpinner.stop();
            setPageActionVisibility(false);
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null)
                setFavicon(selectedTab.getFavicon());
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber stop");
        }
    }

    public void setPageActionVisibility(boolean isLoading) {
        
        mStop.setVisibility(isLoading ? View.VISIBLE : View.GONE);

        
        setSiteSecurityVisibility(mShowSiteSecurity && !isLoading);

        
        
        
        boolean inReaderMode = false;
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null)
            inReaderMode = ReaderModeUtils.isAboutReader(tab.getURL());
        mReader.setImageResource(inReaderMode ? R.drawable.reader_active : R.drawable.reader);

        mReader.setVisibility(!isLoading && (mShowReader || inReaderMode) ? View.VISIBLE : View.GONE);

        
        
        
        mTitle.setPadding(0, 0, (!isLoading && !(mShowReader || inReaderMode) ? mTitlePadding : 0), 0);

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

            if (prevView != null) {
                if (view == mActionItemBar) {
                    final int childCount = mActionItemBar.getChildCount();
                    if (childCount > 1) {
                        View firstChild = mActionItemBar.getChildAt(0);
                        firstChild.setNextFocusLeftId(prevView.getId());
                        prevView.setNextFocusRightId(firstChild.getId());
                    }
                    view = mActionItemBar.getChildAt(childCount - 1);
                }

                view.setNextFocusLeftId(prevView.getId());
                prevView.setNextFocusRightId(view.getId());
            }

            if (needsNewFocus) {
                requestFocus();
            }

            prevView = view;
        }
    }

    public void setShadowVisibility(boolean visible) {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null) {
            return;
        }

        String url = tab.getURL();

        
        visible &= !(url == null || (url.startsWith("about:") && 
                     !url.equals("about:blank")));

        if ((mShadow.getVisibility() == View.VISIBLE) != visible) {
            mShadow.setVisibility(visible ? View.VISIBLE : View.GONE);
        }
    }

    private void setTitle(CharSequence title) {
        mTitle.setText(title);
        setContentDescription(title != null ? title : mTitle.getHint());
    }

    
    private void updateTitle() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        
        if (tab == null || tab.isEnteringReaderMode()) {
            return;
        }

        String url = tab.getURL();
        
        if ("about:home".equals(url) || "about:privatebrowsing".equals(url)) {
            setTitle(null);
            return;
        }

        
        if (!mShowUrl || url == null) {
            setTitle(tab.getDisplayTitle());
            return;
        }

        url = StringUtils.stripScheme(url);
        CharSequence title = StringUtils.stripCommonSubdomains(url);

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
        if (Tabs.getInstance().getSelectedTab().getState() == Tab.STATE_LOADING)
            return;

        if (image != null) {
            image = Bitmap.createScaledBitmap(image, mFaviconSize, mFaviconSize, false);
            mFavicon.setImageBitmap(image);
        } else {
            mFavicon.setImageResource(R.drawable.favicon);
        }
    }
    
    private void setSecurityMode(String mode) {
        int imageLevel = SiteIdentityPopup.getSecurityImageLevel(mode);
        mSiteSecurity.setImageLevel(imageLevel);
        mShowSiteSecurity = (imageLevel != SiteIdentityPopup.LEVEL_UKNOWN);

        setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
    }

    private void setReaderMode(boolean showReader) {
        mShowReader = showReader;
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

    public void updateBackButton(boolean enabled) {
         Drawable drawable = mBack.getDrawable();
         if (drawable != null)
             drawable.setAlpha(enabled ? 255 : 77);

         mBack.setEnabled(enabled);
    }

    public void updateForwardButton(final boolean enabled) {
        if (mForward.isEnabled() == enabled)
            return;

        
        
        mForward.setEnabled(enabled);

        if (mForward.getVisibility() != View.VISIBLE)
            return;

        
        mForwardAnim = new PropertyAnimator(mSwitchingTabs ? 10 : FORWARD_ANIMATION_DURATION);
        final int width = mForward.getWidth() / 2;

        mForwardAnim.setPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                if (!enabled) {
                    
                    
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mUrlDisplayContainer.getLayoutParams();
                    layoutParams.leftMargin = 0;
                    mUrlDisplayContainer.requestLayout();
                    
                    
                    
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (enabled) {
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mUrlDisplayContainer.getLayoutParams();
                    layoutParams.leftMargin = mAddressBarViewOffset;

                    ViewHelper.setTranslationX(mTitle, 0);
                    ViewHelper.setTranslationX(mFavicon, 0);
                    ViewHelper.setTranslationX(mSiteSecurity, 0);
                }

                ViewGroup.MarginLayoutParams layoutParams =
                    (ViewGroup.MarginLayoutParams)mForward.getLayoutParams();
                layoutParams.leftMargin = mDefaultForwardMargin + (mForward.isEnabled() ? width : 0);
                ViewHelper.setTranslationX(mForward, 0);

                mUrlDisplayContainer.requestLayout();
                mForwardAnim = null;
            }
        });

        prepareForwardAnimation(mForwardAnim, enabled, width);
        mForwardAnim.start();
    }

    private void prepareForwardAnimation(PropertyAnimator anim, boolean enabled, int width) {
        if (!enabled) {
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

            
            
            
            ViewHelper.setTranslationX(mTitle, mAddressBarViewOffset);
            ViewHelper.setTranslationX(mFavicon, mAddressBarViewOffset);
            ViewHelper.setTranslationX(mSiteSecurity, mAddressBarViewOffset);
        } else {
            anim.attach(mForward,
                      PropertyAnimator.Property.TRANSLATION_X,
                      width);
            anim.attach(mForward,
                      PropertyAnimator.Property.ALPHA,
                      1);
            anim.attach(mTitle,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mAddressBarViewOffset);
            anim.attach(mFavicon,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mAddressBarViewOffset);
            anim.attach(mSiteSecurity,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mAddressBarViewOffset);
        }
    }

    @Override
    public void addActionItem(View actionItem) {
        mActionItemBar.addView(actionItem);
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

    public void refresh() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null) {
            updateTitle();
            setFavicon(tab.getFavicon());
            setProgressVisibility(tab.getState() == Tab.STATE_LOADING);
            setSecurityMode(tab.getSecurityMode());
            setReaderMode(tab.getReaderEnabled());
            setShadowVisibility(true);
            updateBackButton(tab.canDoBack());
            updateForwardButton(tab.canDoForward());

            final boolean isPrivate = tab.isPrivate();
            mAddressBarBg.setPrivateMode(isPrivate);
            setPrivateMode(isPrivate);
            mTabs.setPrivateMode(isPrivate);
            mTitle.setPrivateMode(isPrivate);
            mMenu.setPrivateMode(isPrivate);
            mMenuIcon.setPrivateMode(isPrivate);

            if (mBack instanceof BackButton)
                ((BackButton) mBack).setPrivateMode(isPrivate);

            if (mForward instanceof ForwardButton)
                ((ForwardButton) mForward).setPrivateMode(isPrivate);
        }
    }

    public void onDestroy() {
        if (mPrefObserverId != null) {
             PrefsHelper.removeObserver(mPrefObserverId);
             mPrefObserverId = null;
        }
        Tabs.unregisterOnTabsChangedListener(this);
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
}
