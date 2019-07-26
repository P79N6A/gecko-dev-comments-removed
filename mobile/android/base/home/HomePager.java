




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.home.HomeAdapter.OnAddPageListener;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.ViewPager;
import android.view.ViewGroup.LayoutParams;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.View;

import java.util.EnumSet;

public class HomePager extends ViewPager {

    private final Context mContext;
    private volatile boolean mLoaded;
    private Decor mDecor;

    private final OnAddPageListener mAddPageListener;

    
    @RobocopTarget
    public enum Page {
        HISTORY,
        TOP_SITES,
        BOOKMARKS,
        READING_LIST
    }

    
    
    static final String LIST_TAG_HISTORY = "history";
    static final String LIST_TAG_BOOKMARKS = "bookmarks";
    static final String LIST_TAG_READING_LIST = "reading_list";
    static final String LIST_TAG_TOP_SITES = "top_sites";
    static final String LIST_TAG_MOST_RECENT = "most_recent";
    static final String LIST_TAG_LAST_TABS = "last_tabs";
    static final String LIST_TAG_BROWSER_SEARCH = "browser_search";

    public interface OnUrlOpenListener {
        public enum Flags {
            ALLOW_SWITCH_TO_TAB
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

    public HomePager(Context context) {
        this(context, null);
    }

    public HomePager(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        mAddPageListener = new OnAddPageListener() {
            @Override
            public void onAddPage(String title) {
                if (mDecor != null) {
                    mDecor.onAddPagerView(title);
                }
            }
        };

        
        
        setOffscreenPageLimit(3);

        
        
        
        
        
        setFocusableInTouchMode(true);
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        if (child instanceof Decor) {
            ((ViewPager.LayoutParams) params).isDecor = true;
            mDecor = (Decor) child;

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
        }

        super.addView(child, index, params);
    }

    public void redisplay(FragmentManager fm) {
        final HomeAdapter adapter = (HomeAdapter) getAdapter();
        final Page currentPage = adapter.getPageAtPosition(getCurrentItem());

        show(fm, currentPage, null);
    }

    




    public void show(FragmentManager fm, Page page, PropertyAnimator animator) {
        mLoaded = true;

        if (mDecor != null) {
            mDecor.removeAllPagerViews();
        }

        final HomeAdapter adapter = new HomeAdapter(mContext, fm);
        adapter.setOnAddPageListener(mAddPageListener);

        
        final boolean shouldAnimate = (animator != null && Build.VERSION.SDK_INT >= 11);

        adapter.addPage(Page.TOP_SITES, TopSitesPage.class, new Bundle(),
                getContext().getString(R.string.home_top_sites_title));
        adapter.addPage(Page.BOOKMARKS, BookmarksPage.class, new Bundle(),
                getContext().getString(R.string.bookmarks_title));

        
        
        if (!HardwareUtils.isLowMemoryPlatform()) {
            adapter.addPage(Page.READING_LIST, ReadingListPage.class, new Bundle(),
                    getContext().getString(R.string.reading_list_title));
        }

        
        
        adapter.addPage(HardwareUtils.isTablet() ? -1 : 0,
                Page.HISTORY, HistoryPage.class, new Bundle(),
                getContext().getString(R.string.home_history_title));

        adapter.setCanLoadHint(!shouldAnimate);

        setAdapter(adapter);

        setCurrentItem(adapter.getItemPosition(page), false);
        setVisibility(VISIBLE);

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
}
