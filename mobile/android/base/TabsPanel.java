




package org.mozilla.gecko;

import org.mozilla.gecko.widget.IconTabWidget;

import org.mozilla.gecko.widget.TwoWayView;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

public class TabsPanel extends LinearLayout
                       implements LightweightTheme.OnChangeListener,
                                  IconTabWidget.OnTabChangedListener {
    private static final String LOGTAG = "GeckoTabsPanel";

    public static enum Panel {
        NORMAL_TABS,
        PRIVATE_TABS,
        REMOTE_TABS
    }

    public static interface PanelView {
        public ViewGroup getLayout();
        public void setTabsPanel(TabsPanel panel);
        public void show();
        public void hide();
        public boolean shouldExpand();
    }

    public static interface TabsLayoutChangeListener {
        public void onTabsLayoutChange(int width, int height);
    }

    private Context mContext;
    private GeckoApp mActivity;
    private TabsListContainer mTabsContainer;
    private PanelView mPanel;
    private PanelView mPanelNormal;
    private PanelView mPanelPrivate;
    private PanelView mPanelRemote;
    private RelativeLayout mFooter;
    private TabsLayoutChangeListener mLayoutChangeListener;

    private IconTabWidget mTabWidget;
    private static ImageButton mAddTab;

    private Panel mCurrentPanel;
    private boolean mIsSideBar;
    private boolean mVisible;

    public TabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mActivity = (GeckoApp) context;

        setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                                                      LinearLayout.LayoutParams.FILL_PARENT));
        setOrientation(LinearLayout.VERTICAL);

        mCurrentPanel = Panel.NORMAL_TABS;
        mVisible = false;

        mIsSideBar = false;

        LayoutInflater.from(context).inflate(R.layout.tabs_panel, this);
        initialize();
    }

    private void initialize() {
        mTabsContainer = (TabsListContainer) findViewById(R.id.tabs_container);

        mPanelNormal = (TabsTray) findViewById(R.id.normal_tabs);
        mPanelNormal.setTabsPanel(this);

        mPanelPrivate = (TabsTray) findViewById(R.id.private_tabs);
        mPanelPrivate.setTabsPanel(this);

        mPanelRemote = (RemoteTabs) findViewById(R.id.synced_tabs);
        mPanelRemote.setTabsPanel(this);

        mFooter = (RelativeLayout) findViewById(R.id.tabs_panel_footer);

        mAddTab = (ImageButton) findViewById(R.id.add_tab);
        mAddTab.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                TabsPanel.this.addTab();
            }
        });

        mTabWidget = (IconTabWidget) findViewById(R.id.tab_widget);
        mTabWidget.addTab(R.drawable.tabs_normal);
        mTabWidget.addTab(R.drawable.tabs_private);
        mTabWidget.addTab(R.drawable.tabs_synced);
        mTabWidget.setTabSelectionListener(this);
    }

    public void addTab() {
        if (mCurrentPanel == Panel.NORMAL_TABS)
           mActivity.addTab();
        else
           mActivity.addPrivateTab();

        mActivity.autoHideTabs();
    }

    @Override
    public void onTabChanged(int index) {
        if (index == 0)
            show(Panel.NORMAL_TABS, false);
        else if (index == 1)
            show(Panel.PRIVATE_TABS, false);
        else
            show(Panel.REMOTE_TABS, false);
    }

    private static int getTabContainerHeight(TabsListContainer listContainer) {
        Context context = listContainer.getContext();

        PanelView panelView = listContainer.getCurrentPanelView();
        if (panelView != null && !panelView.shouldExpand()) {
            final View v = (View) panelView;
            final int sizeSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
            v.measure(sizeSpec, sizeSpec);
            return v.getMeasuredHeight();
        }

        int actionBarHeight = context.getResources().getDimensionPixelSize(R.dimen.browser_toolbar_height);
        int screenHeight = context.getResources().getDisplayMetrics().heightPixels;

        Rect windowRect = new Rect();
        listContainer.getWindowVisibleDisplayFrame(windowRect);
        int windowHeight = windowRect.bottom - windowRect.top;


        
        
        
        return (int) Math.max(screenHeight * 0.5f,
                              Math.min(windowHeight - 2.5f * actionBarHeight, windowHeight * 0.8f) - actionBarHeight);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        mActivity.getLightweightTheme().addListener(this);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mActivity.getLightweightTheme().removeListener(this);
    }
    
    @Override
    public void onLightweightThemeChanged() {
        int background = mActivity.getResources().getColor(R.color.background_tabs_light);
        LightweightThemeDrawable drawable = mActivity.getLightweightTheme().getColorDrawable(this, background, true);
        if (drawable == null)
            return;

        drawable.setAlpha(34, 0);
        setBackgroundDrawable(drawable);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundColor(getContext().getResources().getColor(R.color.background_tabs_light));
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        onLightweightThemeChanged();
    }

    
    public static class TabsListContainer extends FrameLayout {
        private Context mContext;

        public TabsListContainer(Context context, AttributeSet attrs) {
            super(context, attrs);
            mContext = context;
        }

        public PanelView getCurrentPanelView() {
            final int childCount = getChildCount();
            for (int i = 0; i < childCount; i++) {
                View child = getChildAt(i);
                if (!(child instanceof PanelView))
                    continue;

                if (child.getVisibility() == View.VISIBLE)
                    return (PanelView) child;
            }

            return null;
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            if (!GeckoApp.mAppContext.hasTabsSideBar()) {
                int heightSpec = MeasureSpec.makeMeasureSpec(getTabContainerHeight(TabsListContainer.this), MeasureSpec.EXACTLY);
                super.onMeasure(widthMeasureSpec, heightSpec);
            } else {
                super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            }
        }
    }

    
    public static class TabsPanelToolbar extends LinearLayout 
                                         implements LightweightTheme.OnChangeListener {
        private BrowserApp mActivity;

        public TabsPanelToolbar(Context context, AttributeSet attrs) {
            super(context, attrs);
            mActivity = (BrowserApp) context;

            setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                                                          (int) context.getResources().getDimension(R.dimen.browser_toolbar_height)));

            setOrientation(LinearLayout.HORIZONTAL);
        }

        @Override
        public void onAttachedToWindow() {
            super.onAttachedToWindow();
            mActivity.getLightweightTheme().addListener(this);
        }

        @Override
        public void onDetachedFromWindow() {
            super.onDetachedFromWindow();
            mActivity.getLightweightTheme().removeListener(this);
        }
    
        @Override
        public void onLightweightThemeChanged() {
            int background = mActivity.getResources().getColor(R.color.background_tabs_dark);
            LightweightThemeDrawable drawable = mActivity.getLightweightTheme().getColorDrawable(this, background);
            if (drawable == null)
                return;

            drawable.setAlpha(34, 34);
            setBackgroundDrawable(drawable);
        }

        @Override
        public void onLightweightThemeReset() {
            setBackgroundColor(getContext().getResources().getColor(R.color.background_tabs_dark));
        }

        @Override
        protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
            super.onLayout(changed, left, top, right, bottom);
            onLightweightThemeChanged();
        }
    }

    public void show(Panel panel) {
        show(panel, true);
    }

    public void show(Panel panel, boolean shouldResize) {
        if (!isShown())
            setVisibility(View.VISIBLE);

        if (mPanel != null) {
            
            mPanel.hide();
        }

        final boolean showAnimation = !mVisible;
        mVisible = true;
        mCurrentPanel = panel;

        int index = panel.ordinal();
        mTabWidget.setCurrentTab(index);

        if (index == 0) {
            mPanel = mPanelNormal;
        } else if (index == 1) {
            mPanel = mPanelPrivate;
        } else {
            mPanel = mPanelRemote;
        }

        mPanel.show();

        if (mCurrentPanel == Panel.REMOTE_TABS) {
            if (mFooter != null)
                mFooter.setVisibility(View.GONE);

            mAddTab.setVisibility(View.INVISIBLE);
        } else {
            if (mFooter != null)
                mFooter.setVisibility(View.VISIBLE);

            mAddTab.setVisibility(View.VISIBLE);
            mAddTab.setImageLevel(index);
        }

        if (shouldResize) {
            if (isSideBar()) {
                if (showAnimation)
                    dispatchLayoutChange(getWidth(), getHeight());
            } else {
                int actionBarHeight = mContext.getResources().getDimensionPixelSize(R.dimen.browser_toolbar_height);
                int height = actionBarHeight + getTabContainerHeight(mTabsContainer);
                dispatchLayoutChange(getWidth(), height);
            }
        }
    }

    public void hide() {
        if (mVisible) {
            mVisible = false;
            dispatchLayoutChange(0, 0);

            if (mPanel != null) {
                mPanel.hide();
                mPanel = null;
            }
        }
    }

    public void refresh() {
        removeAllViews();

        LayoutInflater.from(mContext).inflate(R.layout.tabs_panel, this);
        initialize();

        if (mVisible)
            show(mCurrentPanel);
    }

    public void autoHidePanel() {
        mActivity.autoHideTabs();
    }

    @Override
    public boolean isShown() {
        return mVisible;
    }

    public boolean isSideBar() {
        return mIsSideBar;
    }

    public void setIsSideBar(boolean isSideBar) {
        mIsSideBar = isSideBar;
    }

    public Panel getCurrentPanel() {
        return mCurrentPanel;
    }

    public void setTabsLayoutChangeListener(TabsLayoutChangeListener listener) {
        mLayoutChangeListener = listener;
    }

    private void dispatchLayoutChange(int width, int height) {
        if (mLayoutChangeListener != null)
            mLayoutChangeListener.onTabsLayoutChange(width, height);
    }
}
