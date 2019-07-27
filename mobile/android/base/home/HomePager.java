




package org.mozilla.gecko.home;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.home.HomeAdapter.OnAddPanelListener;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

public class HomePager extends ViewPager {
    private static final int LOADER_ID_CONFIG = 0;

    private final Context mContext;
    private volatile boolean mVisible;
    private Decor mDecor;
    private View mTabStrip;
    private HomeBanner mHomeBanner;
    private int mDefaultPageIndex = -1;

    private final OnAddPanelListener mAddPanelListener;

    private final HomeConfig mConfig;
    private ConfigLoaderCallbacks mConfigLoaderCallbacks;

    private String mInitialPanelId;

    
    private final Drawable mOriginalBackground;

    
    private TelemetryContract.Session mCurrentPanelSession;
    private String mCurrentPanelSessionSuffix;

    
    private LoadState mLoadState;

    
    private OnPanelChangeListener mPanelChangedListener;

    
    
    public static final String LIST_TAG_HISTORY = "history";
    public static final String LIST_TAG_BOOKMARKS = "bookmarks";
    public static final String LIST_TAG_READING_LIST = "reading_list";
    public static final String LIST_TAG_TOP_SITES = "top_sites";
    public static final String LIST_TAG_RECENT_TABS = "recent_tabs";
    public static final String LIST_TAG_BROWSER_SEARCH = "browser_search";
    public static final String LIST_TAG_REMOTE_TABS = "remote_tabs";

    public interface OnUrlOpenListener {
        public enum Flags {
            ALLOW_SWITCH_TO_TAB,
            OPEN_WITH_INTENT
        }

        public void onUrlOpen(String url, EnumSet<Flags> flags);
    }

    public interface OnNewTabsListener {
        public void onNewTabs(List<String> urls);
    }

    


    public interface OnPanelChangeListener {
        




        public void onPanelSelected(String panelId);
    }

    interface OnTitleClickListener {
        public void onTitleClicked(int index);
    }

    


    interface Decor {
        public void onAddPagerView(String title);
        public void removeAllPagerViews();
        public void onPageSelected(int position);
        public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels);
        public void setOnTitleClickListener(OnTitleClickListener onTitleClickListener);
    }

    


    private enum LoadState {
        UNLOADED,
        LOADING,
        LOADED
    }

    public static final String CAN_LOAD_ARG = "canLoad";
    public static final String PANEL_CONFIG_ARG = "panelConfig";

    public HomePager(Context context) {
        this(context, null);
    }

    public HomePager(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mConfig = HomeConfig.getDefault(mContext);
        mConfigLoaderCallbacks = new ConfigLoaderCallbacks();

        mAddPanelListener = new OnAddPanelListener() {
            @Override
            public void onAddPanel(String title) {
                if (mDecor != null) {
                    mDecor.onAddPagerView(title);
                }
            }
        };

        
        
        setOffscreenPageLimit(3);

        
        
        
        
        
        setFocusableInTouchMode(true);

        mOriginalBackground = getBackground();
        setOnPageChangeListener(new PageChangeListener());

        mLoadState = LoadState.UNLOADED;
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        if (child instanceof Decor) {
            ((ViewPager.LayoutParams) params).isDecor = true;
            mDecor = (Decor) child;
            mTabStrip = child;

            mDecor.setOnTitleClickListener(new OnTitleClickListener() {
                @Override
                public void onTitleClicked(int index) {
                    setCurrentItem(index, true);
                }
            });
        } else if (child instanceof HomePagerTabStrip) {
            mTabStrip = child;
        }

        super.addView(child, index, params);
    }

    




    public void load(LoaderManager lm, FragmentManager fm, String panelId, PropertyAnimator animator) {
        mLoadState = LoadState.LOADING;

        mVisible = true;
        mInitialPanelId = panelId;

        
        if (mHomeBanner != null) {
            mHomeBanner.update();
        }

        
        final boolean shouldAnimate = Versions.feature11Plus && animator != null;

        final HomeAdapter adapter = new HomeAdapter(mContext, fm);
        adapter.setOnAddPanelListener(mAddPanelListener);
        adapter.setCanLoadHint(!shouldAnimate);
        setAdapter(adapter);

        
        
        mTabStrip.setVisibility(View.INVISIBLE);

        
        lm.initLoader(LOADER_ID_CONFIG, null, mConfigLoaderCallbacks);

        if (shouldAnimate) {
            animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
                @Override
                public void onPropertyAnimationStart() {
                    setLayerType(View.LAYER_TYPE_HARDWARE, null);
                }

                @Override
                public void onPropertyAnimationEnd() {
                    setLayerType(View.LAYER_TYPE_NONE, null);
                    adapter.setCanLoadHint(true);
                }
            });

            ViewHelper.setAlpha(this, 0.0f);

            animator.attach(this,
                            PropertyAnimator.Property.ALPHA,
                            1.0f);
        }
    }

    


    public void unload() {
        mVisible = false;
        setAdapter(null);
        mLoadState = LoadState.UNLOADED;

        
        stopCurrentPanelTelemetrySession();
    }

    







    public boolean isVisible() {
        return mVisible;
    }

    @Override
    public void setCurrentItem(int item, boolean smoothScroll) {
        super.setCurrentItem(item, smoothScroll);

        if (mDecor != null) {
            mDecor.onPageSelected(item);
        }

        if (mHomeBanner != null) {
            mHomeBanner.setActive(item == mDefaultPageIndex);
        }
    }

    







    public void showPanel(String panelId) {
        if (!mVisible) {
            return;
        }

        switch (mLoadState) {
            case LOADING:
                mInitialPanelId = panelId;
                break;

            case LOADED:
                int position = mDefaultPageIndex;
                if (panelId != null) {
                    position = ((HomeAdapter) getAdapter()).getItemPosition(panelId);
                }

                if (position > -1) {
                    setCurrentItem(position);
                }
                break;

            default:
                
        }
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            
            requestFocus();
        }

        return super.onInterceptTouchEvent(event);
    }

    public void setBanner(HomeBanner banner) {
        mHomeBanner = banner;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (mHomeBanner != null) {
            mHomeBanner.handleHomeTouch(event);
        }

        return super.dispatchTouchEvent(event);
    }

    public void onToolbarFocusChange(boolean hasFocus) {
        if (mHomeBanner == null) {
            return;
        }

        
        final boolean active = !hasFocus && getCurrentItem() == mDefaultPageIndex;
        mHomeBanner.setActive(active);
    }

    private void updateUiFromConfigState(HomeConfig.State configState) {
        
        
        if (!mVisible) {
            return;
        }

        if (mDecor != null) {
            mDecor.removeAllPagerViews();
        }

        final HomeAdapter adapter = (HomeAdapter) getAdapter();

        
        
        
        final boolean canLoadHint = adapter.getCanLoadHint();
        adapter.setCanLoadHint(false);

        
        
        setAdapter(null);

        
        final List<PanelConfig> enabledPanels = new ArrayList<PanelConfig>();

        for (PanelConfig panelConfig : configState) {
            if (!panelConfig.isDisabled()) {
                enabledPanels.add(panelConfig);
            }
        }

        
        adapter.update(enabledPanels);

        final int count = enabledPanels.size();
        if (count == 0) {
            
            setBackgroundResource(R.drawable.home_pager_empty_state);
            
            mTabStrip.setVisibility(View.INVISIBLE);
        } else {
            mTabStrip.setVisibility(View.VISIBLE);
            
            setBackgroundDrawable(mOriginalBackground);
        }

        
        
        setAdapter(adapter);

        if (count == 0) {
            mDefaultPageIndex = -1;

            
            if (mHomeBanner != null) {
                mHomeBanner.setActive(false);
            }
        } else {
            for (int i = 0; i < count; i++) {
                if (enabledPanels.get(i).isDefault()) {
                    mDefaultPageIndex = i;
                    break;
                }
            }

            
            
            final int itemPosition = (mInitialPanelId == null) ? -1 : adapter.getItemPosition(mInitialPanelId);
            if (itemPosition > -1) {
                setCurrentItem(itemPosition, false);
                mInitialPanelId = null;
            } else {
                setCurrentItem(mDefaultPageIndex, false);
            }
        }

        
        
        if (canLoadHint) {
            
            
            
            ThreadUtils.getUiHandler().post(new Runnable() {
                @Override
                public void run() {
                    adapter.setCanLoadHint(true);
                }
            });
        }
    }

    public void setOnPanelChangeListener(OnPanelChangeListener listener) {
       mPanelChangedListener = listener;
    }

    




    private void notifyPanelSelected(int position) {
        if (mDecor != null) {
            mDecor.onPageSelected(position);
        }

        if (mPanelChangedListener != null) {
            final String panelId = ((HomeAdapter) getAdapter()).getPanelIdAtPosition(position);
            mPanelChangedListener.onPanelSelected(panelId);
        }
    }

    private class ConfigLoaderCallbacks implements LoaderCallbacks<HomeConfig.State> {
        @Override
        public Loader<HomeConfig.State> onCreateLoader(int id, Bundle args) {
            return new HomeConfigLoader(mContext, mConfig);
        }

        @Override
        public void onLoadFinished(Loader<HomeConfig.State> loader, HomeConfig.State configState) {
            mLoadState = LoadState.LOADED;
            updateUiFromConfigState(configState);
        }

        @Override
        public void onLoaderReset(Loader<HomeConfig.State> loader) {
            mLoadState = LoadState.UNLOADED;
        }
    }

    private class PageChangeListener implements ViewPager.OnPageChangeListener {
        @Override
        public void onPageSelected(int position) {
            notifyPanelSelected(position);

            if (mHomeBanner != null) {
                mHomeBanner.setActive(position == mDefaultPageIndex);
            }

            
            final String newPanelId = ((HomeAdapter) getAdapter()).getPanelIdAtPosition(position);
            startNewPanelTelemetrySession(newPanelId);
        }

        @Override
        public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
            if (mDecor != null) {
                mDecor.onPageScrolled(position, positionOffset, positionOffsetPixels);
            }

            if (mHomeBanner != null) {
                mHomeBanner.setScrollingPages(positionOffsetPixels != 0);
            }
        }

        @Override
        public void onPageScrollStateChanged(int state) { }
    }

    






    private void startNewPanelTelemetrySession(String panelId) {
        
        stopCurrentPanelTelemetrySession();

        mCurrentPanelSession = TelemetryContract.Session.HOME_PANEL;
        mCurrentPanelSessionSuffix = panelId;
        Telemetry.startUISession(mCurrentPanelSession, mCurrentPanelSessionSuffix);
    }

    


    private void stopCurrentPanelTelemetrySession() {
        if (mCurrentPanelSession != null) {
            Telemetry.stopUISession(mCurrentPanelSession, mCurrentPanelSessionSuffix);
            mCurrentPanelSession = null;
            mCurrentPanelSessionSuffix = null;
        }
    }
}
