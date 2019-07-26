




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.ViewGroup.LayoutParams;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.View;

import java.util.ArrayList;
import java.util.EnumMap;
import java.util.EnumSet;

public class HomePager extends ViewPager {
    
    public static final String SUBPAGE_TAG = "home_pager_subpage";

    private final Context mContext;
    private volatile boolean mLoaded;
    private Decor mDecor;

    
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

    private EnumMap<Page, Fragment> mPages = new EnumMap<Page, Fragment>(Page.class);

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

        
        
        setOffscreenPageLimit(3);

        
        
        
        
        
        setFocusableInTouchMode(true);
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        if (child instanceof Decor) {
            ((ViewPager.LayoutParams) params).isDecor = true;
            mDecor = (Decor) child;
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

    




    public void show(FragmentManager fm, Page page, PropertyAnimator animator) {
        mLoaded = true;
        final TabsAdapter adapter = new TabsAdapter(fm);

        
        final boolean shouldAnimate = (animator != null && Build.VERSION.SDK_INT >= 11);

        adapter.addTab(Page.TOP_SITES, TopSitesPage.class, new Bundle(),
                getContext().getString(R.string.home_top_sites_title));
        adapter.addTab(Page.BOOKMARKS, BookmarksPage.class, new Bundle(),
                getContext().getString(R.string.bookmarks_title));

        
        
        if (!HardwareUtils.isLowMemoryPlatform()) {
            adapter.addTab(Page.READING_LIST, ReadingListPage.class, new Bundle(),
                    getContext().getString(R.string.reading_list_title));
        }

        
        
        adapter.addTab(HardwareUtils.isTablet() ? -1 : 0,
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

    class TabsAdapter extends FragmentStatePagerAdapter
                      implements OnTitleClickListener {
        private final ArrayList<TabInfo> mTabs = new ArrayList<TabInfo>();

        final class TabInfo {
            private final Page page;
            private final Class<?> clss;
            private final Bundle args;
            private final String title;

            TabInfo(Page page, Class<?> clss, Bundle args, String title) {
                this.page = page;
                this.clss = clss;
                this.args = args;
                this.title = title;
            }
        }

        public TabsAdapter(FragmentManager fm) {
            super(fm);

            if (mDecor != null) {
                mDecor.removeAllPagerViews();
                mDecor.setOnTitleClickListener(this);
            }
        }

        public void addTab(Page page, Class<?> clss, Bundle args, String title) {
            addTab(-1, page, clss, args, title);
        }

        public void addTab(int index, Page page, Class<?> clss, Bundle args, String title) {
            TabInfo info = new TabInfo(page, clss, args, title);

            if (index >= 0) {
                mTabs.add(index, info);
            } else {
                mTabs.add(info);
            }

            notifyDataSetChanged();

            if (mDecor != null) {
                mDecor.onAddPagerView(title);
            }
        }

        @Override
        public void onTitleClicked(int index) {
            setCurrentItem(index, true);
        }

        public int getItemPosition(Page page) {
            for (int i = 0; i < mTabs.size(); i++) {
                TabInfo info = mTabs.get(i);
                if (info.page == page) {
                    return i;
                }
            }

            return -1;
        }

        @Override
        public int getCount() {
            return mTabs.size();
        }

        @Override
        public Fragment getItem(int position) {
            TabInfo info = mTabs.get(position);
            return Fragment.instantiate(mContext, info.clss.getName(), info.args);
        }

        @Override
        public CharSequence getPageTitle(int position) {
            TabInfo info = mTabs.get(position);
            return info.title.toUpperCase();
        }

        @Override
        public Object instantiateItem(ViewGroup container, int position) {
            Fragment fragment = (Fragment) super.instantiateItem(container, position);

            mPages.put(mTabs.get(position).page, fragment);

            return fragment;
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            super.destroyItem(container, position, object);

            mPages.remove(mTabs.get(position).page);
        }

        public void setCanLoadHint(boolean canLoadHint) {
            
            for (TabInfo info : mTabs) {
                info.args.putBoolean(CAN_LOAD_ARG, canLoadHint);
            }

            
            for (Fragment page : mPages.values()) {
                final HomeFragment homePage = (HomeFragment) page;
                homePage.setCanLoadHint(canLoadHint);
            }
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
