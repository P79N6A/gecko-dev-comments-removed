




package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.LightingColorFilter;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ExpandableListView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TabHost;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;

public class AwesomeBarTabs extends TabHost {
    private static final String LOGTAG = "GeckoAwesomeBarTabs";

    private Context mContext;
    private boolean mInflated;
    private LayoutInflater mInflater;
    private OnUrlOpenListener mUrlOpenListener;
    private View.OnTouchListener mListTouchListener;
    
    private AllPagesTab mAllPagesTab;
    public BookmarksTab mBookmarksTab;
    public HistoryTab mHistoryTab;

    
    
    private static final int MAX_RESULTS = 100;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
        public void onSearch(String engine, String text);
        public void onEditSuggestion(String suggestion);
    }

    
    
    public boolean onBackPressed() {
        
        
        
        if (getCurrentTabTag().equals(mBookmarksTab.getTag()) || getCurrentTabTag().equals(mHistoryTab.getTag())) {
            View tabView = getCurrentTabView();
            if (hideSoftInput(tabView))
                return true;
        }

        
        
        if (!getCurrentTabTag().equals(mBookmarksTab.getTag()))
            return false;

        return mBookmarksTab.moveToParentFolder();
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

        addAllPagesTab();
        addBookmarksTab();
        addHistoryTab();

        
        filter("");
    }

    private TabSpec addAwesomeTab(String id, int titleId, TabHost.TabContentFactory factory) {
        TabSpec tab = getTabSpec(id, titleId);
        tab.setContent(factory);
        addTab(tab);

        return tab;
    }

    private TabSpec addAwesomeTab(String id, int titleId, int contentId) {
        TabSpec tab = getTabSpec(id, titleId);
        tab.setContent(contentId);
        addTab(tab);

        return tab;
    }

    private TabSpec getTabSpec(String id, int titleId) {
        TabSpec tab = newTabSpec(id);

        View indicatorView = mInflater.inflate(R.layout.awesomebar_tab_indicator, null);
        Drawable background = indicatorView.getBackground();
        try {
            background.setColorFilter(new LightingColorFilter(Color.WHITE, 0xFFFF9500));
        } catch (Exception e) {
            Log.d(LOGTAG, "background.setColorFilter failed " + e);            
        }
        TextView title = (TextView) indicatorView.findViewById(R.id.title);
        title.setText(titleId);

        tab.setIndicator(indicatorView);
        return tab;
    }

    private void addAllPagesTab() {
        Log.d(LOGTAG, "Creating All Pages tab");

        mAllPagesTab = new AllPagesTab(mContext);
        addAwesomeTab(mAllPagesTab.getTag(),
                      mAllPagesTab.getTitleStringId(),
                      mAllPagesTab.getFactory());
        mAllPagesTab.setListTouchListener(mListTouchListener);
    }

    private void addBookmarksTab() {
        Log.d(LOGTAG, "Creating Bookmarks tab");

        mBookmarksTab = new BookmarksTab(mContext);
        addAwesomeTab(mBookmarksTab.getTag(),
                      mBookmarksTab.getTitleStringId(),
                      mBookmarksTab.getFactory());
        mBookmarksTab.setListTouchListener(mListTouchListener);

        
        
    }

    private void addHistoryTab() {
        Log.d(LOGTAG, "Creating History tab");

        mHistoryTab = new HistoryTab(mContext);
        addAwesomeTab(mHistoryTab.getTag(),
                      mHistoryTab.getTitleStringId(),
                      mHistoryTab.getFactory());
        mHistoryTab.setListTouchListener(mListTouchListener);

        
        
    }

    private boolean hideSoftInput(View view) {
        InputMethodManager imm =
                (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);

        return imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    private String getReaderForUrl(String url) {
        
        
        return "about:reader?url=" + url;
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
        mAllPagesTab.setUrlListener(listener);
        mBookmarksTab.setUrlListener(listener);
        mHistoryTab.setUrlListener(listener);
    }

    public void destroy() {
        mAllPagesTab.destroy();
        mBookmarksTab.destroy();
        mHistoryTab.destroy();
    }

    public void filter(String searchTerm) {
        
        setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);

        
        setCurrentTabByTag(mAllPagesTab.getTag());

        
        setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);

        
        int tabsVisibility = (searchTerm.length() == 0 ? View.VISIBLE : View.GONE);
        getTabWidget().setVisibility(tabsVisibility);

        
        mAllPagesTab.filter(searchTerm);
    }

    



    public void setSuggestions(final ArrayList<String> suggestions) {
        GeckoAppShell.getMainHandler().post(new Runnable() {
            public void run() {
                mAllPagesTab.setSuggestions(suggestions);
            }
        });
    }

    


    public void setSearchEngines(final String suggestEngineName, final JSONArray engines) {
        GeckoAppShell.getMainHandler().post(new Runnable() {
            public void run() {
                mAllPagesTab.setSearchEngines(suggestEngineName, engines);
            }
        });
    }

    public boolean isInReadingList() {
        return mBookmarksTab.isInReadingList();
    }
}
