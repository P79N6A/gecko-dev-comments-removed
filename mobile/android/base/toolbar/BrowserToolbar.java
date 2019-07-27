




package org.mozilla.gecko.toolbar;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoApplication;
import org.mozilla.gecko.LightweightTheme;
import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.PropertyAnimationListener;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.MenuPopup;
import org.mozilla.gecko.toolbar.ToolbarDisplayLayout.OnStopListener;
import org.mozilla.gecko.toolbar.ToolbarDisplayLayout.OnTitleChangeListener;
import org.mozilla.gecko.toolbar.ToolbarDisplayLayout.UpdateFlags;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.MenuUtils;
import org.mozilla.gecko.widget.ThemedImageButton;
import org.mozilla.gecko.widget.ThemedImageView;
import org.mozilla.gecko.widget.ThemedRelativeLayout;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.PopupWindow;


















public abstract class BrowserToolbar extends ThemedRelativeLayout
                                     implements Tabs.OnTabsChangedListener,
                                                GeckoMenu.ActionItemBarPresenter,
                                                GeckoEventListener {
    private static final String LOGTAG = "GeckoToolbar";

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

    protected enum UIMode {
        EDIT,
        DISPLAY
    }

    private final boolean isNewTablet;

    protected final ToolbarDisplayLayout urlDisplayLayout;
    protected final ToolbarEditLayout urlEditLayout;
    protected final View urlBarEntry;
    protected boolean isSwitchingTabs;
    protected final ThemedImageButton tabsButton;

    private ToolbarProgressView progressBar;
    protected final TabCounter tabsCounter;
    protected final ThemedImageButton menuButton;
    protected final ThemedImageView menuIcon;
    private MenuPopup menuPopup;
    protected final List<View> focusOrder;

    protected final ThemedImageView editCancel;

    private OnActivateListener activateListener;
    private OnFocusChangeListener focusChangeListener;
    private OnStartEditingListener startEditingListener;
    private OnStopEditingListener stopEditingListener;

    private final BrowserApp activity;
    private boolean hasSoftMenuButton;

    protected UIMode uiMode;

    private final Paint shadowPaint;
    private final int shadowSize;

    private static final Interpolator buttonsInterpolator = new AccelerateInterpolator();

    private final LightweightTheme theme;
    private final ToolbarPrefs prefs;

    public abstract boolean isAnimating();

    protected abstract void updateNavigationButtons(Tab tab);

    protected abstract void triggerStartEditingTransition(PropertyAnimator animator);
    protected abstract void triggerStopEditingTransition();

    public static BrowserToolbar create(final Context context, final AttributeSet attrs) {
        final BrowserToolbar toolbar;
        if (NewTabletUI.isEnabled(context)) {
            toolbar = new BrowserToolbarNewTablet(context, attrs);
        } else if (HardwareUtils.isTablet()) {
            toolbar = new BrowserToolbarTablet(context, attrs);
        } else if (Versions.preHC) {
            toolbar = new BrowserToolbarPreHC(context, attrs);
        } else {
            toolbar = new BrowserToolbarPhone(context, attrs);
        }
        return toolbar;
    }

    protected BrowserToolbar(final Context context, final AttributeSet attrs) {
        super(context, attrs);
        setWillNotDraw(false);

        isNewTablet = NewTabletUI.isEnabled(context);
        theme = ((GeckoApplication) context.getApplicationContext()).getLightweightTheme();

        
        activity = (BrowserApp) context;

        
        
        if (!isNewTablet) {
            LayoutInflater.from(context).inflate(R.layout.browser_toolbar, this);
        } else {
            LayoutInflater.from(context).inflate(R.layout.new_tablet_browser_toolbar, this);
        }

        Tabs.registerOnTabsChangedListener(this);
        isSwitchingTabs = true;

        EventDispatcher.getInstance().registerGeckoThreadListener(this,
            "Reader:Click",
            "Reader:LongClick");

        urlDisplayLayout = (ToolbarDisplayLayout) findViewById(R.id.display_layout);
        urlBarEntry = findViewById(R.id.url_bar_entry);
        urlEditLayout = (ToolbarEditLayout) findViewById(R.id.edit_layout);

        tabsButton = (ThemedImageButton) findViewById(R.id.tabs);
        tabsCounter = (TabCounter) findViewById(R.id.tabs_counter);
        if (Versions.feature11Plus) {
            tabsCounter.setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        }

        menuButton = (ThemedImageButton) findViewById(R.id.menu);
        menuIcon = (ThemedImageView) findViewById(R.id.menu_icon);
        hasSoftMenuButton = !HardwareUtils.hasMenuButton();

        editCancel = (ThemedImageView) findViewById(R.id.edit_cancel);

        
        focusOrder = new ArrayList<View>();

        final Resources res = getResources();
        shadowSize = res.getDimensionPixelSize(R.dimen.browser_toolbar_shadow_size);

        shadowPaint = new Paint();
        shadowPaint.setColor(res.getColor(R.color.url_bar_shadow));
        shadowPaint.setStrokeWidth(0.0f);

        setUIMode(UIMode.DISPLAY);

        prefs = new ToolbarPrefs();
        urlDisplayLayout.setToolbarPrefs(prefs);
        urlEditLayout.setToolbarPrefs(prefs);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        prefs.open();

        setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (activateListener != null) {
                    activateListener.onActivate();
                }
            }
        });

        setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                
                if (isEditing()) {
                    return;
                }

                
                

                MenuInflater inflater = activity.getMenuInflater();
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
                        menu.findItem(R.id.add_to_launcher).setVisible(false);
                    }

                    MenuUtils.safeSetVisible(menu, R.id.subscribe, tab.hasFeeds());
                    MenuUtils.safeSetVisible(menu, R.id.add_search_engine, tab.hasOpenSearch());
                } else {
                    
                    menu.findItem(R.id.copyurl).setVisible(false);
                    menu.findItem(R.id.add_to_launcher).setVisible(false);
                    MenuUtils.safeSetVisible(menu, R.id.subscribe, false);
                    MenuUtils.safeSetVisible(menu, R.id.add_search_engine, false);
                }
            }
        });

        urlDisplayLayout.setOnStopListener(new OnStopListener() {
            @Override
            public Tab onStop() {
                final Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    tab.doStop();
                    return tab;
                }

                return null;
            }
        });

        urlDisplayLayout.setOnTitleChangeListener(new OnTitleChangeListener() {
            @Override
            public void onTitleChange(CharSequence title) {
                final String contentDescription;
                if (title != null) {
                    contentDescription = title.toString();
                } else {
                    contentDescription = activity.getString(R.string.url_bar_default_text);
                }

                
                
                setContentDescription(contentDescription);
            }
        });

        urlEditLayout.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                
                setSelected(hasFocus);
                if (focusChangeListener != null) {
                    focusChangeListener.onFocusChange(v, hasFocus);
                }
            }
        });

        tabsButton.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleTabs();
            }
        });
        tabsButton.setImageLevel(0);

        editCancel.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                
                
                if (!isAnimating()) {
                    Telemetry.sendUIEvent(TelemetryContract.Event.CANCEL,
                                          TelemetryContract.Method.ACTIONBAR,
                                          getResources().getResourceEntryName(editCancel.getId()));
                    cancelEdit();
                }
            }
        });

        if (hasSoftMenuButton) {
            menuButton.setVisibility(View.VISIBLE);
            menuIcon.setVisibility(View.VISIBLE);

            menuButton.setOnClickListener(new Button.OnClickListener() {
                @Override
                public void onClick(View view) {
                    activity.openOptionsMenu();
                }
            });
        }
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        prefs.close();
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        final int height = getHeight();
        canvas.drawRect(0, height - shadowSize, getWidth(), height, shadowPaint);
    }

    public void setProgressBar(ToolbarProgressView progressBar) {
        this.progressBar = progressBar;
    }

    public void refresh() {
        urlDisplayLayout.dismissSiteIdentityPopup();
    }

    public boolean onBackPressed() {
        
        
        if (isEditing() && !isAnimating()) {
            Telemetry.sendUIEvent(TelemetryContract.Event.CANCEL,
                                  TelemetryContract.Method.BACK);
            cancelEdit();
            return true;
        }

        return urlDisplayLayout.dismissSiteIdentityPopup();
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
                    activity.refreshToolbarHeight();
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
                urlDisplayLayout.dismissSiteIdentityPopup();
                updateTabCount(tabs.getDisplayCount());
                isSwitchingTabs = true;
                break;
        }

        if (tabs.isSelectedTab(tab)) {
            final EnumSet<UpdateFlags> flags = EnumSet.noneOf(UpdateFlags.class);

            
            switch (msg) {
                case START:
                    updateProgressVisibility(tab, Tab.LOAD_PROGRESS_INIT);
                    
                case ADDED:
                case LOCATION_CHANGE:
                case LOAD_ERROR:
                case LOADED:
                case STOP:
                    flags.add(UpdateFlags.PROGRESS);
                    if (progressBar.getVisibility() == View.VISIBLE) {
                        progressBar.animateProgress(tab.getLoadProgress());
                    }
                    break;

                case SELECTED:
                    flags.add(UpdateFlags.PROGRESS);
                    updateProgressVisibility();
                    break;
            }

            switch (msg) {
                case STOP:
                    
                    
                    flags.add(UpdateFlags.TITLE);
                    
                case START:
                case CLOSED:
                case ADDED:
                    updateNavigationButtons(tab);
                    break;

                case SELECTED:
                    flags.add(UpdateFlags.PRIVATE_MODE);
                    setPrivateMode(tab.isPrivate());
                    
                case LOAD_ERROR:
                    flags.add(UpdateFlags.TITLE);
                    
                case LOCATION_CHANGE:
                    
                    
                    flags.add(UpdateFlags.FAVICON);
                    flags.add(UpdateFlags.SITE_IDENTITY);

                    updateNavigationButtons(tab);
                    break;

                case TITLE:
                    flags.add(UpdateFlags.TITLE);
                    break;

                case FAVICON:
                    flags.add(UpdateFlags.FAVICON);
                    break;

                case SECURITY_CHANGE:
                    flags.add(UpdateFlags.SITE_IDENTITY);
                    break;
            }

            if (!flags.isEmpty()) {
                updateDisplayLayout(tab, flags);
            }
        }

        switch (msg) {
            case SELECTED:
            case LOAD_ERROR:
            case LOCATION_CHANGE:
                isSwitchingTabs = false;
        }
    }

    private void updateProgressVisibility() {
        final Tab selectedTab = Tabs.getInstance().getSelectedTab();
        updateProgressVisibility(selectedTab, selectedTab.getLoadProgress());
    }

    private void updateProgressVisibility(Tab selectedTab, int progress) {
        if (!isEditing() && selectedTab.getState() == Tab.STATE_LOADING) {
            progressBar.setProgress(progress);
            progressBar.setVisibility(View.VISIBLE);
        } else {
            progressBar.setVisibility(View.GONE);
        }
    }

    private boolean isVisible() {
        return ViewHelper.getTranslationY(this) == 0;
    }

    @Override
    public void setNextFocusDownId(int nextId) {
        super.setNextFocusDownId(nextId);
        tabsButton.setNextFocusDownId(nextId);
        urlDisplayLayout.setNextFocusDownId(nextId);
        menuButton.setNextFocusDownId(nextId);
    }

    private void addTab() {
        activity.addTab();
    }

    private void toggleTabs() {
        if (activity.areTabsShown()) {
            if (activity.hasTabsSideBar())
                activity.hideTabs();
        } else {
            
            InputMethodManager imm =
                    (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(tabsButton.getWindowToken(), 0);

            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                if (!tab.isPrivate())
                    activity.showNormalTabs();
                else
                    activity.showPrivateTabs();
            }
        }
    }

    protected void updateTabCountAndAnimate(final int count) {
        
        if (!isVisible()) {
            updateTabCount(count);
            return;
        }

        
        
        
        
        if (!isEditing() || HardwareUtils.isTablet()) {
            tabsCounter.setCount(count);

            tabsButton.setContentDescription((count > 1) ?
                                             activity.getString(R.string.num_tabs, count) :
                                             activity.getString(R.string.one_tab));
        }
    }

    private void updateTabCount(int count) {
        
        
        
        
        if (isEditing() && !HardwareUtils.isTablet()) {
            return;
        }

        
        if (isVisible() && ViewHelper.getAlpha(tabsCounter) != 0 && !isEditing()) {
            tabsCounter.setCountWithAnimation(count);
        } else {
            tabsCounter.setCount(count);
        }

        
        tabsButton.setContentDescription((count > 1) ?
                                         activity.getString(R.string.num_tabs, count) :
                                         activity.getString(R.string.one_tab));
    }

    private void updateDisplayLayout(Tab tab, EnumSet<UpdateFlags> flags) {
        if (isSwitchingTabs) {
            flags.add(UpdateFlags.DISABLE_ANIMATIONS);
        }

        urlDisplayLayout.updateFromTab(tab, flags);

        if (flags.contains(UpdateFlags.TITLE)) {
            if (!isEditing()) {
                urlEditLayout.setText(tab.getURL());
            }
        }

        if (flags.contains(UpdateFlags.PROGRESS)) {
            updateFocusOrder();
        }
    }

    private void updateFocusOrder() {
        if (focusOrder.size() == 0) {
            throw new IllegalStateException("Expected focusOrder to be initialized in subclass");
        }

        View prevView = null;

        
        
        boolean needsNewFocus = false;

        for (View view : focusOrder) {
            if (view.getVisibility() != View.VISIBLE || !view.isEnabled()) {
                if (view.hasFocus()) {
                    needsNewFocus = true;
                }
                continue;
            }

            if (view.getId() == R.id.menu_items) {
                final LinearLayout actionItemBar = (LinearLayout) view;
                final int childCount = actionItemBar.getChildCount();
                for (int child = 0; child < childCount; child++) {
                    View childView = actionItemBar.getChildAt(child);
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

        urlEditLayout.onEditSuggestion(suggestion);
    }

    public void setTitle(CharSequence title) {
        urlDisplayLayout.setTitle(title);
    }

    public void prepareTabsAnimation(PropertyAnimator animator, boolean tabsAreShown) {
        if (!tabsAreShown) {
            PropertyAnimator buttonsAnimator =
                    new PropertyAnimator(animator.getDuration(), buttonsInterpolator);

            buttonsAnimator.attach(tabsCounter,
                                   PropertyAnimator.Property.ALPHA,
                                   1.0f);

            if (hasSoftMenuButton && !HardwareUtils.isTablet()) {
                buttonsAnimator.attach(menuIcon,
                                       PropertyAnimator.Property.ALPHA,
                                       1.0f);
            }

            buttonsAnimator.start();

            return;
        }

        ViewHelper.setAlpha(tabsCounter, 0.0f);

        if (hasSoftMenuButton && !HardwareUtils.isTablet()) {
            ViewHelper.setAlpha(menuIcon, 0.0f);
        }
    }

    public void finishTabsAnimation(boolean tabsAreShown) {
        if (tabsAreShown) {
            return;
        }

        PropertyAnimator animator = new PropertyAnimator(150);

        animator.attach(tabsCounter,
                        PropertyAnimator.Property.ALPHA,
                        1.0f);

        if (hasSoftMenuButton && !HardwareUtils.isTablet()) {
            animator.attach(menuIcon,
                            PropertyAnimator.Property.ALPHA,
                            1.0f);
        }

        animator.start();
    }

    public void setOnActivateListener(OnActivateListener listener) {
        activateListener = listener;
    }

    public void setOnCommitListener(OnCommitListener listener) {
        urlEditLayout.setOnCommitListener(listener);
    }

    public void setOnDismissListener(OnDismissListener listener) {
        urlEditLayout.setOnDismissListener(listener);
    }

    public void setOnFilterListener(OnFilterListener listener) {
        urlEditLayout.setOnFilterListener(listener);
    }

    public void setOnFocusChangeListener(OnFocusChangeListener listener) {
        focusChangeListener = listener;
    }

    public void setOnStartEditingListener(OnStartEditingListener listener) {
        startEditingListener = listener;
    }

    public void setOnStopEditingListener(OnStopEditingListener listener) {
        stopEditingListener = listener;
    }

    protected void showUrlEditLayout() {
        setUrlEditLayoutVisibility(true, null);
    }

    protected void showUrlEditLayout(final PropertyAnimator animator) {
        setUrlEditLayoutVisibility(true, animator);
    }

    protected void hideUrlEditLayout() {
        setUrlEditLayoutVisibility(false, null);
    }

    protected void hideUrlEditLayout(final PropertyAnimator animator) {
        setUrlEditLayoutVisibility(false, animator);
    }

    private void setUrlEditLayoutVisibility(final boolean showEditLayout, PropertyAnimator animator) {
        if (showEditLayout) {
            urlEditLayout.prepareShowAnimation(animator);
        }

        if (animator == null) {
            final View viewToShow = (showEditLayout ? urlEditLayout : urlDisplayLayout);
            final View viewToHide = (showEditLayout ? urlDisplayLayout : urlEditLayout);

            viewToHide.setVisibility(View.GONE);
            viewToShow.setVisibility(View.VISIBLE);

            final int cancelVisibility = (showEditLayout ? View.VISIBLE : View.INVISIBLE);
            setCancelVisibility(cancelVisibility);
            return;
        }

        animator.addPropertyAnimationListener(new PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                if (!showEditLayout) {
                    urlEditLayout.setVisibility(View.GONE);
                    urlDisplayLayout.setVisibility(View.VISIBLE);

                    setCancelVisibility(View.INVISIBLE);
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                if (showEditLayout) {
                    urlDisplayLayout.setVisibility(View.GONE);
                    urlEditLayout.setVisibility(View.VISIBLE);

                    setCancelVisibility(View.VISIBLE);
                }
            }
        });
    }

    private void setCancelVisibility(final int visibility) {
        
        if (!isNewTablet) {
            editCancel.setVisibility(visibility);
        }
    }

    private void setUIMode(final UIMode uiMode) {
        this.uiMode = uiMode;
        urlEditLayout.setEnabled(uiMode == UIMode.EDIT);
    }

    



    public boolean isEditing() {
        return (uiMode == UIMode.EDIT);
    }

    public void startEditing(String url, PropertyAnimator animator) {
        if (isEditing()) {
            return;
        }

        urlEditLayout.setText(url != null ? url : "");

        setUIMode(UIMode.EDIT);

        updateProgressVisibility();

        if (startEditingListener != null) {
            startEditingListener.onStartEditing();
        }

        triggerStartEditingTransition(animator);
    }

    




    public String cancelEdit() {
        Telemetry.stopUISession(TelemetryContract.Session.AWESOMESCREEN);
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
        final String url = urlEditLayout.getText();
        if (!isEditing()) {
            return url;
        }
        setUIMode(UIMode.DISPLAY);

        if (stopEditingListener != null) {
            stopEditingListener.onStopEditing();
        }

        updateProgressVisibility();
        triggerStopEditingTransition();

        return url;
    }

    @Override
    public void setPrivateMode(boolean isPrivate) {
        super.setPrivateMode(isPrivate);

        tabsButton.setPrivateMode(isPrivate);
        menuButton.setPrivateMode(isPrivate);
        menuIcon.setPrivateMode(isPrivate);
        editCancel.setPrivateMode(isPrivate);
        urlEditLayout.setPrivateMode(isPrivate);
    }

    public void show() {
        setVisibility(View.VISIBLE);
    }

    public void hide() {
        setVisibility(View.GONE);
    }

    public View getDoorHangerAnchor() {
        return urlDisplayLayout.getDoorHangerAnchor();
    }

    public void onDestroy() {
        Tabs.unregisterOnTabsChangedListener(this);

        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
            "Reader:Click",
            "Reader:LongClick");
    }

    public boolean openOptionsMenu() {
        if (!hasSoftMenuButton) {
            return false;
        }

        
        if (menuPopup == null) {
            View panel = activity.getMenuPanel();
            menuPopup = new MenuPopup(activity);
            menuPopup.setPanelView(panel);

            menuPopup.setOnDismissListener(new PopupWindow.OnDismissListener() {
                @Override
                public void onDismiss() {
                    activity.onOptionsMenuClosed(null);
                }
            });
        }

        GeckoAppShell.getGeckoInterface().invalidateOptionsMenu();
        if (!menuPopup.isShowing()) {
            menuPopup.showAsDropDown(menuButton);
        }

        return true;
    }

    public boolean closeOptionsMenu() {
        if (!hasSoftMenuButton) {
            return false;
        }

        if (menuPopup != null && menuPopup.isShowing()) {
            menuPopup.dismiss();
        }

        return true;
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
        Drawable drawable = theme.getDrawable(this);
        if (drawable == null)
            return;

        StateListDrawable stateList = new StateListDrawable();
        stateList.addState(PRIVATE_STATE_SET, getColorDrawable(R.color.background_private));
        stateList.addState(EMPTY_STATE_SET, drawable);

        setBackgroundDrawable(stateList);

        editCancel.onLightweightThemeChanged();
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.url_bar_bg);
        editCancel.onLightweightThemeReset();
    }
}
