




package org.mozilla.gecko;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.net.Uri;
import android.support.v4.view.ViewPager;
import android.support.v4.view.PagerAdapter;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.TabHost;
import android.widget.TabWidget;

public class AwesomeBarTabs extends TabHost {
    private static final String LOGTAG = "GeckoAwesomeBarTabs";

    private Context mContext;
    private boolean mInflated;
    private LayoutInflater mInflater;
    private OnUrlOpenListener mUrlOpenListener;
    private View.OnTouchListener mListTouchListener;
    private boolean mSearching = false;
    private String mTarget;
    private Background mBackground;
    private ViewPager mViewPager;
    private AwesomePagerAdapter mPagerAdapter;
    
    private AwesomeBarTab mTabs[];

    
    
    private static final int MAX_RESULTS = 100;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
        public void onSearch(String engine, String text);
        public void onEditSuggestion(String suggestion);
    }

    private class AwesomePagerAdapter extends PagerAdapter {
        public AwesomePagerAdapter() {
            super();
        }

        public Object instantiateItem(ViewGroup group, int index) {
            AwesomeBarTab tab = mTabs[index];
            group.addView(tab.getView());
            return tab;
        }

        public void destroyItem(ViewGroup group, int index, Object obj) {
            AwesomeBarTab tab = (AwesomeBarTab)obj;
            group.removeView(tab.getView());
        }

        public int getCount() {
            if (mSearching)
                return 1;
            return mTabs.length;
        }

        public boolean isViewFromObject(View view, Object object) {
            return getAwesomeBarTabForView(view) == object;
        }
    }

    private AwesomeBarTab getCurrentAwesomeBarTab() {
        int index = mViewPager.getCurrentItem();
        return mTabs[index];
    }

    public AwesomeBarTab getAwesomeBarTabForView(View view) {
        String tag = (String)view.getTag();
        return getAwesomeBarTabForTag(tag);
    }

    public AwesomeBarTab getAwesomeBarTabForTag(String tag) {
        for (AwesomeBarTab tab : mTabs) {
            if (tag.equals(tab.getTag())) {
                return tab;
            }
        }
        return null;
    }

    public boolean onBackPressed() {
        AwesomeBarTab tab = getCurrentAwesomeBarTab();
        if (tab == null)
             return false;
        return tab.onBackPressed();
    }

    public AwesomeBarTabs(Context context, AttributeSet attrs) {
        super(context, attrs);

        Log.d(LOGTAG, "Creating AwesomeBarTabs");

        mContext = context;
        mInflated = false;
        mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        
        
        
        if (mInflated)
            return;

        mInflated = true;

        
        
        setup();

        mListTouchListener = new View.OnTouchListener() {
            public boolean onTouch(View view, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN)
                    hideSoftInput(view);
                return false;
            }
        };

        mBackground = (Background) findViewById(R.id.awesomebar_background);

        mTabs = new AwesomeBarTab[] {
            new AllPagesTab(mContext),
            new BookmarksTab(mContext),
            new HistoryTab(mContext)
        };

        final TabWidget tabWidget = (TabWidget) findViewById(android.R.id.tabs);
        
        tabWidget.setStripEnabled(false);

        mViewPager = (ViewPager) findViewById(R.id.tabviewpager);
        mPagerAdapter = new AwesomePagerAdapter();
        mViewPager.setAdapter(mPagerAdapter);
        mViewPager.setCurrentItem(0);
        mViewPager.setOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            public void onPageScrollStateChanged(int state) { }
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) { }
            public void onPageSelected(int position) {
                tabWidget.setCurrentTab(position);
                styleSelectedTab();
             }
         });

        for (int i = 0; i < mTabs.length; i++) {
            addAwesomeTab(mTabs[i].getTag(),
                          mTabs[i].getTitleStringId(),
                          i);
        }

        tabWidget.setCurrentTab(0);

        styleSelectedTab();

        
        filter("");
    }

    private void styleSelectedTab() {
        int selIndex = mViewPager.getCurrentItem();
        TabWidget tabWidget = getTabWidget();

        for (int i = 0; i < tabWidget.getTabCount(); i++) {
            GeckoTextView view = (GeckoTextView) tabWidget.getChildTabViewAt(i);
            if (mTarget != null && mTarget.equals(AwesomeBar.Target.CURRENT_TAB.name())) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null && tab.isPrivate()) {
                    if (i == selIndex)
                        view.setPrivateMode(false);
                    else
                        view.setPrivateMode(true);
                }
            }

            if (i == selIndex)
                continue;

            if (i == (selIndex - 1))
                view.getBackground().setLevel(1);
            else if (i == (selIndex + 1))
                view.getBackground().setLevel(2);
            else
                view.getBackground().setLevel(0);
        }

        if (selIndex == 0)
            findViewById(R.id.tab_widget_left).getBackground().setLevel(1);
        else
            findViewById(R.id.tab_widget_left).getBackground().setLevel(0);

        if (selIndex == (tabWidget.getTabCount() - 1))
            findViewById(R.id.tab_widget_right).getBackground().setLevel(2);
        else
            findViewById(R.id.tab_widget_right).getBackground().setLevel(0);
    }


    private View addAwesomeTab(String id, int titleId, final int contentId) {
        GeckoTextView indicatorView = (GeckoTextView) mInflater.inflate(R.layout.awesomebar_tab_indicator, null);
        indicatorView.setText(titleId);

        getTabWidget().addView(indicatorView);

        
        
        indicatorView.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mViewPager.setCurrentItem(contentId, true);
            }
        });

        return indicatorView;
    }

    private boolean hideSoftInput(View view) {
        InputMethodManager imm =
                (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);

        return imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
        for (AwesomeBarTab tab : mTabs) {
            tab.setUrlListener(listener);
        }
    }

    public void destroy() {
        for (AwesomeBarTab tab : mTabs) {
            tab.destroy();
        }
    }

    public AllPagesTab getAllPagesTab() {
        return (AllPagesTab)getAwesomeBarTabForTag("allPages");
    }

    public BookmarksTab getBookmarksTab() {
        return (BookmarksTab)getAwesomeBarTabForTag("bookmarks");
    }

    public HistoryTab getHistoryTab() {
        return (HistoryTab)getAwesomeBarTabForTag("history");
    }

    public void filter(String searchTerm) {
        
        setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);

        
        AllPagesTab allPages = getAllPagesTab();
        setCurrentTabByTag(allPages.getTag());

        
        setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);

        
        mSearching = searchTerm.length() != 0;
        
        mViewPager.setAdapter(mPagerAdapter);
        int tabsVisibility = !mSearching ? View.VISIBLE : View.GONE;
        findViewById(R.id.tab_widget_container).setVisibility(tabsVisibility);

        
        allPages.filter(searchTerm);
    }

    public boolean isInReadingList() {
        return getBookmarksTab().isInReadingList();
    }

    public void setTarget(String target) {
        mTarget = target;
        styleSelectedTab();
        if (mTarget.equals(AwesomeBar.Target.CURRENT_TAB.name())) {
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null && tab.isPrivate())
                mBackground.setPrivateMode(true);
        }
    }

    public static class Background extends GeckoLinearLayout
                                   implements LightweightTheme.OnChangeListener { 
        private GeckoActivity mActivity;

        public Background(Context context, AttributeSet attrs) {
            super(context, attrs);
            mActivity = (GeckoActivity) context;
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

        StateListDrawable stateList = new StateListDrawable();
        stateList.addState(new int[] { R.attr.state_private }, mActivity.getResources().getDrawable(R.drawable.address_bar_bg_private));
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
            setBackgroundResource(R.drawable.awesomebar_tabs_bg);
            setPadding(padding[0], padding[1], padding[2], padding[3]);
        }

        @Override
        protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
            super.onLayout(changed, left, top, right, bottom);
            onLightweightThemeChanged();
        }
    }
}
