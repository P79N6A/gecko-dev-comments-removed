




































package org.mozilla.gecko;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.MatrixCursor;
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
import android.widget.FilterQueryProvider;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;
import android.widget.SimpleCursorTreeAdapter;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TabHost;
import android.widget.TextView;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.sync.repositories.android.BrowserContract.Bookmarks;

public class AwesomeBarTabs extends TabHost {
    private static final String LOGTAG = "GeckoAwesomeBarTabs";

    private static final String ALL_PAGES_TAB = "all";
    private static final String BOOKMARKS_TAB = "bookmarks";
    private static final String HISTORY_TAB = "history";

    private static enum HistorySection { TODAY, YESTERDAY, WEEK, OLDER };

    private Context mContext;
    private boolean mInflated;
    private OnUrlOpenListener mUrlOpenListener;
    private View.OnTouchListener mListTouchListener;
    private JSONArray mSearchEngines;

    private AwesomeBarCursorAdapter mAllPagesCursorAdapter;
    private BookmarksListAdapter mBookmarksAdapter;
    private SimpleExpandableListAdapter mHistoryAdapter;

    
    
    private static final int MAX_RESULTS = 100;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
        public void onSearch(String engine);
    }

    private class HistoryListAdapter extends SimpleExpandableListAdapter {
        public HistoryListAdapter(Context context, List<? extends Map<String, ?>> groupData,
                int groupLayout, String[] groupFrom, int[] groupTo,
                List<? extends List<? extends Map<String, ?>>> childData,
                int childLayout, String[] childFrom, int[] childTo) {

            super(context, groupData, groupLayout, groupFrom, groupTo,
                  childData, childLayout, childFrom, childTo);
        }

        @Override
        public View getChildView(int groupPosition, int childPosition, boolean isLastChild,
                View convertView, ViewGroup parent) {

            View childView =
                    super.getChildView(groupPosition, childPosition, isLastChild, convertView, parent); 

            @SuppressWarnings("unchecked")
            Map<String,Object> historyItem =
                    (Map<String,Object>) mHistoryAdapter.getChild(groupPosition, childPosition);

            byte[] b = (byte[]) historyItem.get(URLColumns.FAVICON);
            ImageView favicon = (ImageView) childView.findViewById(R.id.favicon);

            if (b == null) {
                favicon.setImageDrawable(null);
            } else {
                Bitmap bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);
                favicon.setImageBitmap(bitmap);
            }

            return childView;
        }
    }

    private class AwesomeCursorViewBinder implements SimpleCursorAdapter.ViewBinder,
                                                     SimpleCursorTreeAdapter.ViewBinder {
        private boolean updateFavicon(View view, Cursor cursor, int faviconIndex) {
            byte[] b = cursor.getBlob(faviconIndex);
            ImageView favicon = (ImageView) view;

            if (b == null) {
                favicon.setImageDrawable(null);
            } else {
                Bitmap bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);
                favicon.setImageBitmap(bitmap);
            }

            return true;
        }

        private boolean updateTitle(View view, Cursor cursor, int titleIndex) {
            String title = cursor.getString(titleIndex);
            TextView titleView = (TextView)view;
            
            
            if (TextUtils.isEmpty(title)) {
                int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
                title = cursor.getString(urlIndex);
            }

            titleView.setText(title);
            return true;
        }

        public boolean setViewValue(View view, Cursor cursor, int columnIndex) {
            
            
            int isFolderIndex = cursor.getColumnIndex(Bookmarks.IS_FOLDER);
            if (isFolderIndex >= 0 && cursor.getInt(isFolderIndex) == 1)
                return false;

            int faviconIndex = cursor.getColumnIndexOrThrow(URLColumns.FAVICON);
            if (columnIndex == faviconIndex) {
                return updateFavicon(view, cursor, faviconIndex);
            }

            int titleIndex = cursor.getColumnIndexOrThrow(URLColumns.TITLE);
            if (columnIndex == titleIndex) {
                return updateTitle(view, cursor, titleIndex);
            }

            
            return false;
        }
    }

    private class RefreshChildrenCursorTask extends AsyncTask<String, Void, Cursor> {
        private int mGroupPosition;

        public RefreshChildrenCursorTask(int groupPosition) {
            mGroupPosition = groupPosition;
        }

        @Override
        protected Cursor doInBackground(String... params) {
            ContentResolver resolver = mContext.getContentResolver();
            String guid = params[0];
            if (guid != null && guid.equals(Bookmarks.MOBILE_FOLDER_GUID))
                return BrowserDB.getMobileBookmarks(resolver);

            
            
            return BrowserDB.getDesktopBookmarks(resolver);
        }

        @Override
        protected void onPostExecute(Cursor childrenCursor) {
            mBookmarksAdapter.setChildrenCursor(mGroupPosition, childrenCursor);
        }
    }

    public class BookmarksListAdapter extends SimpleCursorTreeAdapter {
        public BookmarksListAdapter(Context context, Cursor cursor,
                int groupLayout, String[] groupFrom, int[] groupTo,
                int childLayout, String[] childFrom, int[] childTo) {
            super(context, cursor, groupLayout, groupFrom, groupTo, childLayout, childFrom, childTo);
        }

        @Override
        protected Cursor getChildrenCursor(Cursor groupCursor) {
            String guid = groupCursor.getString(groupCursor.getColumnIndexOrThrow(Bookmarks.GUID));

            
            new RefreshChildrenCursorTask(groupCursor.getPosition()).execute(guid);

            
            return new MatrixCursor(new String [] { Bookmarks._ID,
                                                    Bookmarks.URL,
                                                    Bookmarks.TITLE,
                                                    Bookmarks.FAVICON });
        }
    }

    private class BookmarksQueryTask extends AsyncTask<Void, Void, Cursor> {
        protected Cursor doInBackground(Void... arg0) {
            
            
            MatrixCursor c = new MatrixCursor(new String[] { Bookmarks._ID,
                                                             Bookmarks.IS_FOLDER,
                                                             Bookmarks.GUID,
                                                             URLColumns.TITLE }, 2);

            Resources resources = mContext.getResources();
            c.addRow(new Object[] { 0, 1, Bookmarks.MOBILE_FOLDER_GUID,
                                    resources.getString(R.string.bookmarks_folder_mobile)} );
            c.addRow(new Object[] { 1, 1, null,
                                    resources.getString(R.string.bookmarks_folder_desktop)} );
            return c;
        }

        protected void onPostExecute(Cursor cursor) {
            
            mBookmarksAdapter = new BookmarksListAdapter(
                mContext,
                cursor,
                R.layout.awesomebar_header_row,
                new String[] { URLColumns.TITLE },
                new int[] { R.id.title },
                R.layout.awesomebar_row,
                new String[] { URLColumns.TITLE,
                               URLColumns.URL,
                               URLColumns.FAVICON },
                new int[] { R.id.title, R.id.url, R.id.favicon }
            );

            mBookmarksAdapter.setViewBinder(new AwesomeCursorViewBinder());

            final ExpandableListView bookmarksList = (ExpandableListView) findViewById(R.id.bookmarks_list);

            
            bookmarksList.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {
                public boolean onChildClick(ExpandableListView parent, View view,
                        int groupPosition, int childPosition, long id) {
                    handleBookmarkItemClick(groupPosition, childPosition);
                    return true;
                }
            });

            bookmarksList.setAdapter(mBookmarksAdapter);

            
            bookmarksList.expandGroup(0);
            
            
            
            bookmarksList.expandGroup(1);
        }
    }

    private class HistoryQueryTask extends AsyncTask<Void, Void, Pair<List,List>> {
        private static final long MS_PER_DAY = 86400000;
        private static final long MS_PER_WEEK = MS_PER_DAY * 7;

        protected Pair<List,List> doInBackground(Void... arg0) {
            Pair<List,List> result = null;
            ContentResolver resolver = mContext.getContentResolver();
            Cursor cursor = BrowserDB.getRecentHistory(resolver, MAX_RESULTS);

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

            
            
            
            
            while (cursor.moveToNext()) {
                long time = cursor.getLong(cursor.getColumnIndexOrThrow(URLColumns.DATE_LAST_VISITED));
                HistorySection itemSection = getSectionForTime(time, today);

                if (groups == null)
                    groups = new LinkedList<Map<String,?>>();

                if (childrenLists == null)
                    childrenLists = new LinkedList<List<Map<String,?>>>();

                if (section != itemSection) {
                    if (section != null) {
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

            if (groups != null && childrenLists != null) {
                result = Pair.create((List) groups, (List) childrenLists);
            }

            return result;
        }

        public Map<String,?> createHistoryItem(Cursor cursor) {
            Map<String,Object> historyItem = new HashMap<String,Object>();

            String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            String title = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE));
            byte[] favicon = cursor.getBlob(cursor.getColumnIndexOrThrow(URLColumns.FAVICON));

            
            
            if (title == null || title.length() == 0)
                title = url;

            historyItem.put(URLColumns.URL, url);
            historyItem.put(URLColumns.TITLE, title);

            if (favicon != null)
                historyItem.put(URLColumns.FAVICON, favicon);

            return historyItem;
        }

        public Map<String,?> createGroupItem(HistorySection section) {
            Map<String,String> groupItem = new HashMap<String,String>();

            groupItem.put(URLColumns.TITLE, getSectionName(section));

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
            }

            if (delta < MS_PER_DAY) {
                return HistorySection.YESTERDAY;
            }

            if (delta < MS_PER_WEEK) {
                return HistorySection.WEEK;
            }

            return HistorySection.OLDER;
        }

        @SuppressWarnings("unchecked")
        protected void onPostExecute(Pair<List,List> result) {
            
            if (result == null)
                return;

            mHistoryAdapter = new HistoryListAdapter(
                mContext,
                result.first,
                R.layout.awesomebar_header_row,
                new String[] { URLColumns.TITLE },
                new int[] { R.id.title },
                result.second,
                R.layout.awesomebar_row,
                new String[] { URLColumns.TITLE, URLColumns.URL },
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

    private class AwesomeBarCursorAdapter extends SimpleCursorAdapter {
        private String mSearchTerm;

        public AwesomeBarCursorAdapter(Context context, int layout, Cursor c, String[] from, int[] to) {
            super(context, layout, c, from, to);
            mSearchTerm = "";
        }

        public void filter(String searchTerm) {
            mSearchTerm = searchTerm;
            getFilter().filter(searchTerm);
        }

        
        @Override
        public int getCount() {
            final int resultCount = super.getCount();

            
            if (mSearchTerm.length() == 0)
                return resultCount;

            return resultCount + mSearchEngines.length();
        }

        
        
        @Override
        public Object getItem(int position) {
            final int resultCount = super.getCount();
            if (position < resultCount)
                return super.getItem(position);

            JSONObject engine;
            String engineName = null;
            try {
                engine = mSearchEngines.getJSONObject(position - resultCount);
                engineName = engine.getString("name");
            } catch (JSONException e) {
                Log.e(LOGTAG, "error getting json arguments");
            }

            return engineName;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final int resultCount = super.getCount();
            if (position < resultCount)
                return super.getView(position, convertView, parent);

            View v;
            if (convertView == null)
                v = newView(mContext, null, parent);
            else
                v = convertView;
            bindSearchEngineView(position - resultCount, v);

            return v;
        }

        private Drawable getDrawableFromDataURI(String dataURI) {
            String base64 = dataURI.substring(dataURI.indexOf(',') + 1);
            Drawable drawable = null;
            try {
                byte[] bytes = GeckoAppShell.decodeBase64(base64);
                ByteArrayInputStream stream = new ByteArrayInputStream(bytes);
                drawable = Drawable.createFromStream(stream, "src");
                stream.close();
            } catch (IllegalArgumentException e) {
                Log.i(LOGTAG, "exception while decoding drawable: " + base64, e);
            } catch (IOException e) { }
            return drawable;
        }

        private void bindSearchEngineView(int position, View view) {
            String name;
            String iconURI;
            String searchText = getResources().getString(R.string.awesomebar_search_engine, mSearchTerm);
            try {
                JSONObject searchEngine = mSearchEngines.getJSONObject(position);
                name = searchEngine.getString("name");
                iconURI = searchEngine.getString("iconURI");
            } catch (JSONException e) {
                Log.e(LOGTAG, "error getting json arguments");
                return;
            }

            TextView titleView = (TextView) view.findViewById(R.id.title);
            TextView urlView = (TextView) view.findViewById(R.id.url);
            ImageView faviconView = (ImageView) view.findViewById(R.id.favicon);

            titleView.setText(name);
            urlView.setText(searchText);
            Drawable drawable = getDrawableFromDataURI(iconURI);
            if (drawable != null)
                faviconView.setImageDrawable(drawable);
        }
    };

    public AwesomeBarTabs(Context context, AttributeSet attrs) {
        super(context, attrs);

        Log.d(LOGTAG, "Creating AwesomeBarTabs");

        mContext = context;
        mInflated = false;
        mSearchEngines = new JSONArray();
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
                hideSoftInput(view);
                return false;
            }
        };

        addAllPagesTab();
        addBookmarksTab();
        addHistoryTab();

        setOnTabChangedListener(new TabHost.OnTabChangeListener() {
            public void onTabChanged(String tabId) {
                boolean hideSoftInput = true;

                
                
                if (tabId.equals(BOOKMARKS_TAB) && mBookmarksAdapter == null) {
                    new BookmarksQueryTask().execute();
                } else if (tabId.equals(HISTORY_TAB) && mHistoryAdapter == null) {
                    new HistoryQueryTask().execute();
                } else {
                    hideSoftInput = false;
                }

                
                if (hideSoftInput) {
                    View tabView = getCurrentTabView();
                    hideSoftInput(tabView);
                }
            }
        });

        
        filter("");
    }

    private TabSpec addAwesomeTab(String id, int titleId, int contentId) {
        TabSpec tab = newTabSpec(id);

        LayoutInflater inflater =
                (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        View indicatorView = inflater.inflate(R.layout.awesomebar_tab_indicator, null);
        Drawable background = indicatorView.getBackground();
        try {
            background.setColorFilter(new LightingColorFilter(Color.WHITE, GeckoApp.mBrowserToolbar.getHighlightColor()));
        } catch (Exception e) {
            Log.d(LOGTAG, "background.setColorFilter failed " + e);            
        }
        TextView title = (TextView) indicatorView.findViewById(R.id.title);
        title.setText(titleId);

        tab.setIndicator(indicatorView);
        tab.setContent(contentId);

        addTab(tab);

        return tab;
    }

    private void addAllPagesTab() {
        Log.d(LOGTAG, "Creating All Pages tab");

        addAwesomeTab(ALL_PAGES_TAB,
                      R.string.awesomebar_all_pages_title,
                      R.id.all_pages_list);

        
        mAllPagesCursorAdapter = new AwesomeBarCursorAdapter(
            mContext,
            R.layout.awesomebar_row,
            null,
            new String[] { URLColumns.TITLE,
                           URLColumns.URL,
                           URLColumns.FAVICON },
            new int[] { R.id.title, R.id.url, R.id.favicon }
        );

        mAllPagesCursorAdapter.setViewBinder(new AwesomeCursorViewBinder());

        mAllPagesCursorAdapter.setFilterQueryProvider(new FilterQueryProvider() {
            public Cursor runQuery(CharSequence constraint) {
                ContentResolver resolver = mContext.getContentResolver();
                long start = SystemClock.uptimeMillis();

                Cursor c = BrowserDB.filter(resolver, constraint, MAX_RESULTS);
                c.getCount(); 

                long end = SystemClock.uptimeMillis();
                Log.i(LOGTAG, "Got cursor in " + (end - start) + "ms");

                return c;
            }
        });

        final ListView allPagesList = (ListView) findViewById(R.id.all_pages_list);

        allPagesList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                handleItemClick(allPagesList, position);
            }
        });

        allPagesList.setAdapter(mAllPagesCursorAdapter);
        allPagesList.setOnTouchListener(mListTouchListener);
    }

    private void addBookmarksTab() {
        Log.d(LOGTAG, "Creating Bookmarks tab");

        addAwesomeTab(BOOKMARKS_TAB,
                      R.string.awesomebar_bookmarks_title,
                      R.id.bookmarks_list);

        ListView bookmarksList = (ListView) findViewById(R.id.bookmarks_list);
        bookmarksList.setOnTouchListener(mListTouchListener);

        
        
    }

    private void addHistoryTab() {
        Log.d(LOGTAG, "Creating History tab");

        addAwesomeTab(HISTORY_TAB,
                      R.string.awesomebar_history_title,
                      R.id.history_list);

        ListView historyList = (ListView) findViewById(R.id.history_list);
        historyList.setOnTouchListener(mListTouchListener);

        
        
    }

    private void hideSoftInput(View view) {
        InputMethodManager imm =
                (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);

        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    private void handleBookmarkItemClick(int groupPosition, int childPosition) {
        Cursor cursor = mBookmarksAdapter.getChild(groupPosition, childPosition);
        String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));

        if (mUrlOpenListener != null)
            mUrlOpenListener.onUrlOpen(url);
    }

    private void handleHistoryItemClick(int groupPosition, int childPosition) {
        @SuppressWarnings("unchecked")
        Map<String,Object> historyItem =
                (Map<String,Object>) mHistoryAdapter.getChild(groupPosition, childPosition);

        String url = (String) historyItem.get(URLColumns.URL);

        if (mUrlOpenListener != null)
            mUrlOpenListener.onUrlOpen(url);
    }

    private void handleItemClick(ListView list, int position) {
        Object item = list.getItemAtPosition(position);
        
        
        
        if (item instanceof Cursor) {
            Cursor cursor = (Cursor) item;
            String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            if (mUrlOpenListener != null)
                mUrlOpenListener.onUrlOpen(url);
        } else {
            if (mUrlOpenListener != null)
                mUrlOpenListener.onSearch((String)item);
        }
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    public void destroy() {
        Cursor allPagesCursor = mAllPagesCursorAdapter.getCursor();
        if (allPagesCursor != null)
            allPagesCursor.close();

        if (mBookmarksAdapter != null) {
            Cursor bookmarksCursor = mBookmarksAdapter.getCursor();
            if (bookmarksCursor != null)
                bookmarksCursor.close();
        }
    }

    public void filter(String searchTerm) {
        
        setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);

        
        setCurrentTabByTag(ALL_PAGES_TAB);

        
        setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);

        
        int tabsVisibility = (searchTerm.length() == 0 ? View.VISIBLE : View.GONE);
        getTabWidget().setVisibility(tabsVisibility);

        
        mAllPagesCursorAdapter.filter(searchTerm);
    }

    public void setSearchEngines(JSONArray engines) {
        mSearchEngines = engines;
        GeckoAppShell.getMainHandler().post(new Runnable() {
            public void run() {
                mAllPagesCursorAdapter.notifyDataSetChanged();
            }
        });
    }

    public void refreshBookmarks() {
        new AsyncTask<Void, Void, Cursor>() {
            protected Cursor doInBackground(Void... arg0) {
                ContentResolver resolver = mContext.getContentResolver();
                return BrowserDB.getAllBookmarks(resolver);
            }

            protected void onPostExecute(Cursor cursor) {
                mBookmarksAdapter.changeCursor(cursor);
            }
        }.execute();
    }
}
