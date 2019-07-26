




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.home.HomeAdapter.OnAddPanelListener;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.PanelType;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.view.ViewPager;
import android.view.ViewGroup.LayoutParams;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.View;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

public class HomePager extends ViewPager {

    private static final int LOADER_ID_CONFIG = 0;

    private final Context mContext;
    private volatile boolean mLoaded;
    private Decor mDecor;
    private View mTabStrip;

    private final OnAddPanelListener mAddPanelListener;

    private final HomeConfig mConfig;
    private ConfigLoaderCallbacks mConfigLoaderCallbacks;

    private String mInitialPanelId;

    
    private final Drawable mOriginalBackground;

    
    
    static final String LIST_TAG_HISTORY = "history";
    static final String LIST_TAG_BOOKMARKS = "bookmarks";
    static final String LIST_TAG_READING_LIST = "reading_list";
    static final String LIST_TAG_TOP_SITES = "top_sites";
    static final String LIST_TAG_MOST_RECENT = "most_recent";
    static final String LIST_TAG_LAST_TABS = "last_tabs";
    static final String LIST_TAG_BROWSER_SEARCH = "browser_search";

    public interface OnUrlOpenListener {
        public enum Flags {
            ALLOW_SWITCH_TO_TAB,
            OPEN_WITH_INTENT
        }

        public void onUrlOpen(String url, EnumSet<Flags> flags);
    }

    public interface OnNewTabsListener {
        public void onNewTabs(String[] urls);
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

    static final String CAN_LOAD_ARG = "canLoad";
    static final String PANEL_CONFIG_ARG = "panelConfig";

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

            setOnPageChangeListener(new ViewPager.OnPageChangeListener() {
                @Override
                public void onPageSelected(int position) {
                    mDecor.onPageSelected(position);
                }

                @Override
                public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
                    mDecor.onPageScrolled(position, positionOffset, positionOffsetPixels);
                }

                @Override
                public void onPageScrollStateChanged(int state) { }
            });
        } else if (child instanceof HomePagerTabStrip) {
            mTabStrip = child;
        }

        super.addView(child, index, params);
    }

    




    public void show(LoaderManager lm, FragmentManager fm, String panelId, PropertyAnimator animator) {
        mLoaded = true;
        mInitialPanelId = panelId;

        
        final boolean shouldAnimate = (animator != null && Build.VERSION.SDK_INT >= 11);

        final HomeAdapter adapter = new HomeAdapter(mContext, fm);
        adapter.setOnAddPanelListener(mAddPanelListener);
        adapter.setCanLoadHint(!shouldAnimate);
        setAdapter(adapter);

        setVisibility(VISIBLE);

        
        
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

    


    public void hide() {
        mLoaded = false;
        setVisibility(GONE);
        setAdapter(null);
    }

    







    public boolean isVisible() {
        return mLoaded;
    }

    @Override
    public void setCurrentItem(int item, boolean smoothScroll) {
        super.setCurrentItem(item, smoothScroll);

        if (mDecor != null) {
            mDecor.onPageSelected(item);
        }
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            
            requestFocus();
        }

        return super.onInterceptTouchEvent(event);
    }

    private void updateUiFromPanelConfigs(List<PanelConfig> panelConfigs) {
        
        
        if (!mLoaded) {
            return;
        }

        if (mDecor != null) {
            mDecor.removeAllPagerViews();
        }

        final HomeAdapter adapter = (HomeAdapter) getAdapter();

        
        
        setAdapter(null);

        
        final List<PanelConfig> enabledPanels = new ArrayList<PanelConfig>();

        for (PanelConfig panelConfig : panelConfigs) {
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

        
        
        
        final int itemPosition = (mInitialPanelId == null) ? -1 : adapter.getItemPosition(mInitialPanelId);
        if (itemPosition > -1) {
            setCurrentItem(itemPosition, false);
            mInitialPanelId = null;
        } else {
            for (int i = 0; i < count; i++) {
                final PanelConfig panelConfig = enabledPanels.get(i);
                if (panelConfig.isDefault()) {
                    setCurrentItem(i, false);
                    break;
                }
            }
        }
    }

    private class ConfigLoaderCallbacks implements LoaderCallbacks<List<PanelConfig>> {
        @Override
        public Loader<List<PanelConfig>> onCreateLoader(int id, Bundle args) {
            return new HomeConfigLoader(mContext, mConfig);
        }

        @Override
        public void onLoadFinished(Loader<List<PanelConfig>> loader, List<PanelConfig> panelConfigs) {
            updateUiFromPanelConfigs(panelConfigs);
        }

        @Override
        public void onLoaderReset(Loader<List<PanelConfig>> loader) {
        }
    }
}
