




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.provider.Browser;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ExpandableListView;
import android.widget.FilterQueryProvider;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TabHost;
import android.widget.TextView;

import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class AwesomeBarTabs extends TabHost {
    private static final String ALL_PAGES_TAB = "all";
    private static final String BOOKMARKS_TAB = "bookmarks";
    private static final String HISTORY_TAB = "history";

    private static enum HistorySection { TODAY, YESTERDAY, WEEK, OLDER };

    private static final String LOG_NAME = "AwesomeBarTabs";

    private Context mContext;
    private OnUrlOpenListener mUrlOpenListener;

    private Cursor mAllPagesCursor;
    private SimpleCursorAdapter mAllPagesAdapter;

    private SimpleCursorAdapter mBookmarksAdapter;
    private SimpleExpandableListAdapter mHistoryAdapter;

    public interface OnUrlOpenListener {
        public abstract void onUrlOpen(AwesomeBarTabs tabs, String url);
    }

    private class BookmarksQueryTask extends AsyncTask<Void, Void, Cursor> {
        protected Cursor doInBackground(Void... arg0) {
            ContentResolver resolver = mContext.getContentResolver();

            return resolver.query(Browser.BOOKMARKS_URI,
                                  null,
                                  Browser.BookmarkColumns.BOOKMARK + " = 1",
                                  null,
                                  Browser.BookmarkColumns.TITLE);
        }

        protected void onPostExecute(Cursor cursor) {
            
            mBookmarksAdapter = new SimpleCursorAdapter(
                mContext,
                R.layout.awesomebar_row,
                cursor,
                new String[] { AwesomeBar.TITLE_KEY, AwesomeBar.URL_KEY },
                new int[] { R.id.title, R.id.url }
            );

            final ListView bookmarksList = (ListView) findViewById(R.id.bookmarks_list);

            bookmarksList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    handleItemClick(bookmarksList, position);
                }
            });

            bookmarksList.setAdapter(mBookmarksAdapter);
        }
    }

    private class HistoryQueryTask extends AsyncTask<Void, Void, Cursor> {
        
        
        private static final int MAX_RESULTS = 100;

        private static final long MS_PER_DAY = 86400000;
        private static final long MS_PER_WEEK = MS_PER_DAY * 7;

        protected Cursor doInBackground(Void... arg0) {
            ContentResolver resolver = mContext.getContentResolver();

            return resolver.query(Browser.BOOKMARKS_URI,
                                  null,
                                  null,
                                  null,
                                  Browser.BookmarkColumns.DATE + " DESC");
        }

        public Map<String,?> createHistoryItem(Cursor cursor) {
            Map<String,String> historyItem = new HashMap<String,String>();

            

            String url = cursor.getString(cursor.getColumnIndexOrThrow(AwesomeBar.URL_KEY));
            String title = cursor.getString(cursor.getColumnIndexOrThrow(AwesomeBar.TITLE_KEY));

            historyItem.put(AwesomeBar.URL_KEY, url);
            historyItem.put(AwesomeBar.TITLE_KEY, title);

            return historyItem;
        }

        public Map<String,?> createGroupItem(HistorySection section) {
            Map<String,String> groupItem = new HashMap<String,String>();

            groupItem.put(AwesomeBar.TITLE_KEY, getSectionName(section));

            return groupItem;
        }

        private String getSectionName(HistorySection section) {
            Resources resources = mContext.getResources();

            switch (section) {
            case TODAY:
                return resources.getString(R.string.history_today_section);
            case YESTERDAY:
                return resources.getString(R.string.history_yesterday_section);
            case WEEK:
                return resources.getString(R.string.history_week_section);
            case OLDER:
                return resources.getString(R.string.history_older_section);
            }

            return null;
        }

        private void expandAllGroups(ExpandableListView historyList) {
            int groupCount = mHistoryAdapter.getGroupCount();

            for (int i = 0; i < groupCount; i++) {
                historyList.expandGroup(i);
            }
        }

        private HistorySection getSectionForTime(long time, long today) {
            long delta = today - time;

            if (delta < 0) {
                return HistorySection.TODAY;
            } else if (delta > 0 && delta < MS_PER_DAY) {
                return HistorySection.YESTERDAY;
            } else if (delta > MS_PER_DAY && delta < MS_PER_WEEK) {
                return HistorySection.WEEK;
            }

            return HistorySection.OLDER;
        }

        protected void onPostExecute(Cursor cursor) {
            Date now = new Date();
            now.setHours(0);
            now.setMinutes(0);
            now.setSeconds(0);

            long today = now.getTime();

            
            
            List<List<Map<String,?>>> childrenLists = null;
            List<Map<String,?>> children = null;
            List<Map<String,?>> groups = null;
            HistorySection section = null;

            
            
            cursor.moveToPosition(-1);

            
            
            
            
            while (cursor.moveToNext() && cursor.getPosition() < MAX_RESULTS) {
                long time = cursor.getLong(cursor.getColumnIndexOrThrow(Browser.BookmarkColumns.DATE));
                HistorySection itemSection = getSectionForTime(time, today);

                if (section != itemSection) {
                    if (section != null) {
                        if (groups == null)
                            groups = new LinkedList<Map<String,?>>();

                        if (childrenLists == null)
                            childrenLists = new LinkedList<List<Map<String,?>>>();

                        groups.add(createGroupItem(section));
                        childrenLists.add(children);
                    }

                    section = itemSection;
                    children = new LinkedList<Map<String,?>>();
                }

                children.add(createHistoryItem(cursor));
            }

            
            
            if (section != null && children != null) {
                groups.add(createGroupItem(section));
                childrenLists.add(children);
            }

            
            cursor.close();

            mHistoryAdapter = new SimpleExpandableListAdapter(
                mContext,
                groups,
                R.layout.awesomebar_header_row,
                new String[] { AwesomeBar.TITLE_KEY },
                new int[] { R.id.title },
                childrenLists,
                R.layout.awesomebar_row,
                new String[] { AwesomeBar.TITLE_KEY, AwesomeBar.URL_KEY },
                new int[] { R.id.title, R.id.url }
            );

            final ExpandableListView historyList =
                    (ExpandableListView) findViewById(R.id.history_list);

            historyList.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {
                public boolean onChildClick(ExpandableListView parent, View view,
                        int groupPosition, int childPosition, long id) {
                    handleHistoryItemClick(groupPosition, childPosition);
                    return true;
                }
            });

            
            
            
            historyList.setOnGroupClickListener(new ExpandableListView.OnGroupClickListener() {
                public boolean onGroupClick(ExpandableListView parent, View v,
                        int groupPosition, long id) {
                    return true;
                }
            });

            historyList.setAdapter(mHistoryAdapter);

            expandAllGroups(historyList);
        }
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

        new BookmarksQueryTask().execute();
    }

    private void addHistoryTab() {
        Log.d(LOG_NAME, "Creating History tab");

        addAwesomeTab(HISTORY_TAB,
                      R.string.awesomebar_history_title,
                      R.id.history_list);

        new HistoryQueryTask().execute();
    }

    private void handleHistoryItemClick(int groupPosition, int childPosition) {
        @SuppressWarnings("unchecked")
        Map<String,String> historyItem =
                (Map<String,String>) mHistoryAdapter.getChild(groupPosition, childPosition);

        String url = historyItem.get(AwesomeBar.URL_KEY);

        if (mUrlOpenListener != null)
            mUrlOpenListener.onUrlOpen(this, url);
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

        Cursor bookmarksCursor = mBookmarksAdapter.getCursor();
        if (bookmarksCursor != null)
            bookmarksCursor.close();
    }

    public void filter(String searchTerm) {
        
        setCurrentTabByTag(ALL_PAGES_TAB);

        
        int tabsVisibility = (searchTerm.isEmpty() ? View.VISIBLE : View.GONE);
        getTabWidget().setVisibility(tabsVisibility);

        
        mAllPagesAdapter.getFilter().filter(searchTerm);
    }
}
