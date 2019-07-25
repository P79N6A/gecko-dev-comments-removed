




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

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;

public class AwesomeBarTabs extends TabHost {
    private static final String LOGTAG = "GeckoAwesomeBarTabs";

    private static final String BOOKMARKS_TAB = "bookmarks";
    private static final String HISTORY_TAB = "history";

    private static enum HistorySection { TODAY, YESTERDAY, WEEK, OLDER };

    private Context mContext;
    private boolean mInflated;
    private LayoutInflater mInflater;
    private OnUrlOpenListener mUrlOpenListener;
    private ContentResolver mContentResolver;
    private ContentObserver mContentObserver;

    private BookmarksQueryTask mBookmarksQueryTask;
    private HistoryQueryTask mHistoryQueryTask;
    
    private AllPagesTab mAllPagesTab;
    private BookmarksListAdapter mBookmarksAdapter;
    private SimpleExpandableListAdapter mHistoryAdapter;

    private boolean mInReadingList;

    
    
    private static final int MAX_RESULTS = 100;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
        public void onSearch(String engine, String text);
        public void onEditSuggestion(String suggestion);
    }

    private class AwesomeEntryViewHolder {
        public TextView titleView;
        public TextView urlView;
        public ImageView faviconView;
        public ImageView bookmarkIconView;
    }

    private class HistoryListAdapter extends SimpleExpandableListAdapter {
        public HistoryListAdapter(Context context, List<? extends Map<String, ?>> groupData,
                int groupLayout, String[] groupFrom, int[] groupTo,
                List<? extends List<? extends Map<String, ?>>> childData) {

            super(context, groupData, groupLayout, groupFrom, groupTo,
                  childData, -1, new String[] {}, new int[] {});
        }

        @Override
        public View getChildView(int groupPosition, int childPosition, boolean isLastChild,
                View convertView, ViewGroup parent) {
            AwesomeEntryViewHolder viewHolder = null;

            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.awesomebar_row, null);

                viewHolder = new AwesomeEntryViewHolder();
                viewHolder.titleView = (TextView) convertView.findViewById(R.id.title);
                viewHolder.urlView = (TextView) convertView.findViewById(R.id.url);
                viewHolder.faviconView = (ImageView) convertView.findViewById(R.id.favicon);
                viewHolder.bookmarkIconView = (ImageView) convertView.findViewById(R.id.bookmark_icon);

                convertView.setTag(viewHolder);
            } else {
                viewHolder = (AwesomeEntryViewHolder) convertView.getTag();
            }

            @SuppressWarnings("unchecked")
            Map<String,Object> historyItem =
                    (Map<String,Object>) mHistoryAdapter.getChild(groupPosition, childPosition);

            String title = (String) historyItem.get(URLColumns.TITLE);
            String url = (String) historyItem.get(URLColumns.URL);

            if (TextUtils.isEmpty(title))
                title = url;

            viewHolder.titleView.setText(title);
            viewHolder.urlView.setText(url);

            byte[] b = (byte[]) historyItem.get(URLColumns.FAVICON);

            if (b == null) {
                viewHolder.faviconView.setImageDrawable(null);
            } else {
                Bitmap bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);
                viewHolder.faviconView.setImageBitmap(bitmap);
            }

            Integer bookmarkId = (Integer) historyItem.get(Combined.BOOKMARK_ID);
            Integer display = (Integer) historyItem.get(Combined.DISPLAY);

            
            
            
            int visibility = (bookmarkId != 0 && display != Combined.DISPLAY_READER ?
                              View.VISIBLE : View.GONE);

            viewHolder.bookmarkIconView.setVisibility(visibility);
            viewHolder.bookmarkIconView.setImageResource(R.drawable.ic_awesomebar_star);

            return convertView;
        }
    }

    private class BookmarksListAdapter extends SimpleCursorAdapter {
        private static final int VIEW_TYPE_ITEM = 0;
        private static final int VIEW_TYPE_FOLDER = 1;
        private static final int VIEW_TYPE_COUNT = 2;

        private Resources mResources;
        private LinkedList<Pair<Integer, String>> mParentStack;
        private LinearLayout mBookmarksTitleView;

        public BookmarksListAdapter(Context context, Cursor c) {
            super(context, -1, c, new String[] {}, new int[] {});

            mResources = mContext.getResources();

            
            
            mParentStack = new LinkedList<Pair<Integer, String>>();

            
            Pair<Integer, String> rootFolder = new Pair<Integer, String>(Bookmarks.FIXED_ROOT_ID, "");
            mParentStack.addFirst(rootFolder);
        }

        public void refreshCurrentFolder() {
            
            if (mBookmarksQueryTask != null)
                mBookmarksQueryTask.cancel(false);

            Pair<Integer, String> folderPair = mParentStack.getFirst();
            mInReadingList = (folderPair.first == Bookmarks.FIXED_READING_LIST_ID);

            mBookmarksQueryTask = new BookmarksQueryTask(folderPair.first, folderPair.second);
            mBookmarksQueryTask.execute();
        }

        
        public boolean moveToParentFolder() {
            
            if (mParentStack.size() == 1)
                return false;

            mParentStack.removeFirst();
            refreshCurrentFolder();
            return true;
        }

        public void moveToChildFolder(int folderId, String folderTitle) {
            Pair<Integer, String> folderPair = new Pair<Integer, String>(folderId, folderTitle);
            mParentStack.addFirst(folderPair);
            refreshCurrentFolder();
        }

        public int getItemViewType(int position) {
            Cursor c = getCursor();
 
            if (c.moveToPosition(position) &&
                c.getInt(c.getColumnIndexOrThrow(Bookmarks.TYPE)) == Bookmarks.TYPE_FOLDER)
                return VIEW_TYPE_FOLDER;

            
            return VIEW_TYPE_ITEM;
        }
 
        @Override
        public int getViewTypeCount() {
            return VIEW_TYPE_COUNT;
        }

        public String getFolderTitle(int position) {
            Cursor c = getCursor();
            if (!c.moveToPosition(position))
                return "";

            String guid = c.getString(c.getColumnIndexOrThrow(Bookmarks.GUID));

            
            if (guid == null || guid.length() == 12)
                return c.getString(c.getColumnIndexOrThrow(Bookmarks.TITLE));

            
            if (guid.equals(Bookmarks.FAKE_DESKTOP_FOLDER_GUID))
                return mResources.getString(R.string.bookmarks_folder_desktop);
            else if (guid.equals(Bookmarks.MENU_FOLDER_GUID))
                return mResources.getString(R.string.bookmarks_folder_menu);
            else if (guid.equals(Bookmarks.TOOLBAR_FOLDER_GUID))
                return mResources.getString(R.string.bookmarks_folder_toolbar);
            else if (guid.equals(Bookmarks.UNFILED_FOLDER_GUID))
                return mResources.getString(R.string.bookmarks_folder_unfiled);
            else if (guid.equals(Bookmarks.READING_LIST_FOLDER_GUID))
                return mResources.getString(R.string.bookmarks_folder_reading_list);

            
            
            return c.getString(c.getColumnIndexOrThrow(Bookmarks.TITLE));
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            int viewType = getItemViewType(position);
            AwesomeEntryViewHolder viewHolder = null;

            if (convertView == null) {
                if (viewType == VIEW_TYPE_ITEM)
                    convertView = mInflater.inflate(R.layout.awesomebar_row, null);
                else
                    convertView = mInflater.inflate(R.layout.awesomebar_folder_row, null);

                viewHolder = new AwesomeEntryViewHolder();
                viewHolder.titleView = (TextView) convertView.findViewById(R.id.title);
                viewHolder.faviconView = (ImageView) convertView.findViewById(R.id.favicon);

                if (viewType == VIEW_TYPE_ITEM)
                    viewHolder.urlView = (TextView) convertView.findViewById(R.id.url);

                convertView.setTag(viewHolder);
            } else {
                viewHolder = (AwesomeEntryViewHolder) convertView.getTag();
            }

            Cursor cursor = getCursor();
            if (!cursor.moveToPosition(position))
                throw new IllegalStateException("Couldn't move cursor to position " + position);

            if (viewType == VIEW_TYPE_ITEM) {
                updateTitle(viewHolder.titleView, cursor);
                updateUrl(viewHolder.urlView, cursor);
                updateFavicon(viewHolder.faviconView, cursor);
            } else {
                int guidIndex = cursor.getColumnIndexOrThrow(Bookmarks.GUID);
                String guid = cursor.getString(guidIndex);

                if (guid.equals(Bookmarks.READING_LIST_FOLDER_GUID)) {
                    viewHolder.faviconView.setImageResource(R.drawable.reading_list);
                } else {
                    viewHolder.faviconView.setImageResource(R.drawable.folder);
                }

                viewHolder.titleView.setText(getFolderTitle(position));
            }

            return convertView;
        }

        public LinearLayout getHeaderView() {
            return mBookmarksTitleView;
        }

        public void setHeaderView(LinearLayout titleView) {
            mBookmarksTitleView = titleView;
        }
    }

    
    
    public boolean onBackPressed() {
        
        
        
        if (getCurrentTabTag().equals(BOOKMARKS_TAB) || getCurrentTabTag().equals(HISTORY_TAB)) {
            View tabView = getCurrentTabView();
            if (hideSoftInput(tabView))
                return true;
        }

        
        
        if (!getCurrentTabTag().equals(BOOKMARKS_TAB) || mBookmarksAdapter == null)
            return false;

        return mBookmarksAdapter.moveToParentFolder();
    }
     
    private class BookmarksQueryTask extends AsyncTask<Void, Void, Cursor> {
        private int mFolderId;
        private String mFolderTitle;

        public BookmarksQueryTask() {
            mFolderId = Bookmarks.FIXED_ROOT_ID;
            mFolderTitle = "";
        }

        public BookmarksQueryTask(int folderId, String folderTitle) {
            mFolderId = folderId;
            mFolderTitle = folderTitle;
        }

        @Override
        protected Cursor doInBackground(Void... arg0) {
            return BrowserDB.getBookmarksInFolder(mContentResolver, mFolderId);
        }

        @Override
        protected void onPostExecute(final Cursor cursor) {
            final ListView list = (ListView) findViewById(R.id.bookmarks_list);

            
            GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                public void run() {
                    list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                            handleBookmarkItemClick(parent, view, position, id);
                        }
                    });
                    
                    
                    list.setAdapter(null);

                    if (mBookmarksAdapter == null) {
                        mBookmarksAdapter = new BookmarksListAdapter(mContext, cursor);
                    } else {
                        mBookmarksAdapter.changeCursor(cursor);
                    }

                    LinearLayout headerView = mBookmarksAdapter.getHeaderView();
                    if (headerView == null) {
                        headerView = (LinearLayout) mInflater.inflate(R.layout.awesomebar_header_row, null);
                        mBookmarksAdapter.setHeaderView(headerView);
                    }

                    
                    if (mFolderId == Bookmarks.FIXED_ROOT_ID) {
                        if (list.getHeaderViewsCount() == 1)
                            list.removeHeaderView(headerView);
                    } else {
                        if (list.getHeaderViewsCount() == 0)
                            list.addHeaderView(headerView, null, true);

                        ((TextView) headerView.findViewById(R.id.title)).setText(mFolderTitle);
                    }

                    list.setAdapter(mBookmarksAdapter);
                }
            });

            mBookmarksQueryTask = null;
        }
    }

    private static class GroupList extends LinkedList<Map<String,String>> {
        private static final long serialVersionUID = 0L;
    }

    private static class ChildrenList extends LinkedList<Map<String,Object>> {
        private static final long serialVersionUID = 0L;
    }

    private class HistoryQueryTask extends AsyncTask<Void, Void, Pair<GroupList,List<ChildrenList>>> {
        private static final long MS_PER_DAY = 86400000;
        private static final long MS_PER_WEEK = MS_PER_DAY * 7;

        protected Pair<GroupList,List<ChildrenList>> doInBackground(Void... arg0) {
            Cursor cursor = BrowserDB.getRecentHistory(mContentResolver, MAX_RESULTS);

            Date now = new Date();
            now.setHours(0);
            now.setMinutes(0);
            now.setSeconds(0);

            long today = now.getTime();

            
            
            List<ChildrenList> childrenLists = new LinkedList<ChildrenList>();
            ChildrenList children = null;
            GroupList groups = new GroupList();
            HistorySection section = null;

            
            
            cursor.moveToPosition(-1);

            
            
            
            
            while (cursor.moveToNext()) {
                long time = cursor.getLong(cursor.getColumnIndexOrThrow(URLColumns.DATE_LAST_VISITED));
                HistorySection itemSection = getSectionForTime(time, today);

                if (section != itemSection) {
                    if (section != null) {
                        groups.add(createGroupItem(section));
                        childrenLists.add(children);
                    }

                    section = itemSection;
                    children = new ChildrenList();
                }

                children.add(createHistoryItem(cursor));
            }

            
            
            if (section != null && children != null) {
                groups.add(createGroupItem(section));
                childrenLists.add(children);
            }

            
            cursor.close();

            
            return Pair.<GroupList,List<ChildrenList>>create(groups, childrenLists);
        }

        public Map<String,Object> createHistoryItem(Cursor cursor) {
            Map<String,Object> historyItem = new HashMap<String,Object>();

            String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            String title = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE));
            byte[] favicon = cursor.getBlob(cursor.getColumnIndexOrThrow(URLColumns.FAVICON));
            Integer bookmarkId = cursor.getInt(cursor.getColumnIndexOrThrow(Combined.BOOKMARK_ID));
            Integer historyId = cursor.getInt(cursor.getColumnIndexOrThrow(Combined.HISTORY_ID));
            Integer display = cursor.getInt(cursor.getColumnIndexOrThrow(Combined.DISPLAY));

            
            
            if (title == null || title.length() == 0)
                title = url;

            historyItem.put(URLColumns.URL, url);
            historyItem.put(URLColumns.TITLE, title);

            if (favicon != null)
                historyItem.put(URLColumns.FAVICON, favicon);

            historyItem.put(Combined.BOOKMARK_ID, bookmarkId);
            historyItem.put(Combined.HISTORY_ID, historyId);
            historyItem.put(Combined.DISPLAY, display);

            return historyItem;
        }

        public Map<String,String> createGroupItem(HistorySection section) {
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

        protected void onPostExecute(Pair<GroupList,List<ChildrenList>> result) {
            mHistoryAdapter = new HistoryListAdapter(
                mContext,
                result.first,
                R.layout.awesomebar_header_row,
                new String[] { URLColumns.TITLE },
                new int[] { R.id.title },
                result.second
            );

            if (mContentObserver == null) {
                
                mContentObserver = new ContentObserver(GeckoAppShell.getHandler()) {
                    public void onChange(boolean selfChange) {
                        mHistoryQueryTask = new HistoryQueryTask();
                        mHistoryQueryTask.execute();
                    }
                };
                BrowserDB.registerHistoryObserver(mContentResolver, mContentObserver);
            }

            final ExpandableListView historyList =
                    (ExpandableListView) findViewById(R.id.history_list);

            
            GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                public void run() {
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
            });

            mHistoryQueryTask = null;
        }
    }

    public AwesomeBarTabs(Context context, AttributeSet attrs) {
        super(context, attrs);

        Log.d(LOGTAG, "Creating AwesomeBarTabs");

        mContext = context;
        mInflated = false;
        mContentResolver = context.getContentResolver();
        mContentObserver = null;
        mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        mInReadingList = false;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        
        
        
        if (mInflated)
            return;

        mInflated = true;

        
        
        setup();

        addAllPagesTab();
        addBookmarksTab();
        addHistoryTab();

        setOnTabChangedListener(new TabHost.OnTabChangeListener() {
            public void onTabChanged(String tabId) {
                boolean hideSoftInput = true;

                
                
                if (tabId.equals(BOOKMARKS_TAB) && mBookmarksAdapter == null
                        && mBookmarksQueryTask == null) {
                    mBookmarksQueryTask = new BookmarksQueryTask();
                    mBookmarksQueryTask.execute();
                } else if (tabId.equals(HISTORY_TAB) && mHistoryAdapter == null
                        && mHistoryQueryTask == null) {
                    mHistoryQueryTask = new HistoryQueryTask();
                    mHistoryQueryTask.execute();
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
    }

    private void addBookmarksTab() {
        Log.d(LOGTAG, "Creating Bookmarks tab");

        addAwesomeTab(BOOKMARKS_TAB,
                      R.string.awesomebar_bookmarks_title,
                      R.id.bookmarks_list);

        ListView bookmarksList = (ListView) findViewById(R.id.bookmarks_list);

        
        
    }

    private void addHistoryTab() {
        Log.d(LOGTAG, "Creating History tab");

        addAwesomeTab(HISTORY_TAB,
                      R.string.awesomebar_history_title,
                      R.id.history_list);

        ListView historyList = (ListView) findViewById(R.id.history_list);

        
        
    }

    private boolean hideSoftInput(View view) {
        InputMethodManager imm =
                (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);

        return imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    private String getReaderForUrl(String url) {
        
        
        return "about:reader?url=" + url;
    }

    private void handleBookmarkItemClick(AdapterView<?> parent, View view, int position, long id) {
        int headerCount = ((ListView) parent).getHeaderViewsCount();
        
        if (headerCount == 1 && position == 0)
            return;

        Cursor cursor = mBookmarksAdapter.getCursor();
        
        if (headerCount == 1)
            position--;

        cursor.moveToPosition(position);

        int type = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks.TYPE));
        if (type == Bookmarks.TYPE_FOLDER) {
            
            int folderId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
            String folderTitle = mBookmarksAdapter.getFolderTitle(position);

            mBookmarksAdapter.moveToChildFolder(folderId, folderTitle);
            return;
        }

        
        String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
        if (mUrlOpenListener != null) {
            if (mInReadingList) {
                url = getReaderForUrl(url);
            }

            mUrlOpenListener.onUrlOpen(url);
        }
    }

    private void handleHistoryItemClick(int groupPosition, int childPosition) {
        @SuppressWarnings("unchecked")
        Map<String,Object> historyItem =
                (Map<String,Object>) mHistoryAdapter.getChild(groupPosition, childPosition);

        String url = (String) historyItem.get(URLColumns.URL);

        if (mUrlOpenListener != null)
            mUrlOpenListener.onUrlOpen(url);
    }

    private void updateFavicon(ImageView faviconView, Cursor cursor) {
        byte[] b = cursor.getBlob(cursor.getColumnIndexOrThrow(URLColumns.FAVICON));
        if (b == null) {
            faviconView.setImageDrawable(null);
        } else {
            Bitmap bitmap = BitmapFactory.decodeByteArray(b, 0, b.length);
            faviconView.setImageBitmap(bitmap);
        }
    }

    private void updateTitle(TextView titleView, Cursor cursor) {
        int titleIndex = cursor.getColumnIndexOrThrow(URLColumns.TITLE);
        String title = cursor.getString(titleIndex);

        
        
        if (TextUtils.isEmpty(title)) {
            int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
            title = cursor.getString(urlIndex);
        }

        titleView.setText(title);
    }

    private void updateUrl(TextView urlView, Cursor cursor) {
        int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
        String url = cursor.getString(urlIndex);

        urlView.setText(url);
    }

    private void updateBookmarkIcon(ImageView bookmarkIconView, Cursor cursor) {
        int bookmarkIdIndex = cursor.getColumnIndexOrThrow(Combined.BOOKMARK_ID);
        long id = cursor.getLong(bookmarkIdIndex);

        int displayIndex = cursor.getColumnIndexOrThrow(Combined.DISPLAY);
        int display = cursor.getInt(displayIndex);

        
        
        int visibility = (id == 0 ? View.GONE : View.VISIBLE);
        bookmarkIconView.setVisibility(visibility);

        if (display == Combined.DISPLAY_READER) {
            bookmarkIconView.setImageResource(R.drawable.ic_awesomebar_reader);
        } else {
            bookmarkIconView.setImageResource(R.drawable.ic_awesomebar_star);
        }
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
        mAllPagesTab.setUrlListener(listener);
    }

    public void destroy() {
        mAllPagesTab.destroy();

        if (mBookmarksAdapter != null) {
            Cursor bookmarksCursor = mBookmarksAdapter.getCursor();
            if (bookmarksCursor != null)
                bookmarksCursor.close();
        }

        if (mContentObserver != null)
            BrowserDB.unregisterContentObserver(mContentResolver, mContentObserver);
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
        return mInReadingList;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        
        
        if (ev.getAction() == MotionEvent.ACTION_DOWN)
            hideSoftInput(this);

        
        
        return false;
    }
}
