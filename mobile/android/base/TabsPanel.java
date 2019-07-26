




package org.mozilla.gecko;

import org.mozilla.gecko.sync.setup.SyncAccounts;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class TabsPanel extends LinearLayout
                       implements GeckoPopupMenu.OnMenuItemClickListener,
                                  LightweightTheme.OnChangeListener {
    private static final String LOGTAG = "GeckoTabsPanel";

    public static enum Panel {
        LOCAL_TABS,
        REMOTE_TABS
    }

    public static interface PanelView {
        public ViewGroup getLayout();
        public void setTabsPanel(TabsPanel panel);
        public void show();
        public void hide();
    }

    public static interface TabsLayoutChangeListener {
        public void onTabsLayoutChange(int width, int height);
    }

    private Context mContext;
    private GeckoApp mActivity;
    private PanelView mPanel;
    private TabsPanelToolbar mToolbar;
    private TabsListContainer mListContainer;
    private TabsLayoutChangeListener mLayoutChangeListener;

    private ImageButton mMenuButton;
    private static ImageButton mRemoteTabs;
    private TextView mTitle;

    private Panel mCurrentPanel;
    private boolean mIsSideBar;
    private boolean mVisible;

    private GeckoPopupMenu mPopupMenu;
    private Menu mMenu;

    private static final int REMOTE_TABS_HIDDEN = 1;
    private static final int REMOTE_TABS_SHOWN = 2;

    public TabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mActivity = (GeckoApp) context;

        setOrientation(LinearLayout.VERTICAL);
        LayoutInflater.from(context).inflate(R.layout.tabs_panel, this);

        mCurrentPanel = Panel.LOCAL_TABS;
        mVisible = false;

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabsPanel);
        mIsSideBar = a.getBoolean(R.styleable.TabsPanel_sidebar, false);
        a.recycle();

        mToolbar = (TabsPanelToolbar) findViewById(R.id.toolbar);
        mListContainer = (TabsListContainer) findViewById(R.id.list_container);

        mPopupMenu = new GeckoPopupMenu(context);
        mPopupMenu.inflate(R.menu.tabs_menu);
        mPopupMenu.setOnMenuItemClickListener(this);
        mMenu = mPopupMenu.getMenu();

        initToolbar();
    }

    void initToolbar() {
        mTitle = (TextView) mToolbar.findViewById(R.id.title);
        ImageButton addTab = (ImageButton) mToolbar.findViewById(R.id.add_tab);
        addTab.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                mActivity.addTab();
                mActivity.autoHideTabs();
            }
        });

        mRemoteTabs = (ImageButton) mToolbar.findViewById(R.id.remote_tabs);
        mRemoteTabs.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                if (mRemoteTabs.getDrawable().getLevel() == REMOTE_TABS_SHOWN)
                    mActivity.showLocalTabs();
                else
                    mActivity.showRemoteTabs();
            }
        });

        mMenuButton = (ImageButton) mToolbar.findViewById(R.id.menu);
        mMenuButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View view) {
                TabsPanel.this.openTabsMenu();
            }
        });

        mPopupMenu.setAnchor(mMenuButton);
    }

    public void openTabsMenu() {
        if (mCurrentPanel == Panel.REMOTE_TABS)
            mMenu.findItem(R.id.close_all_tabs).setEnabled(false);
        else
            mMenu.findItem(R.id.close_all_tabs).setEnabled(true); 

        mPopupMenu.show();

        final Context context = mContext;
        new SyncAccounts.AccountsExistTask() {
            @Override
            protected void onPostExecute(Boolean result) {
                if (!result.booleanValue()) {
                    return;
                }
                TabsAccessor.areClientsAvailable(context, new TabsAccessor.OnClientsAvailableListener() {
                    @Override
                    public void areAvailable(boolean available) {
                        TabsPanel.this.enableRemoteTabs(available);
                    }
                });
            }
        }.execute(context);
    }

    public void enableRemoteTabs(boolean enable) {
        mMenu.findItem(R.id.synced_tabs).setEnabled(enable);
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.synced_tabs:
                show(Panel.REMOTE_TABS);
                return true;
            case R.id.close_all_tabs:
                for (Tab tab : Tabs.getInstance().getTabsInOrder()) {
                    Tabs.getInstance().closeTab(tab);
                }
                return true;
            case R.id.new_tab:
            case R.id.new_private_tab:
                hide();
            
            default:
                return mActivity.onOptionsItemSelected(item);
        }
    }  

    private static int getTabContainerHeight(View view) {
        Context context = view.getContext();

        int actionBarHeight = context.getResources().getDimensionPixelSize(R.dimen.browser_toolbar_height);
        int screenHeight = context.getResources().getDisplayMetrics().heightPixels;

        Rect windowRect = new Rect();
        view.getWindowVisibleDisplayFrame(windowRect);
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
        Drawable drawable = mActivity.getLightweightTheme().getDrawableWithAlpha(this, 255, 0);
        if (drawable == null)
            return;

        drawable.setAlpha(30);

        LayerDrawable layers = new LayerDrawable(new Drawable[] { mContext.getResources().getDrawable(R.drawable.tabs_tray_bg_repeat), drawable });
        setBackgroundDrawable(layers);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.tabs_tray_bg_repeat);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        onLightweightThemeChanged();
    }

    
    public static class TabsListContainer extends LinearLayout {
        private Context mContext;

        public TabsListContainer(Context context, AttributeSet attrs) {
            super(context, attrs);
            mContext = context;
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            if (!GeckoApp.mAppContext.hasTabsSideBar()) {
                int heightSpec = MeasureSpec.makeMeasureSpec(getTabContainerHeight(this), MeasureSpec.EXACTLY);
                super.onMeasure(widthMeasureSpec, heightSpec);
            } else {
                super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            }
        }
    }

    
    public static class TabsPanelToolbar extends RelativeLayout 
                                         implements LightweightTheme.OnChangeListener {
        private BrowserApp mActivity;

        public TabsPanelToolbar(Context context, AttributeSet attrs) {
            super(context, attrs);
            mActivity = (BrowserApp) context;

            setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT,
                                                          (int) context.getResources().getDimension(R.dimen.browser_toolbar_height)));

            LayoutInflater.from(context).inflate(R.layout.tabs_panel_toolbar_menu, this);
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
            Drawable drawable = mActivity.getLightweightTheme().getDrawableWithAlpha(this, 34);
            if (drawable == null)
                return;

            Resources resources = this.getContext().getResources();
            LayerDrawable layers = new LayerDrawable(new Drawable[] { resources.getDrawable(R.drawable.tabs_tray_bg_repeat), drawable }); 
            setBackgroundDrawable(layers);
        }

        @Override
        public void onLightweightThemeReset() {
            setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }

        @Override
        protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
            super.onLayout(changed, left, top, right, bottom);
            onLightweightThemeChanged();
        }
    }

    public void show(Panel panel) {
        if (mPanel != null) {
            
            mPanel.hide();
            mListContainer.removeAllViews();
        }

        final boolean showAnimation = !mVisible;
        mVisible = true;
        mCurrentPanel = panel;

        if (panel == Panel.LOCAL_TABS) {
            mPanel = new TabsTray(mContext, null);
            mTitle.setText("");
            mRemoteTabs.setImageLevel(REMOTE_TABS_HIDDEN);
        } else {
            mPanel = new RemoteTabs(mContext, null);
            mTitle.setText(R.string.remote_tabs);
            mRemoteTabs.setVisibility(View.VISIBLE);
            mRemoteTabs.setImageLevel(REMOTE_TABS_SHOWN);
        }

        mPanel.setTabsPanel(this);
        mPanel.show();
        mListContainer.addView(mPanel.getLayout());

        if (isSideBar()) {
            if (showAnimation)
                dispatchLayoutChange(getWidth(), getHeight());
        } else {
            int actionBarHeight = mContext.getResources().getDimensionPixelSize(R.dimen.browser_toolbar_height);
            int height = actionBarHeight + getTabContainerHeight(mListContainer);
            dispatchLayoutChange(getWidth(), height);
        }

        
        final Context context = mContext;
        new SyncAccounts.AccountsExistTask() {
            @Override
            protected void onPostExecute(Boolean result) {
                if (!result.booleanValue()) {
                    return;
                }
                TabsAccessor.areClientsAvailable(context, new TabsAccessor.OnClientsAvailableListener() {
                    @Override
                    public void areAvailable(boolean available) {
                        final int visibility = available ? View.VISIBLE : View.GONE;
                        mRemoteTabs.setVisibility(visibility);
                    }
                });
            }
        }.execute(context);
    }

    public void hide() {
        if (mVisible) {
            mVisible = false;
            mPopupMenu.dismiss();
            dispatchLayoutChange(0, 0);
        }
    }

    public void refresh() {
        mListContainer.forceLayout();

        int index = indexOfChild(mToolbar);
        removeViewAt(index);

        mToolbar = new TabsPanelToolbar(mContext, null);
        addView(mToolbar, index);
        initToolbar();

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

    public void setTabsLayoutChangeListener(TabsLayoutChangeListener listener) {
        mLayoutChangeListener = listener;
    }

    private void dispatchLayoutChange(int width, int height) {
        if (mLayoutChangeListener != null)
            mLayoutChangeListener.onTabsLayoutChange(width, height);
    }
}
