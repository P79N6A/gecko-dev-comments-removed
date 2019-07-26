




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.HardwareUtils;

import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.SystemClock;
import android.text.style.ForegroundColorSpan;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.Window;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.Animation;
import android.view.animation.AlphaAnimation;
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

public class BrowserToolbar implements Tabs.OnTabsChangedListener,
                                       GeckoMenu.ActionItemBarPresenter,
                                       Animation.AnimationListener,
                                       SharedPreferences.OnSharedPreferenceChangeListener {
    private static final String LOGTAG = "GeckoToolbar";
    public static final String PREFS_NAME = "BrowserToolbar";
    public static final String PREFS_SHOW_URL = "ShowUrl";
    private GeckoRelativeLayout mLayout;
    private LayoutParams mAwesomeBarParams;
    private View mAwesomeBarContent;
    private View mAwesomeBarEntry;
    private ImageView mAwesomeBarRightEdge;
    private BrowserToolbarBackground mAddressBarBg;
    private GeckoTextView mTitle;
    private int mTitlePadding;
    private boolean mSiteSecurityVisible;
    private boolean mSwitchingTabs;
    private ShapedButton mTabs;
    private int mTabsPaneWidth;
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
    private LinearLayout mActionItemBar;
    private MenuPopup mMenuPopup;
    private List<View> mFocusOrder;

    final private BrowserApp mActivity;
    private Handler mHandler;
    private boolean mHasSoftMenuButton;

    private boolean mShowSiteSecurity;
    private boolean mShowReader;

    private static List<View> sActionItems;

    private boolean mAnimatingEntry;

    private AlphaAnimation mLockFadeIn;
    private TranslateAnimation mTitleSlideLeft;
    private TranslateAnimation mTitleSlideRight;

    private int mAddressBarViewOffset;
    private int mDefaultForwardMargin;
    private PropertyAnimator mForwardAnim = null;

    private int mFaviconSize;

    private PropertyAnimator mVisibilityAnimator;

    private static final int TABS_CONTRACTED = 1;
    private static final int TABS_EXPANDED = 2;

    private static final int FORWARD_ANIMATION_DURATION = 450;
    private final ForegroundColorSpan mUrlColor;
    private final ForegroundColorSpan mDomainColor;
    private final ForegroundColorSpan mPrivateDomainColor;

    private boolean mShowUrl;

    public BrowserToolbar(BrowserApp activity) {
        
        mActivity = activity;

        sActionItems = new ArrayList<View>();
        Tabs.registerOnTabsChangedListener(this);
        mSwitchingTabs = true;

        mAnimatingEntry = false;
        mShowUrl = false;

        (new UiAsyncTask<Void, Void, Void>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public synchronized Void doInBackground(Void... params) {
                SharedPreferences settings = mActivity.getSharedPreferences(PREFS_NAME, 0);
                settings.registerOnSharedPreferenceChangeListener(BrowserToolbar.this);
                mShowUrl = settings.getBoolean(PREFS_SHOW_URL, false);
                return null;
            }

            @Override
            public void onPostExecute(Void v) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    setTitle(tab.getDisplayTitle());
                }
            }
        }).execute();

        Resources res = mActivity.getResources();
        mUrlColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_urltext));
        mDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext));
        mPrivateDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext_private));

    }

    public void from(RelativeLayout layout) {
        if (mLayout != null) {
            
            layout.setVisibility(mLayout.getVisibility());
        }

        mLayout = (GeckoRelativeLayout) layout;

        mLayout.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                mActivity.autoHideTabs();
                onAwesomeBarSearch();
            }
        });

        mLayout.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                MenuInflater inflater = mActivity.getMenuInflater();
                inflater.inflate(R.menu.titlebar_contextmenu, menu);

                String clipboard = GeckoAppShell.getClipboardText();
                if (clipboard == null || TextUtils.isEmpty(clipboard)) {
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

        mShowSiteSecurity = false;
        mShowReader = false;

        mAnimatingEntry = false;

        mAddressBarBg = (BrowserToolbarBackground) mLayout.findViewById(R.id.address_bar_bg);
        mAddressBarViewOffset = mActivity.getResources().getDimensionPixelSize(R.dimen.addressbar_offset_left);
        mDefaultForwardMargin = mActivity.getResources().getDimensionPixelSize(R.dimen.forward_default_offset);
        mAwesomeBarContent = mLayout.findViewById(R.id.awesome_bar_content);
        mAwesomeBarEntry = mLayout.findViewById(R.id.awesome_bar_entry);

        
        mAwesomeBarRightEdge = (ImageView) mLayout.findViewById(R.id.awesome_bar_right_edge);
        mAwesomeBarRightEdge.getDrawable().setLevel(5000);

        
        
        mTabsPaneWidth = 0;

        mTitle = (GeckoTextView) mLayout.findViewById(R.id.awesome_bar_title);
        mTitlePadding = mTitle.getPaddingRight();
        if (Build.VERSION.SDK_INT >= 16)
            mTitle.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);

        mTabs = (ShapedButton) mLayout.findViewById(R.id.tabs);
        mTabs.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleTabs();
            }
        });
        mTabs.setImageLevel(0);

        mTabsCounter = (TabCounter) mLayout.findViewById(R.id.tabs_counter);

        mBack = (ImageButton) mLayout.findViewById(R.id.back);
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

        mForward = (ImageButton) mLayout.findViewById(R.id.forward);
        mForward.setEnabled(false); 
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

                SiteIdentityPopup.getInstance().show(mSiteSecurity);
            }
        };

        mFavicon = (ImageButton) mLayout.findViewById(R.id.favicon);
        mFavicon.setOnClickListener(faviconListener);
        if (Build.VERSION.SDK_INT >= 16)
            mFavicon.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        mFaviconSize = Math.round(mActivity.getResources().getDimension(R.dimen.browser_toolbar_favicon_size));

        mSiteSecurity = (ImageButton) mLayout.findViewById(R.id.site_security);
        mSiteSecurity.setOnClickListener(faviconListener);
        mSiteSecurityVisible = (mSiteSecurity.getVisibility() == View.VISIBLE);

        mProgressSpinner = (AnimationDrawable) mActivity.getResources().getDrawable(R.drawable.progress_spinner);
        
        mStop = (ImageButton) mLayout.findViewById(R.id.stop);
        mStop.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null)
                    tab.doStop();
                setProgressVisibility(false);
            }
        });

        mReader = (ImageButton) mLayout.findViewById(R.id.reader);
        mReader.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    if (ReaderModeUtils.isAboutReader(tab.getURL())) {
                        tab.doBack();
                    } else {
                        tab.readerMode();
                    }
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

        mShadow = (ImageView) mLayout.findViewById(R.id.shadow);
        mShadow.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
            }
        });

        mHandler = new Handler();

        float slideWidth = mActivity.getResources().getDimension(R.dimen.browser_toolbar_lock_width);

        LinearLayout.LayoutParams siteSecParams = (LinearLayout.LayoutParams) mSiteSecurity.getLayoutParams();
        final float scale = mActivity.getResources().getDisplayMetrics().density;
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

        mMenu = (GeckoImageButton) mLayout.findViewById(R.id.menu);
        mActionItemBar = (LinearLayout) mLayout.findViewById(R.id.menu_items);
        mHasSoftMenuButton = !HardwareUtils.hasMenuButton();

        if (mHasSoftMenuButton) {
            mMenu.setVisibility(View.VISIBLE);
            mMenu.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mActivity.openOptionsMenu();
                }
            });
        }

        if (!HardwareUtils.isTablet()) {
            
            
            mLayout.post(new Runnable() {
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

        if (Build.VERSION.SDK_INT >= 11) {
            View panel = mActivity.getMenuPanel();

            
            
            
            

            if (panel == null) {
                mActivity.onCreatePanelMenu(Window.FEATURE_OPTIONS_PANEL, null);
                panel = mActivity.getMenuPanel();

                if (mHasSoftMenuButton) {
                    mMenuPopup = new MenuPopup(mActivity);
                    mMenuPopup.setPanelView(panel);

                    mMenuPopup.setOnDismissListener(new PopupWindow.OnDismissListener() {
                        @Override
                        public void onDismiss() {
                            mActivity.onOptionsMenuClosed(null);
                        }
                    });
                }
            }
        }

        mFocusOrder = Arrays.asList(mBack, mForward, mLayout, mReader, mSiteSecurity, mStop, mTabs);
    }

    public View getLayout() {
        return mLayout;
    }

    public void refreshBackground() {
        mAddressBarBg.requestLayout();

        if (mAwesomeBarRightEdge != null)
            mAwesomeBarRightEdge.requestLayout();
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        switch(msg) {
            case TITLE:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    setTitle(tab.getDisplayTitle());
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
                    
                    setTitle(tab.getDisplayTitle());
                }
                break;
            case RESTORED:
                updateTabCount(Tabs.getInstance().getDisplayCount());
                break;
            case SELECTED:
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
                updateTabCountAndAnimate(Tabs.getInstance().getDisplayCount());
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
        return mLayout.getScrollY() == 0;
    }

    public void setNextFocusDownId(int nextId) {
        mLayout.setNextFocusDownId(nextId);
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
        return mLayout.getWidth() - mAwesomeBarEntry.getRight();
    }

    private int getAwesomeBarCurveTranslation() {
        return mLayout.getWidth() - mTabs.getLeft();
    }

    public void fromAwesomeBarSearch(String url) {
        
        
        if (url != null && url.length() > 0) {
            setTitle(url);
        }

        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            return;
        }

        AnimatorProxy proxy = null;

        
        
        
        
        
        if (!mLayout.isSelected()) {
            
            mLayout.setSelected(true);

            final int entryTranslation = getAwesomeBarEntryTranslation();
            final int curveTranslation = getAwesomeBarCurveTranslation();

            proxy = AnimatorProxy.create(mAwesomeBarRightEdge);
            proxy.setTranslationX(entryTranslation);
            proxy = AnimatorProxy.create(mTabs);
            proxy.setTranslationX(curveTranslation);
            proxy = AnimatorProxy.create(mTabsCounter);
            proxy.setTranslationX(curveTranslation);
            proxy = AnimatorProxy.create(mActionItemBar);
            proxy.setTranslationX(curveTranslation);

            if (mHasSoftMenuButton) {
                proxy = AnimatorProxy.create(mMenu);
                proxy.setTranslationX(curveTranslation);
            }

            proxy = AnimatorProxy.create(mReader);
            proxy.setAlpha(0);
            proxy = AnimatorProxy.create(mStop);
            proxy.setAlpha(0);
        }

        final PropertyAnimator contentAnimator = new PropertyAnimator(250);
        contentAnimator.setUseHardwareLayer(false);

        
        contentAnimator.attach(mAwesomeBarRightEdge,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mTabs,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mTabsCounter,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mActionItemBar,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);

        if (mHasSoftMenuButton)
            contentAnimator.attach(mMenu,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   0);

        contentAnimator.setPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                
                mLayout.setSelected(false);

                PropertyAnimator buttonsAnimator = new PropertyAnimator(300);

                
                
                buttonsAnimator.attach(mReader,
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

        mHandler.postDelayed(new Runnable() {
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

        
        mLayout.setSelected(true);

        
        AnimatorProxy proxy = AnimatorProxy.create(mReader);
        proxy.setAlpha(0);
        proxy = AnimatorProxy.create(mStop);
        proxy.setAlpha(0);

        
        contentAnimator.attach(mAwesomeBarRightEdge,
                               PropertyAnimator.Property.TRANSLATION_X,
                               entryTranslation);
        contentAnimator.attach(mTabs,
                               PropertyAnimator.Property.TRANSLATION_X,
                               curveTranslation);
        contentAnimator.attach(mTabsCounter,
                               PropertyAnimator.Property.TRANSLATION_X,
                               curveTranslation);
        contentAnimator.attach(mActionItemBar,
                               PropertyAnimator.Property.TRANSLATION_X,
                               curveTranslation);

        if (mHasSoftMenuButton)
            contentAnimator.attach(mMenu,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   curveTranslation);

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

    public void updateTabCountAndAnimate(int count) {
        
        if (!isVisible()) {
            updateTabCount(count);
            return;
        }

        
        
        
        
        if (!mLayout.isSelected()) {
            mTabsCounter.setCount(count);

            mTabs.setContentDescription((count > 1) ?
                                        mActivity.getString(R.string.num_tabs, count) :
                                        mActivity.getString(R.string.one_tab));
        }
    }

    public void updateTabCount(int count) {
        
        
        
        
        if (mLayout.isSelected()) {
            return;
        }

        mTabsCounter.setCurrentText(String.valueOf(count));
        mTabs.setContentDescription((count > 1) ?
                                    mActivity.getString(R.string.num_tabs, count) :
                                    mActivity.getString(R.string.one_tab));
        updateTabs(mActivity.areTabsShown());
    }

    public void prepareTabsAnimation(PropertyAnimator animator, int width) {
        animator.attach(mAwesomeBarEntry,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mAddressBarBg,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mTabs,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mTabsCounter,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mBack,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mForward,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mTitle,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mFavicon,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mSiteSecurity,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);

        
        adjustTabsAnimation(false);

        mTabsPaneWidth = width;

        
        
        
        if (mTabsPaneWidth > 0)
            setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
    }

    public void adjustTabsAnimation(boolean reset) {
        int width = reset ? 0 : mTabsPaneWidth;
        mAwesomeBarEntry.setTranslationX(width);
        mAddressBarBg.setTranslationX(width);
        mTabs.setTranslationX(width);
        mTabsCounter.setTranslationX(width);
        mBack.setTranslationX(width);
        mForward.setTranslationX(width);
        mTitle.setTranslationX(width);
        mFavicon.setTranslationX(width);
        mSiteSecurity.setTranslationX(width);

        ((ViewGroup.MarginLayoutParams) mLayout.getLayoutParams()).leftMargin = reset ? mTabsPaneWidth : 0;
    }

    public void finishTabsAnimation() {
        setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
    }

    public void adjustForTabsLayout(int width) {
        mTabsPaneWidth = width;
        adjustTabsAnimation(true);
    }

    public void updateTabs(boolean areTabsShown) {
        if (areTabsShown)
            mTabs.setImageLevel(TABS_EXPANDED);
        else
            mTabs.setImageLevel(TABS_CONTRACTED);

        
        
        mTabs.requestLayout();
    }

    public void setIsSideBar(boolean isSideBar) {
        Resources resources = mActivity.getResources();
        mTabs.setBackgroundDrawable(resources.getDrawable(R.drawable.shaped_button));
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

        
        
        
        boolean exitableReaderMode = false;
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null)
            exitableReaderMode = ReaderModeUtils.isAboutReader(tab.getURL()) && tab.canDoBack();
        mReader.setImageResource(exitableReaderMode ? R.drawable.reader_active : R.drawable.reader);
        mReader.setVisibility(!isLoading && (mShowReader || exitableReaderMode) ? View.VISIBLE : View.GONE);

        
        
        
        mTitle.setPadding(0, 0, (!isLoading && !(mShowReader || exitableReaderMode) ? mTitlePadding : 0), 0);

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

        for (View view : mFocusOrder) {
            if (view.getVisibility() != View.VISIBLE)
                continue;

            if (prevView != null) {
                view.setNextFocusLeftId(prevView.getId());
                prevView.setNextFocusRightId(view.getId());
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

    private void setTitle(String title) {
        Tab tab = Tabs.getInstance().getSelectedTab();
        CharSequence displayTitle = title;

        
        if (tab != null && tab.isEnteringReaderMode())
            return;

        
        
        
        if (tab != null && ("about:home".equals(title) ||
                            "about:privatebrowsing".equals(title))) {
            displayTitle = null;
        }

        if (mShowUrl && displayTitle != null) {
            title = StringUtils.stripScheme(tab.getURL());
            title = StringUtils.stripCommonSubdomains(title);
            displayTitle = title;

            
            String baseDomain = tab.getBaseDomain();
            if (!TextUtils.isEmpty(baseDomain)) {
                SpannableStringBuilder builder = new SpannableStringBuilder(title);
                int index = title.indexOf(baseDomain);
                if (index > -1) {
                    builder.setSpan(mUrlColor, 0, title.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
                    builder.setSpan(tab.isPrivate() ? mPrivateDomainColor : mDomainColor, index, index+baseDomain.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);

                    displayTitle = builder;
                }
            }
        }

        mTitle.setText(displayTitle);
        mLayout.setContentDescription(title != null ? title : mTitle.getHint());
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
        mShowSiteSecurity = true;

        if (mode.equals(SiteIdentityPopup.IDENTIFIED)) {
            mSiteSecurity.setImageLevel(1);
        } else if (mode.equals(SiteIdentityPopup.VERIFIED)) {
            mSiteSecurity.setImageLevel(2);
        } else {
            mSiteSecurity.setImageLevel(0);
            mShowSiteSecurity = false;
        }

        setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
    }

    private void setReaderMode(boolean showReader) {
        mShowReader = showReader;
        setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
    }

    public void requestFocusFromTouch() {
        mLayout.requestFocusFromTouch();
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
                        (ViewGroup.MarginLayoutParams)mAwesomeBarContent.getLayoutParams();
                    layoutParams.leftMargin = 0;
                    mAwesomeBarContent.requestLayout();
                    
                    
                    
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (enabled) {
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mAwesomeBarContent.getLayoutParams();
                    layoutParams.leftMargin = mAddressBarViewOffset;

                    AnimatorProxy proxy = AnimatorProxy.create(mTitle);
                    proxy.setTranslationX(0);
                    proxy = AnimatorProxy.create(mFavicon);
                    proxy.setTranslationX(0);
                    proxy = AnimatorProxy.create(mSiteSecurity);
                    proxy.setTranslationX(0);
                }

                ViewGroup.MarginLayoutParams layoutParams =
                    (ViewGroup.MarginLayoutParams)mForward.getLayoutParams();
                layoutParams.leftMargin = mDefaultForwardMargin + (mForward.isEnabled() ? mForward.getWidth() / 2 : 0);
                AnimatorProxy proxy = AnimatorProxy.create(mForward);
                proxy.setTranslationX(0);

                mAwesomeBarContent.requestLayout();
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

            
            
            
            AnimatorProxy proxy = AnimatorProxy.create(mTitle);
            proxy.setTranslationX(mAddressBarViewOffset);
            proxy = AnimatorProxy.create(mFavicon);
            proxy.setTranslationX(mAddressBarViewOffset);
            proxy = AnimatorProxy.create(mSiteSecurity);
            proxy.setTranslationX(mAddressBarViewOffset);
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

        if (!sActionItems.contains(actionItem))
            sActionItems.add(actionItem);
    }

    @Override
    public void removeActionItem(int index) {
        mActionItemBar.removeViewAt(index);
        sActionItems.remove(index);
    }

    @Override
    public int getActionItemsCount() {
        return sActionItems.size();
    }

    public void show() {
        mLayout.setVisibility(View.VISIBLE);
    }

    public void hide() {
        mLayout.setVisibility(View.GONE);
    }

    public void refresh() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null) {
            String url = tab.getURL();
            setTitle(tab.getDisplayTitle());
            setFavicon(tab.getFavicon());
            setProgressVisibility(tab.getState() == Tab.STATE_LOADING);
            setSecurityMode(tab.getSecurityMode());
            setReaderMode(tab.getReaderEnabled());
            setShadowVisibility(true);
            updateTabCount(Tabs.getInstance().getDisplayCount());
            updateBackButton(tab.canDoBack());
            updateForwardButton(tab.canDoForward());

            final boolean isPrivate = tab.isPrivate();
            mAddressBarBg.setPrivateMode(isPrivate);
            mLayout.setPrivateMode(isPrivate);
            mTabs.setPrivateMode(isPrivate);
            mTitle.setPrivateMode(isPrivate);
            mMenu.setPrivateMode(isPrivate);

            if (mBack instanceof BackButton)
                ((BackButton) mBack).setPrivateMode(isPrivate);

            if (mForward instanceof ForwardButton)
                ((ForwardButton) mForward).setPrivateMode(isPrivate);
        }
    }

    public void onDestroy() {
        Tabs.unregisterOnTabsChangedListener(this);
    }

    public boolean openOptionsMenu() {
        if (!mHasSoftMenuButton)
            return false;

        GeckoApp.mAppContext.invalidateOptionsMenu();
        if (mMenuPopup != null && !mMenuPopup.isShowing())
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

    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals(PREFS_SHOW_URL)) {
            mShowUrl = sharedPreferences.getBoolean(key, false);
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                setTitle(tab.getDisplayTitle());
            }
        }
    }
}
