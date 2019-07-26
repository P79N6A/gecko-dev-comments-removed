




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.AboutHome;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.EnumMap;
import java.util.EnumSet;

public class HomePager extends ViewPager {
    private final Context mContext;
    private volatile boolean mLoaded;

    
    private enum Page {
        BOOKMARKS
    }

    private EnumMap<Page, Fragment> mPages = new EnumMap<Page, Fragment>(Page.class);

    public HomePager(Context context) {
        super(context);
        mContext = context;
    }

    public HomePager(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    




    public void show(FragmentManager fm) {
        mLoaded = true;
        TabsAdapter adapter = new TabsAdapter(fm);

        
        adapter.addTab(Page.BOOKMARKS, BookmarksPage.class, null, getContext().getString(R.string.bookmarks_title));

        setAdapter(adapter);
        setVisibility(VISIBLE);
    }

    


    public void hide() {
        mLoaded = false;
        setVisibility(GONE);
        setAdapter(null);
    }

    







    public boolean isVisible() {
        return mLoaded;
    }

    


    public void updateAboutHome(final EnumSet<AboutHome.UpdateFlags> flags) {
        
    }

    


    public void setAboutHomeLastTabsVisibility(boolean visible) {
        
    }

    class TabsAdapter extends FragmentStatePagerAdapter {
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
        }

        public void addTab(Page page, Class<?> clss, Bundle args, String title) {
            TabInfo info = new TabInfo(page, clss, args, title);
            mTabs.add(info);
            notifyDataSetChanged();
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
    }
}
