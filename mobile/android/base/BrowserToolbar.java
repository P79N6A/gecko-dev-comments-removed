




package org.mozilla.gecko;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.Window;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.ViewSwitcher;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.TimerTask;

import org.mozilla.gecko.gfx.ImmutableViewportMetrics;

public class BrowserToolbar implements ViewSwitcher.ViewFactory,
                                       Tabs.OnTabsChangedListener,
                                       GeckoMenu.ActionItemBarPresenter,
                                       Animation.AnimationListener {
    private static final String LOGTAG = "GeckoToolbar";
    private LinearLayout mLayout;
    private View mAwesomeBar;
    private LayoutParams mAwesomeBarParams;
    private View mAwesomeBarEntry;
    private int mAwesomeBarEntryRightMargin;
    private GeckoFrameLayout mAwesomeBarRightEdge;
    private BrowserToolbarBackground mAddressBarBg;
    private View mAddressBarView;
    private BrowserToolbarBackground.CurveTowards mAddressBarBgCurveTowards;
    private int mAddressBarBgRightMargin;
    private GeckoTextView mTitle;
    private int mTitlePadding;
    private boolean mSiteSecurityVisible;
    private boolean mAnimateSiteSecurity;
    private TabsButton mTabs;
    private int mTabsPaneWidth;
    private ImageButton mBack;
    private ImageButton mForward;
    public ImageButton mFavicon;
    public ImageButton mStop;
    public ImageButton mSiteSecurity;
    public ImageButton mReader;
    private AnimationDrawable mProgressSpinner;
    private GeckoTextSwitcher mTabsCount;
    private ImageView mShadow;
    private GeckoImageButton mMenu;
    private LinearLayout mActionItemBar;
    private MenuPopup mMenuPopup;
    private List<View> mFocusOrder;

    final private BrowserApp mActivity;
    private LayoutInflater mInflater;
    private Handler mHandler;
    private boolean mHasSoftMenuButton;

    private boolean mShowSiteSecurity;
    private boolean mShowReader;

    private static List<View> sActionItems;

    private boolean mAnimatingEntry;

    private int mDuration;
    private TranslateAnimation mSlideUpIn;
    private TranslateAnimation mSlideUpOut;
    private TranslateAnimation mSlideDownIn;
    private TranslateAnimation mSlideDownOut;

    private AlphaAnimation mLockFadeIn;
    private TranslateAnimation mTitleSlideLeft;
    private TranslateAnimation mTitleSlideRight;

    private int mAddressBarViewOffset;
    private int mAddressBarViewOffsetNoForward;
    private int mDefaultForwardMargin;
    private PropertyAnimator mForwardAnim = null;

    private int mCount;
    private int mFaviconSize;

    private PropertyAnimator mVisibilityAnimator;

    private enum ToolbarVisibility {
        VISIBLE,
        HIDDEN,
        INCONSISTENT
    };
    private ToolbarVisibility mVisibility;

    private static final int TABS_CONTRACTED = 1;
    private static final int TABS_EXPANDED = 2;

    private static final int FORWARD_ANIMATION_DURATION = 450;

    private static final int VISIBILITY_ANIMATION_DURATION = 250;

    public BrowserToolbar(BrowserApp activity) {
        
        mActivity = activity;
        mInflater = LayoutInflater.from(activity);

        sActionItems = new ArrayList<View>();
        Tabs.registerOnTabsChangedListener(this);
        mAnimateSiteSecurity = true;

        mAnimatingEntry = false;

        mVisibility = ToolbarVisibility.INCONSISTENT;
    }

    public void from(LinearLayout layout) {
        if (mLayout != null) {
            
            layout.setVisibility(mLayout.getVisibility());
        }
        mLayout = layout;
        mLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            }
        });

        mShowSiteSecurity = false;
        mShowReader = false;

        mAnimatingEntry = false;

        mAddressBarBg = (BrowserToolbarBackground) mLayout.findViewById(R.id.address_bar_bg);
        mAddressBarView = mLayout.findViewById(R.id.addressbar);
        mAddressBarViewOffset = mActivity.getResources().getDimensionPixelSize(R.dimen.addressbar_offset_left);
        mAddressBarViewOffsetNoForward = mActivity.getResources().getDimensionPixelSize(R.dimen.addressbar_offset_left_noforward);
        mDefaultForwardMargin = mActivity.getResources().getDimensionPixelSize(R.dimen.forward_default_offset);
        mAwesomeBarRightEdge = (GeckoFrameLayout) mLayout.findViewById(R.id.awesome_bar_right_edge);
        mAwesomeBarEntry = mLayout.findViewById(R.id.awesome_bar_entry);

        
        
        mTabsPaneWidth = 0;

        mTitle = (GeckoTextView) mLayout.findViewById(R.id.awesome_bar_title);
        mTitlePadding = mTitle.getPaddingRight();
        if (Build.VERSION.SDK_INT >= 16)
            mTitle.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);

        mAwesomeBar = mLayout.findViewById(R.id.awesome_bar);
        mAwesomeBar.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                mActivity.autoHideTabs();
                onAwesomeBarSearch();
            }
        });
        mAwesomeBar.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
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
                } else {
                    
                    menu.findItem(R.id.copyurl).setVisible(false);
                    menu.findItem(R.id.share).setVisible(false);
                    menu.findItem(R.id.add_to_launcher).setVisible(false);
                }
            }
        });

        mTabs = (TabsButton) mLayout.findViewById(R.id.tabs);
        mTabs.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleTabs();
            }
        });
        mTabs.setImageLevel(0);

        mTabsCount = (GeckoTextSwitcher) mLayout.findViewById(R.id.tabs_count);
        mTabsCount.removeAllViews();
        mTabsCount.setFactory(this);
        mTabsCount.setText("");
        mCount = 0;
        if (Build.VERSION.SDK_INT >= 16) {
            
            
            
            
            
            mTabsCount.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
            mTabsCount.setAccessibilityDelegate(new View.AccessibilityDelegate() {
                    @Override
                    public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {}
                });
        }

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

        mShadow = (ImageView) mLayout.findViewById(R.id.shadow);

        mHandler = new Handler();
        mSlideUpIn = new TranslateAnimation(0, 0, 40, 0);
        mSlideUpOut = new TranslateAnimation(0, 0, 0, -40);
        mSlideDownIn = new TranslateAnimation(0, 0, -40, 0);
        mSlideDownOut = new TranslateAnimation(0, 0, 0, 40);

        mDuration = 750;
        mSlideUpIn.setDuration(mDuration);
        mSlideUpOut.setDuration(mDuration);
        mSlideDownIn.setDuration(mDuration);
        mSlideDownOut.setDuration(mDuration);

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
        mHasSoftMenuButton = !mActivity.hasPermanentMenuKey();

        if (mHasSoftMenuButton) {
            mMenu.setVisibility(View.VISIBLE);
            mMenu.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mActivity.openOptionsMenu();
                }
            });
        }

        if (!mActivity.isTablet()) {
            
            
            mLayout.post(new Runnable() {
                @Override
                public void run() {
                    int height = mTabs.getHeight();
                    int width = mTabs.getWidth();
                    int tail = (width - height) / 2;

                    Rect leftBounds = new Rect(0, 0, tail, height);
                    Rect rightBounds = new Rect(width - tail, 0, width, height);

                    TailTouchDelegate delegate = new TailTouchDelegate(leftBounds, mAddressBarView);

                    if (mHasSoftMenuButton)
                        delegate.add(rightBounds, mMenu);

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

        mFocusOrder = Arrays.asList(mBack, mForward, mAwesomeBar, mReader, mSiteSecurity, mStop, mTabs);
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
                mAnimateSiteSecurity = false;
                
            case LOCATION_CHANGE:
            case LOAD_ERROR:
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    refresh();
                }
                mAnimateSiteSecurity = true;
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

    private boolean canToolbarHide() {
        
        
        ImmutableViewportMetrics metrics = GeckoApp.mAppContext.getLayerView().
            getLayerClient().getViewportMetrics();
        return (metrics.getPageHeight() >= metrics.getHeight());
    }

    public void animateVisibility(boolean show) {
        
        
        if (mVisibility != ToolbarVisibility.INCONSISTENT &&
            show == isVisible()) {
            return;
        }

        cancelVisibilityAnimation();
        mVisibility = show ? ToolbarVisibility.VISIBLE : ToolbarVisibility.HIDDEN;

        mVisibilityAnimator = new PropertyAnimator(VISIBILITY_ANIMATION_DURATION);
        mVisibilityAnimator.attach(mLayout, PropertyAnimator.Property.SCROLL_Y,
                                   show ? 0 : mLayout.getHeight());

        
        
        if (mVisibility == ToolbarVisibility.VISIBLE ||
            canToolbarHide()) {
            mVisibilityAnimator.start();
        }
    }

    






    public void animateVisibilityWithVelocityBias(boolean show, float velocity) {
        
        
        
        float defaultVelocity =
            mLayout.getHeight() / ((VISIBILITY_ANIMATION_DURATION / 1000.0f) * 60);

        if (Math.abs(velocity) > defaultVelocity) {
            show = (velocity > 0) ? false : true;
        }

        animateVisibility(show);
    }

    public void cancelVisibilityAnimation() {
        if (mVisibilityAnimator != null) {
            mVisibility = ToolbarVisibility.INCONSISTENT;
            mVisibilityAnimator.stop(false);
            mVisibilityAnimator = null;
        }
    }

    public boolean isVisible() {
        return mVisibility == ToolbarVisibility.VISIBLE;
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

    @Override
    public View makeView() {
        
        return mInflater.inflate(R.layout.tabs_counter, null);
    }

    private int prepareAwesomeBarAnimation() {
        
        mAwesomeBar.setSelected(true);

        
        
        
        MarginLayoutParams entryParams = (MarginLayoutParams) mAwesomeBarEntry.getLayoutParams();
        mAwesomeBarEntryRightMargin = entryParams.rightMargin;
        entryParams.rightMargin = 0;
        mAwesomeBarEntry.requestLayout();

        
        
        MarginLayoutParams barParams = (MarginLayoutParams) mAddressBarBg.getLayoutParams();
        mAddressBarBgRightMargin = barParams.rightMargin;
        barParams.rightMargin = 0;
        mAddressBarBgCurveTowards = mAddressBarBg.getCurveTowards();
        mAddressBarBg.setCurveTowards(BrowserToolbarBackground.CurveTowards.NONE);

        
        
        int translation = mAwesomeBarEntryRightMargin;

        if (mActionItemBar.getVisibility() == View.VISIBLE) {
            
            
            MarginLayoutParams itemBarParams = (MarginLayoutParams) mActionItemBar.getLayoutParams();
            translation = itemBarParams.rightMargin + mActionItemBar.getWidth() - entryParams.leftMargin;

            
            View awesomeBarParent = (View) mAwesomeBar.getParent();
            mAwesomeBarParams = (LayoutParams) awesomeBarParent.getLayoutParams();
            awesomeBarParent.setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                              ViewGroup.LayoutParams.MATCH_PARENT));

            
            MarginLayoutParams rightEdgeParams = (MarginLayoutParams) mAwesomeBarRightEdge.getLayoutParams();
            rightEdgeParams.rightMargin = itemBarParams.rightMargin + mActionItemBar.getWidth() - 100;
            mAwesomeBarRightEdge.requestLayout();
        }

        
        mAwesomeBarRightEdge.setVisibility(View.VISIBLE);

        return translation;
    }

    public void fromAwesomeBarSearch(String url) {
        
        
        if (url != null && url.length() > 0) {
            setTitle(url);
        }

        if (mActivity.isTablet() || Build.VERSION.SDK_INT < 11) {
            return;
        }

        AnimatorProxy proxy = null;

        
        
        
        
        
        if (!mAwesomeBar.isSelected()) {
            int translation = prepareAwesomeBarAnimation();

            proxy = AnimatorProxy.create(mAwesomeBarRightEdge);
            proxy.setTranslationX(translation);
            proxy = AnimatorProxy.create(mTabs);
            proxy.setTranslationX(translation);
            proxy = AnimatorProxy.create(mTabsCount);
            proxy.setTranslationX(translation);
            proxy = AnimatorProxy.create(mActionItemBar);
            proxy.setTranslationX(translation);

            if (mHasSoftMenuButton) {
                proxy = AnimatorProxy.create(mMenu);
                proxy.setTranslationX(translation);
            }
        }

        
        
        
        proxy = AnimatorProxy.create(mFavicon);
        proxy.setAlpha(1);
        proxy = AnimatorProxy.create(mSiteSecurity);
        proxy.setAlpha(1);
        proxy = AnimatorProxy.create(mTitle);
        proxy.setAlpha(1);
        proxy = AnimatorProxy.create(mForward);
        proxy.setAlpha(mForward.isEnabled() ? 1 : 0);
        proxy = AnimatorProxy.create(mBack);
        proxy.setAlpha(1);

        final PropertyAnimator contentAnimator = new PropertyAnimator(250);
        contentAnimator.setUseHardwareLayer(false);

        
        contentAnimator.attach(mAwesomeBarRightEdge,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mTabs,
                               PropertyAnimator.Property.TRANSLATION_X,
                               0);
        contentAnimator.attach(mTabsCount,
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
                mTabs.setVisibility(View.VISIBLE);
            }

            @Override
            public void onPropertyAnimationEnd() {
                
                mAwesomeBar.setSelected(false);

                
                MarginLayoutParams entryParams = (MarginLayoutParams) mAwesomeBarEntry.getLayoutParams();
                entryParams.rightMargin = mAwesomeBarEntryRightMargin;
                mAwesomeBarEntry.requestLayout();

                
                MarginLayoutParams barParams = (MarginLayoutParams) mAddressBarBg.getLayoutParams();
                barParams.rightMargin = mAddressBarBgRightMargin;
                mAddressBarBg.setCurveTowards(mAddressBarBgCurveTowards);

                
                
                
                
                if (mActionItemBar.getVisibility() == View.VISIBLE)
                    ((View) mAwesomeBar.getParent()).setLayoutParams(mAwesomeBarParams);

                
                mAwesomeBarRightEdge.setVisibility(View.INVISIBLE);

                PropertyAnimator buttonsAnimator = new PropertyAnimator(150);

                
                
                buttonsAnimator.attach(mReader,
                                       PropertyAnimator.Property.ALPHA,
                                       1);
                buttonsAnimator.attach(mStop,
                                       PropertyAnimator.Property.ALPHA,
                                       1);

                buttonsAnimator.start();

                mAnimatingEntry = false;
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
        
        if (mActivity.isTablet() || Build.VERSION.SDK_INT < 11) {
            mActivity.onSearchRequested();
            return;
        }

        if (mAnimatingEntry)
            return;

        final PropertyAnimator contentAnimator = new PropertyAnimator(250);
        contentAnimator.setUseHardwareLayer(false);

        int translation = prepareAwesomeBarAnimation();

        if (mActionItemBar.getVisibility() == View.VISIBLE) {
            contentAnimator.attach(mFavicon,
                                   PropertyAnimator.Property.ALPHA,
                                   0);
            contentAnimator.attach(mSiteSecurity,
                                   PropertyAnimator.Property.ALPHA,
                                   0);
            contentAnimator.attach(mTitle,
                                   PropertyAnimator.Property.ALPHA,
                                   0);
        }

        
        contentAnimator.attach(mForward,
                               PropertyAnimator.Property.ALPHA,
                               0);
        contentAnimator.attach(mBack,
                               PropertyAnimator.Property.ALPHA,
                               0);
        contentAnimator.attach(mReader,
                               PropertyAnimator.Property.ALPHA,
                               0);
        contentAnimator.attach(mStop,
                               PropertyAnimator.Property.ALPHA,
                               0);

        
        contentAnimator.attach(mAwesomeBarRightEdge,
                               PropertyAnimator.Property.TRANSLATION_X,
                               translation);
        contentAnimator.attach(mTabs,
                               PropertyAnimator.Property.TRANSLATION_X,
                               translation);
        contentAnimator.attach(mTabsCount,
                               PropertyAnimator.Property.TRANSLATION_X,
                               translation);
        contentAnimator.attach(mActionItemBar,
                               PropertyAnimator.Property.TRANSLATION_X,
                               translation);

        if (mHasSoftMenuButton)
            contentAnimator.attach(mMenu,
                                   PropertyAnimator.Property.TRANSLATION_X,
                                   translation);

        contentAnimator.setPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                mTabs.setVisibility(View.INVISIBLE);

                
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
        if (mCount > count) {
            mTabsCount.setInAnimation(mSlideDownIn);
            mTabsCount.setOutAnimation(mSlideDownOut);
        } else if (mCount < count) {
            mTabsCount.setInAnimation(mSlideUpIn);
            mTabsCount.setOutAnimation(mSlideUpOut);
        } else {
            return;
        }

        mTabsCount.setText(String.valueOf(count));
        mTabs.setContentDescription((count > 1) ?
                                    mActivity.getString(R.string.num_tabs, count) :
                                    mActivity.getString(R.string.one_tab));
        mCount = count;
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                GeckoTextView view = (GeckoTextView) mTabsCount.getCurrentView();
                view.setSelected(true);
            }
        }, mDuration);

        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                GeckoTextView view = (GeckoTextView) mTabsCount.getCurrentView();
                view.setSelected(false);
            }
        }, 2 * mDuration);
    }

    public void updateTabCount(int count) {
        mTabsCount.setCurrentText(String.valueOf(count));
        mTabs.setContentDescription((count > 1) ?
                                    mActivity.getString(R.string.num_tabs, count) :
                                    mActivity.getString(R.string.one_tab));
        mCount = count;
        updateTabs(mActivity.areTabsShown());
    }

    public void prepareTabsAnimation(PropertyAnimator animator, int width) {
        
        
        animator.attach(mAwesomeBarRightEdge,
                        PropertyAnimator.Property.TRANSLATION_X,
                        -width);

        animator.attach(mAwesomeBar,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mAddressBarBg,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mTabs,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        animator.attach(mTabsCount,
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
        mAwesomeBarRightEdge.setTranslationX(-width);
        mAwesomeBar.setTranslationX(width);
        mAddressBarBg.setTranslationX(width);
        mTabs.setTranslationX(width);
        mTabsCount.setTranslationX(width);
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
        adjustTabsAnimation(false);
    }

    public void updateTabs(boolean areTabsShown) {
        if (areTabsShown) {
            mTabs.getBackground().setLevel(TABS_EXPANDED);

            if (!mActivity.hasTabsSideBar()) {
                mTabs.setImageLevel(0);
                mTabsCount.setVisibility(View.GONE);
                mMenu.setImageLevel(TABS_EXPANDED);
                mMenu.getBackground().setLevel(TABS_EXPANDED);
            } else {
                mTabs.setImageLevel(TABS_EXPANDED);
            }
        } else {
            mTabs.setImageLevel(TABS_CONTRACTED);
            mTabs.getBackground().setLevel(TABS_CONTRACTED);

            if (!mActivity.hasTabsSideBar()) {
                mTabsCount.setVisibility(View.VISIBLE);
                mMenu.setImageLevel(TABS_CONTRACTED);
                mMenu.getBackground().setLevel(TABS_CONTRACTED);
            }
        }

        
        
        mTabs.requestLayout();
    }

    public void setIsSideBar(boolean isSideBar) {
        mTabs.setIsSideBar(isSideBar);

        mTabs.setImageResource(R.drawable.tabs_level);
        mTabs.setBackgroundResource(R.drawable.tabs_button);
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

        if (!mAnimateSiteSecurity) {
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

    private void setTitle(CharSequence title) {
        Tab tab = Tabs.getInstance().getSelectedTab();

        
        if (tab != null && tab.isEnteringReaderMode())
            return;

        
        
        
        if (tab != null && ("about:home".equals(title) ||
                            "about:privatebrowsing".equals(title)))
            title = null;

        mTitle.setText(title);
        mAwesomeBar.setContentDescription(title != null ? title : mTitle.getHint());
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
        mAwesomeBar.requestFocusFromTouch();
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

        mForwardAnim = new PropertyAnimator(FORWARD_ANIMATION_DURATION);
        final int width = mForward.getWidth()/2;

        mForwardAnim.setPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                if (!enabled) {
                    
                    
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mAddressBarView.getLayoutParams();
                    layoutParams.leftMargin = mAddressBarViewOffsetNoForward;
                    mAddressBarView.requestLayout();
                    
                    
                    
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (enabled) {
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mAddressBarView.getLayoutParams();
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
                layoutParams.leftMargin = mDefaultForwardMargin + (mForward.isEnabled() ? mForward.getWidth()/2 : 0);
                AnimatorProxy proxy = AnimatorProxy.create(mForward);
                proxy.setTranslationX(0);

                mAddressBarView.requestLayout();
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
                      -1*width);
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

            
            
            
            int startTrans = mAddressBarViewOffset - mAddressBarViewOffsetNoForward;
            AnimatorProxy proxy = AnimatorProxy.create(mTitle);
            proxy.setTranslationX(startTrans);
            proxy = AnimatorProxy.create(mFavicon);
            proxy.setTranslationX(startTrans);
            proxy = AnimatorProxy.create(mSiteSecurity);
            proxy.setTranslationX(startTrans);
        } else {
            anim.attach(mForward,
                      PropertyAnimator.Property.TRANSLATION_X,
                      width);
            anim.attach(mForward,
                      PropertyAnimator.Property.ALPHA,
                      1);
            anim.attach(mTitle,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mAddressBarViewOffset - mAddressBarViewOffsetNoForward);
            anim.attach(mFavicon,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mAddressBarViewOffset - mAddressBarViewOffsetNoForward);
            anim.attach(mSiteSecurity,
                      PropertyAnimator.Property.TRANSLATION_X,
                      mAddressBarViewOffset - mAddressBarViewOffsetNoForward);
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

            mAddressBarBg.setPrivateMode(tab.isPrivate());

            if (mAwesomeBar instanceof GeckoButton)
                ((GeckoButton) mAwesomeBar).setPrivateMode(tab.isPrivate());
            else if (mAwesomeBar instanceof GeckoRelativeLayout)
                ((GeckoRelativeLayout) mAwesomeBar).setPrivateMode(tab.isPrivate());

            mTabs.setPrivateMode(tab.isPrivate());
            mTitle.setPrivateMode(tab.isPrivate());
            mMenu.setPrivateMode(tab.isPrivate());

            if (mBack instanceof BackButton)
                ((BackButton) mBack).setPrivateMode(tab.isPrivate());

            if (mForward instanceof ForwardButton)
                ((ForwardButton) mForward).setPrivateMode(tab.isPrivate());
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

    public static class RightEdge extends GeckoFrameLayout {
        private BrowserApp mActivity;

        public RightEdge(Context context, AttributeSet attrs) {
            super(context, attrs);
            mActivity = (BrowserApp) context;
        }

        @Override
        public void onLightweightThemeChanged() {
            Drawable drawable = mActivity.getLightweightTheme().getDrawable(this);
            if (drawable == null)
                return;

            StateListDrawable stateList = new StateListDrawable();
            stateList.addState(new int[] { R.attr.state_private }, new ColorDrawable(mActivity.getResources().getColor(R.color.background_private)));
            stateList.addState(new int[] {}, drawable);

            int[] padding =  new int[] { getPaddingLeft(),
                                         getPaddingTop(),
                                         getPaddingRight(),
                                         getPaddingBottom()
                                       };
            setBackgroundDrawable(stateList);
            setPadding(padding[0], padding[1], padding[2], padding[3]);
        }

        @Override
        public void onLightweightThemeReset() {
            int[] padding =  new int[] { getPaddingLeft(),
                                         getPaddingTop(),
                                         getPaddingRight(),
                                         getPaddingBottom()
                                       };
            setBackgroundResource(R.drawable.address_bar_bg);
            setPadding(padding[0], padding[1], padding[2], padding[3]);
        }
    }
}
