




package org.mozilla.gecko;

import android.content.Context;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;

public class AwesomeBarTabs extends TabHost {
    private static final String LOGTAG = "GeckoAwesomeBarTabs";

    private Context mContext;
    private boolean mInflated;
    private LayoutInflater mInflater;
    private OnUrlOpenListener mUrlOpenListener;
    private View.OnTouchListener mListTouchListener;
    
    private AwesomeBarTab mTabs[];

    
    
    private static final int MAX_RESULTS = 100;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
        public void onSearch(String engine, String text);
        public void onEditSuggestion(String suggestion);
    }

    private AwesomeBarTab getCurrentAwesomeBarTab() {
        String tag = getCurrentTabTag();
        return getAwesomeBarTabForTag(tag);
    }

    public AwesomeBarTab getAwesomeBarTabForView(View view) {
        String tag = (String)view.getTag();
        return getAwesomeBarTabForTag(tag);
    }

    public AwesomeBarTab getAwesomeBarTabForTag(String tag) {
        for (AwesomeBarTab tab : mTabs) {
            if (tag == tab.getTag()) {
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

        mTabs = new AwesomeBarTab[] {
            new AllPagesTab(mContext),
            new BookmarksTab(mContext),
            new HistoryTab(mContext)
        };

        for (AwesomeBarTab tab : mTabs) {
            addAwesomeTab(tab);
        }

        styleSelectedTab();

         setOnTabChangedListener(new TabHost.OnTabChangeListener() {
             public void onTabChanged(String tabId) {
                 styleSelectedTab();
             }
         });

        
        filter("");
    }

    private void styleSelectedTab() {
        int selIndex = getCurrentTab();
        TabWidget tabWidget = getTabWidget();
        for (int i = 0; i < tabWidget.getTabCount(); i++) {
             if (i == selIndex)
                 continue;

             if (i == (selIndex - 1))
                 tabWidget.getChildTabViewAt(i).getBackground().setLevel(1);
             else if (i == (selIndex + 1))
                 tabWidget.getChildTabViewAt(i).getBackground().setLevel(2);
             else
                 tabWidget.getChildTabViewAt(i).getBackground().setLevel(0);
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


    private void addAwesomeTab(AwesomeBarTab tab) {
        TabSpec tabspec = getTabSpec(tab.getTag(), tab.getTitleStringId());
        tabspec.setContent(tab.getFactory());
        addTab(tabspec);
        tab.setListTouchListener(mListTouchListener);
 
        return;
    }

    private TabSpec getTabSpec(String id, int titleId) {
        TabSpec tab = newTabSpec(id);

        TextView indicatorView = (TextView) mInflater.inflate(R.layout.awesomebar_tab_indicator, null);
        indicatorView.setText(titleId);

        tab.setIndicator(indicatorView);
        return tab;
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

        
        int tabsVisibility = (searchTerm.length() == 0 ? View.VISIBLE : View.GONE);
        findViewById(R.id.tab_widget_container).setVisibility(tabsVisibility);

        
        allPages.filter(searchTerm);
    }

    public boolean isInReadingList() {
        return getBookmarksTab().isInReadingList();
    }
}
