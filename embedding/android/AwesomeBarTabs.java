




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.provider.Browser;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.FilterQueryProvider;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;
import android.widget.TabHost;
import android.widget.TextView;

import java.util.Date;

public class AwesomeBarTabs extends TabHost {
    private static final String ALL_PAGES_TAB = "all";
    private static final String BOOKMARKS_TAB = "bookmarks";
    private static final String HISTORY_TAB = "history";

    private static final String LOG_NAME = "AwesomeBarTabs";

    private Context mContext;
    private OnUrlOpenListener mUrlOpenListener;

    private Cursor mAllPagesCursor;
    private SimpleCursorAdapter mAllPagesAdapter;

    public interface OnUrlOpenListener {
        public abstract void onUrlOpen(AwesomeBarTabs tabs, String url);
    }

    public AwesomeBarTabs(Context context, AttributeSet attrs) {
        super(context, attrs);

        Log.d(LOG_NAME, "Creating AwesomeBarTabs");

        mContext = context;

        
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.awesomebar_tabs, this);

        
        
        setup();

        addAllPagesTab();
        addBookmarksTab();
        addHistoryTab();

        
        filter("");
    }

    private TabSpec addAwesomeTab(String id, int titleId, int contentId) {
        TabSpec tab = newTabSpec(id);

        Resources resources = mContext.getResources();
        tab.setIndicator(resources.getString(titleId));

        tab.setContent(contentId);

        addTab(tab);

        return tab;
    }

    private void addAllPagesTab() {
        Log.d(LOG_NAME, "Creating All Pages tab");

        addAwesomeTab(ALL_PAGES_TAB,
                      R.string.awesomebar_all_pages_title,
                      R.id.all_pages_list);

        
        mAllPagesAdapter = new SimpleCursorAdapter(
            mContext,
            R.layout.awesomebar_row,
            null,
            new String[] { AwesomeBar.TITLE_KEY, AwesomeBar.URL_KEY },
            new int[] { R.id.title, R.id.url }
        );

        mAllPagesAdapter.setFilterQueryProvider(new FilterQueryProvider() {
            public Cursor runQuery(CharSequence constraint) {
                ContentResolver resolver = mContext.getContentResolver();

                mAllPagesCursor =
                    resolver.query(Browser.BOOKMARKS_URI,
                                   null, Browser.BookmarkColumns.URL + " LIKE ? OR title LIKE ?", 
                                   new String[] {"%" + constraint.toString() + "%", "%" + constraint.toString() + "%",},
                                   
                                   
                                   
                                   Browser.BookmarkColumns.VISITS + " * MAX(1, (" +
                                   Browser.BookmarkColumns.DATE + " - " + new Date().getTime() + ") / 86400000 + 120) DESC");   

                return mAllPagesCursor;
            }
        });

        final ListView allPagesList = (ListView) findViewById(R.id.all_pages_list);

        allPagesList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                handleItemClick(allPagesList, position);
            }
        });

        allPagesList.setAdapter(mAllPagesAdapter);
    }

    private void addBookmarksTab() {
        Log.d(LOG_NAME, "Creating Bookmarks tab");

        addAwesomeTab(BOOKMARKS_TAB,
                      R.string.awesomebar_bookmarks_title,
                      R.id.bookmarks_list);
    }

    private void addHistoryTab() {
        Log.d(LOG_NAME, "Creating History tab");

        addAwesomeTab(HISTORY_TAB,
                      R.string.awesomebar_history_title,
                      R.id.history_list);
    }

    private void handleItemClick(ListView list, int position) {
        Cursor cursor = (Cursor) list.getItemAtPosition(position);
        String url = cursor.getString(cursor.getColumnIndexOrThrow(AwesomeBar.URL_KEY));

        if (mUrlOpenListener != null)
            mUrlOpenListener.onUrlOpen(this, url);
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    public void destroy() {
        if (mAllPagesCursor != null) mAllPagesCursor.close();
    }

    public void filter(String searchTerm) {
        
        setCurrentTabByTag(ALL_PAGES_TAB);

        
        int tabsVisibility = (searchTerm.isEmpty() ? View.VISIBLE : View.GONE);
        getTabWidget().setVisibility(tabsVisibility);

        
        mAllPagesAdapter.getFilter().filter(searchTerm);
    }
}
