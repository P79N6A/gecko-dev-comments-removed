




package org.mozilla.gecko;

import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.MenuPopup;
import org.mozilla.gecko.PageActionLayout;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.StringUtils;

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
import android.text.Editable;
import android.text.InputType;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
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
import android.view.animation.AnimationUtils;
import android.view.animation.AlphaAnimation;
import android.view.animation.Interpolator;
import android.view.animation.TranslateAnimation;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.ViewSwitcher;

import java.util.Arrays;
import java.util.List;

public class BrowserToolbar extends GeckoRelativeLayout
                            implements TextWatcher,
                                       AutocompleteHandler,
                                       Tabs.OnTabsChangedListener,
                                       GeckoMenu.ActionItemBarPresenter,
                                       Animation.AnimationListener,
                                       GeckoEventListener {
    private static final String LOGTAG = "GeckoToolbar";
    public static final String PREF_TITLEBAR_MODE = "browser.chrome.titlebarMode";

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

    private LayoutParams mAwesomeBarParams;
    private View mUrlDisplayContainer;
    private View mUrlEditContainer;
    private CustomEditText mUrlEditText;
    private View mUrlBarEntry;
    private ImageView mUrlBarRightEdge;
    private BrowserToolbarBackground mUrlBarBackground;
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
    public ImageButton mGo;
    public PageActionLayout mPageActionLayout;
    private Animation mProgressSpinner;
    private TabCounter mTabsCounter;
    private ImageView mShadow;
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

    final private BrowserApp mActivity;
    private boolean mHasSoftMenuButton;

    private boolean mShowSiteSecurity;
    private boolean mShowReader;
    private boolean mSpinnerVisible;

    private boolean mDelayRestartInput;
    
    private String mAutoCompleteResult = "";
    
    private String mAutoCompletePrefix = null;

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

        mIsEditing = false;
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
        mBlockedColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_blockedtext));
        mDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext));
        mPrivateDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext_private));

        registerEventListener("Reader:Click");
        registerEventListener("Reader:LongClick");

        mShowSiteSecurity = false;
        mShowReader = false;

        mAnimatingEntry = false;

        mUrlBarBackground = (BrowserToolbarBackground) findViewById(R.id.url_bar_bg);
        mUrlBarViewOffset = res.getDimensionPixelSize(R.dimen.url_bar_offset_left);
        mDefaultForwardMargin = res.getDimensionPixelSize(R.dimen.forward_default_offset);
        mUrlDisplayContainer = findViewById(R.id.url_display_container);
        mUrlBarEntry = findViewById(R.id.url_bar_entry);

        mUrlEditContainer = findViewById(R.id.url_edit_container);
        mUrlEditText = (CustomEditText) findViewById(R.id.url_edit_text);

        
        mUrlBarRightEdge = (ImageView) findViewById(R.id.url_bar_right_edge);
        if (mUrlBarRightEdge != null) {
            mUrlBarRightEdge.getDrawable().setLevel(6000);
        }

        mTitle = (GeckoTextView) findViewById(R.id.url_bar_title);
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

        mProgressSpinner = AnimationUtils.loadAnimation(mActivity, R.anim.progress_spinner);

        mStop = (ImageButton) findViewById(R.id.stop);
        mShadow = (ImageView) findViewById(R.id.shadow);
        mPageActionLayout = (PageActionLayout) findViewById(R.id.page_action_layout);

        if (Build.VERSION.SDK_INT >= 16) {
            mShadow.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        }

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
                    if (!tab.getFeedsEnabled()) {
                        menu.findItem(R.id.subscribe).setVisible(false);
                    }
                } else {
                    
                    menu.findItem(R.id.copyurl).setVisible(false);
                    menu.findItem(R.id.share).setVisible(false);
                    menu.findItem(R.id.add_to_launcher).setVisible(false);
                    menu.findItem(R.id.subscribe).setVisible(false);
                }

                menu.findItem(R.id.share).setVisible(!GeckoProfile.get(getContext()).inGuestMode());
            }
        });

        mUrlEditText.addTextChangedListener(this);

        mUrlEditText.setOnKeyPreImeListener(new CustomEditText.OnKeyPreImeListener() {
            @Override
            public boolean onKeyPreIme(View v, int keyCode, KeyEvent event) {
                
                if (event.getAction() != KeyEvent.ACTION_DOWN)
                    return false;

                if (keyCode == KeyEvent.KEYCODE_ENTER) {
                    
                    
                    Editable content = mUrlEditText.getText();
                    if (!hasCompositionString(content)) {
                        if (mCommitListener != null) {
                            mCommitListener.onCommit();
                        }
                        return true;
                    }
                }

                if (keyCode == KeyEvent.KEYCODE_BACK) {
                    clearFocus();
                    return true;
                }

                return false;
            }
        });

        mUrlEditText.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER || GamepadUtils.isActionKey(event)) {
                    if (event.getAction() != KeyEvent.ACTION_DOWN)
                        return true;

                    if (mCommitListener != null) {
                        mCommitListener.onCommit();
                    }
                    return true;
                } else if (GamepadUtils.isBackKey(event)) {
                    if (mDismissListener != null) {
                        mDismissListener.onDismiss();
                    }
                    return true;
                }

                return false;
            }
        });

        mUrlEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (v == null) {
                    return;
                }

                setSelected(hasFocus);
                if (hasFocus) {
                    return;
                }

                InputMethodManager imm = (InputMethodManager) mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
                try {
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                } catch (NullPointerException e) {
                    Log.e(LOGTAG, "InputMethodManagerService, why are you throwing"
                                  + " a NullPointerException? See bug 782096", e);
                }
            }
        });

        mUrlEditText.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (Build.VERSION.SDK_INT >= 11) {
                    CustomEditText text = (CustomEditText) v;

                    if (text.getSelectionStart() == text.getSelectionEnd())
                        return false;

                    return false;
                }

                return false;
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

        mGo = (ImageButton) findViewById(R.id.go);
        mGo.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mCommitListener != null) {
                    mCommitListener.onCommit();
                }
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
            final int prevSelStart = mUrlEditText.getSelectionStart();
            final int prevSelEnd = mUrlEditText.getSelectionEnd();

            
            
            mUrlEditText.dispatchKeyEvent(event);

            final int curSelStart = mUrlEditText.getSelectionStart();
            final int curSelEnd = mUrlEditText.getSelectionEnd();

            if (prevSelStart != curSelStart || prevSelEnd != curSelEnd) {
                mUrlEditText.requestFocusFromTouch();

                
                mUrlEditText.setSelection(curSelStart, curSelEnd);
            }

            return true;
        }

        return false;
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
                    setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
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
            case LOADED:
                if (Tabs.getInstance().isSelectedTab(tab)) {
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
                    setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
                }
                break;
        }
    }

    
    
    @Override
    public void onAutocomplete(final String result) {
        final String text = mUrlEditText.getText().toString();

        if (result == null) {
            mAutoCompleteResult = "";
            return;
        }

        if (!result.startsWith(text) || text.equals(result)) {
            return;
        }

        mAutoCompleteResult = result;
        mUrlEditText.getText().append(result.substring(text.length()));
        mUrlEditText.setSelection(text.length(), result.length());
    }

    @Override
    public void afterTextChanged(final Editable s) {
        final String text = s.toString();
        boolean useHandler = false;
        boolean reuseAutocomplete = false;
        if (!hasCompositionString(s) && !StringUtils.isSearchQuery(text, false)) {
            useHandler = true;

            
            
            if (mAutoCompletePrefix != null && (mAutoCompletePrefix.length() >= text.length())) {
                useHandler = false;
            } else if (mAutoCompleteResult != null && mAutoCompleteResult.startsWith(text)) {
                
                
                useHandler = false;
                reuseAutocomplete = true;
            }
        }

        
        if (TextUtils.isEmpty(mAutoCompleteResult) || !mAutoCompleteResult.equals(text)) {
            if (isEditing() && mFilterListener != null) {
                mFilterListener.onFilter(text, useHandler ? this : null);
            }
            mAutoCompletePrefix = text;

            if (reuseAutocomplete) {
                onAutocomplete(mAutoCompleteResult);
            }
        }

        
        
        if (!hasCompositionString(s) ||
            InputMethods.isGestureKeyboard(mUrlEditText.getContext())) {
            updateGoButton(text);
        }
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count,
                                  int after) {
        
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before,
                              int count) {
        
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

    private int getUrlBarEntryTranslation() {
        return getWidth() - mUrlBarEntry.getRight();
    }

    private int getUrlBarCurveTranslation() {
        return getWidth() - mTabs.getLeft();
    }

    private static boolean hasCompositionString(Editable content) {
        Object[] spans = content.getSpans(0, content.length(), Object.class);
        if (spans != null) {
            for (Object span : spans) {
                if ((content.getSpanFlags(span) & Spanned.SPAN_COMPOSING) != 0) {
                    
                    return true;
                }
            }
        }
        return false;
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
        
        
        
        if (visible) {
            mFavicon.setImageResource(R.drawable.progress_spinner);
            
            if (!mSpinnerVisible) {
                setPageActionVisibility(true);
                mFavicon.setAnimation(mProgressSpinner);
                mProgressSpinner.start();
                mSpinnerVisible = true;
            }
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber start");
        } else {
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null)
                setFavicon(selectedTab.getFavicon());

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

        boolean inReaderMode = false;
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null)
            inReaderMode = ReaderModeUtils.isAboutReader(tab.getURL());

        mPageActionLayout.setVisibility(!isLoading ? View.VISIBLE : View.GONE);
        
        
        
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

    public void setShadowVisibility(boolean visible) {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null) {
            return;
        }

        String url = tab.getURL();

        if ((mShadow.getVisibility() == View.VISIBLE) != visible) {
            mShadow.setVisibility(visible ? View.VISIBLE : View.GONE);
        }
    }

    public void onEditSuggestion(String suggestion) {
        if (!isEditing()) {
            return;
        }

        mUrlEditText.setText(suggestion);
        mUrlEditText.setSelection(mUrlEditText.getText().length());
        mUrlEditText.requestFocus();

        showSoftInput();
    }

    public void setTitle(CharSequence title) {
        mTitle.setText(title);
        setContentDescription(title != null ? title : mTitle.getHint());
    }

    
    private void updateTitle() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        
        if (tab == null || tab.isEnteringReaderMode()) {
            return;
        }

        String url = tab.getURL();

        if (!isEditing()) {
            mUrlEditText.setText(url);
        }

        
        if ("about:home".equals(url) || "about:privatebrowsing".equals(url)) {
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

        CharSequence title = StringUtils.stripCommonSubdomains(StringUtils.stripScheme(url));

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
    }

    public void setOnDismissListener(OnDismissListener listener) {
        mDismissListener = listener;
    }

    public void setOnFilterListener(OnFilterListener listener) {
        mFilterListener = listener;
    }

    public void setOnStartEditingListener(OnStartEditingListener listener) {
        mStartEditingListener = listener;
    }

    public void setOnStopEditingListener(OnStopEditingListener listener) {
        mStopEditingListener = listener;
    }

    private void showSoftInput() {
        InputMethodManager imm =
               (InputMethodManager) mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(mUrlEditText, InputMethodManager.SHOW_IMPLICIT);
    }

    private void showUrlEditContainer() {
        setUrlEditContainerVisibility(true, null);
    }

    private void showUrlEditContainer(PropertyAnimator animator) {
        setUrlEditContainerVisibility(true, animator);
    }

    private void hideUrlEditContainer() {
        setUrlEditContainerVisibility(false, null);
    }

    private void hideUrlEditContainer(PropertyAnimator animator) {
        setUrlEditContainerVisibility(false, animator);
    }

    private void setUrlEditContainerVisibility(final boolean showEditContainer, PropertyAnimator animator) {
        final View viewToShow = (showEditContainer ? mUrlEditContainer : mUrlDisplayContainer);
        final View viewToHide = (showEditContainer ? mUrlDisplayContainer : mUrlEditContainer);

        if (animator == null) {
            viewToHide.setVisibility(View.GONE);
            viewToShow.setVisibility(View.VISIBLE);

            if (showEditContainer) {
                mUrlEditText.requestFocus();
                showSoftInput();
            }

            return;
        }

        ViewHelper.setAlpha(viewToShow, 0.0f);
        animator.attach(viewToShow,
                        PropertyAnimator.Property.ALPHA,
                        1.0f);

        animator.attach(viewToHide,
                        PropertyAnimator.Property.ALPHA,
                        0.0f);

        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                viewToShow.setVisibility(View.VISIBLE);

                if (showEditContainer) {
                    ViewHelper.setAlpha(mGo, 0.0f);
                    mUrlEditText.requestFocus();
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                viewToHide.setVisibility(View.GONE);
                ViewHelper.setAlpha(viewToHide, 1.0f);

                if (showEditContainer) {
                    ViewHelper.setAlpha(mGo, 1.0f);
                    showSoftInput();
                }
            }
        });
    }

    



    public boolean isEditing() {
        return mIsEditing;
    }

    public void startEditing(String url, PropertyAnimator animator) {
        if (isEditing()) {
            return;
        }

        mUrlEditText.setText(url != null ? url : "");
        mIsEditing = true;

        if (mStartEditingListener != null) {
            mStartEditingListener.onStartEditing();
        }

        final int entryTranslation = getUrlBarEntryTranslation();
        final int curveTranslation = getUrlBarCurveTranslation();

        
        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            showUrlEditContainer();

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

        showUrlEditContainer(animator);

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
        final String url = mUrlEditText.getText().toString();
        if (!isEditing()) {
            return url;
        }
        mIsEditing = false;

        if (mStopEditingListener != null) {
            mStopEditingListener.onStopEditing();
        }

        if (HardwareUtils.isTablet() || Build.VERSION.SDK_INT < 11) {
            hideUrlEditContainer();

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

        hideUrlEditContainer(contentAnimator);

        contentAnimator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
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

    private void updateGoButton(String text) {
        if (text.length() == 0) {
            mGo.setVisibility(View.GONE);
            return;
        }

        mGo.setVisibility(View.VISIBLE);

        int imageResource = R.drawable.ic_url_bar_go;
        String contentDescription = mActivity.getString(R.string.go);
        int imeAction = EditorInfo.IME_ACTION_GO;

        int actionBits = mUrlEditText.getImeOptions() & EditorInfo.IME_MASK_ACTION;
        if (StringUtils.isSearchQuery(text, actionBits == EditorInfo.IME_ACTION_SEARCH)) {
            imageResource = R.drawable.ic_url_bar_search;
            contentDescription = mActivity.getString(R.string.search);
            imeAction = EditorInfo.IME_ACTION_SEARCH;
        }

        InputMethodManager imm = InputMethods.getInputMethodManager(mUrlEditText.getContext());
        if (imm == null) {
            return;
        }
        boolean restartInput = false;
        if (actionBits != imeAction) {
            int optionBits = mUrlEditText.getImeOptions() & ~EditorInfo.IME_MASK_ACTION;
            mUrlEditText.setImeOptions(optionBits | imeAction);

            mDelayRestartInput = (imeAction == EditorInfo.IME_ACTION_GO) &&
                                 (InputMethods.shouldDelayUrlBarUpdate(mUrlEditText.getContext()));
            if (!mDelayRestartInput) {
                restartInput = true;
            }
        } else if (mDelayRestartInput) {
            
            
            
            mDelayRestartInput = false;
            restartInput = true;
        }
        if (restartInput) {
            updateKeyboardInputType();
            imm.restartInput(mUrlEditText);
            mGo.setImageResource(imageResource);
            mGo.setContentDescription(contentDescription);
        }
    }

    private void updateKeyboardInputType() {
        
        
        
        
        String text = mUrlEditText.getText().toString();
        int currentInputType = mUrlEditText.getInputType();
        int newInputType = StringUtils.isSearchQuery(text, false)
                           ? (currentInputType & ~InputType.TYPE_TEXT_VARIATION_URI) 
                           : (currentInputType | InputType.TYPE_TEXT_VARIATION_URI); 
        if (newInputType != currentInputType) {
            mUrlEditText.setRawInputType(newInputType);
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

        mForwardAnim.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                if (!enabled) {
                    
                    
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mUrlDisplayContainer.getLayoutParams();
                    layoutParams.leftMargin = 0;

                    
                    layoutParams = (ViewGroup.MarginLayoutParams)mUrlEditContainer.getLayoutParams();
                    layoutParams.leftMargin = 0;

                    requestLayout();
                    
                    
                    
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (enabled) {
                    ViewGroup.MarginLayoutParams layoutParams =
                        (ViewGroup.MarginLayoutParams)mUrlDisplayContainer.getLayoutParams();
                    layoutParams.leftMargin = mUrlBarViewOffset;

                    layoutParams = (ViewGroup.MarginLayoutParams)mUrlEditContainer.getLayoutParams();
                    layoutParams.leftMargin = mUrlBarViewOffset;

                    ViewHelper.setTranslationX(mTitle, 0);
                    ViewHelper.setTranslationX(mFavicon, 0);
                    ViewHelper.setTranslationX(mSiteSecurity, 0);
                }

                ViewGroup.MarginLayoutParams layoutParams =
                    (ViewGroup.MarginLayoutParams)mForward.getLayoutParams();
                layoutParams.leftMargin = mDefaultForwardMargin + (mForward.isEnabled() ? width : 0);
                ViewHelper.setTranslationX(mForward, 0);

                requestLayout();
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
            setPageActionVisibility(mStop.getVisibility() == View.VISIBLE);
            updateBackButton(tab.canDoBack());
            updateForwardButton(tab.canDoForward());

            final boolean isPrivate = tab.isPrivate();
            mUrlBarBackground.setPrivateMode(isPrivate);
            setPrivateMode(isPrivate);
            mTabs.setPrivateMode(isPrivate);
            mTitle.setPrivateMode(isPrivate);
            mMenu.setPrivateMode(isPrivate);
            mMenuIcon.setPrivateMode(isPrivate);
            mUrlEditText.setPrivateMode(isPrivate);

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
}
