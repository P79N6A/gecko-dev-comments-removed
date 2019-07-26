




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.database.Cursor;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.CursorAdapter;
import android.widget.ListView;

import java.util.LinkedList;




public class BookmarksListView extends HomeListView
                               implements AdapterView.OnItemClickListener{
    
    public static final String LOGTAG = "GeckoBookmarksListView";

    private int mFolderId = Bookmarks.FIXED_ROOT_ID;
    private String mFolderTitle = "";

    
    private BookmarksListAdapter mCursorAdapter = null;

    
    private BookmarksQueryTask mQueryTask = null;

    
    private BookmarkFolderView mFolderView;

    
    private boolean mHasFolderHeader = false;

    
    private MotionEvent mMotionEvent;

    
    private int mTouchSlop;

    public BookmarksListView(Context context) {
        this(context, null);
    }

    public BookmarksListView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.listViewStyle);
    }

    public BookmarksListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        
        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();

        
        mFolderView = (BookmarkFolderView) LayoutInflater.from(context).inflate(R.layout.bookmark_folder_row, null);
        mFolderView.open();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        
        mCursorAdapter = new BookmarksListAdapter(getContext(), null);

        
        refreshListWithCursor(null);

        setOnItemClickListener(this);
        setOnKeyListener(GamepadUtils.getListItemClickDispatcher());

        mQueryTask = new BookmarksQueryTask();
        mQueryTask.execute();
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        
        if (mCursorAdapter != null) {
            final Cursor cursor = mCursorAdapter.getCursor();
            mCursorAdapter = null;

            
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    if (cursor != null && !cursor.isClosed())
                        cursor.close();
                }
            });
        }
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        switch(event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN: {
                
                mMotionEvent = MotionEvent.obtain(event);
                break;
            }

            case MotionEvent.ACTION_MOVE: {
                if ((mMotionEvent != null) &&
                    (Math.abs(event.getY() - mMotionEvent.getY()) > mTouchSlop)) {
                    
                    
                    onTouchEvent(mMotionEvent);
                    return true;
                }
                break;
            }

            default: {
                mMotionEvent = null;
                break;
            }
        }

        
        return super.onInterceptTouchEvent(event);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        final ListView list = (ListView) parent;
        final int headerCount = list.getHeaderViewsCount();

        if (mHasFolderHeader) {
            
            
            if (position == headerCount - 1) {   
                mCursorAdapter.moveToParentFolder();
                return;
            }
        } else if (position < headerCount) {
            
            return;
        }

        final Cursor cursor = mCursorAdapter.getCursor();
        if (cursor == null) {
            return;
        }

        
        position -= headerCount;

        cursor.moveToPosition(position);

        int type = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks.TYPE));
        if (type == Bookmarks.TYPE_FOLDER) {
            
            final int folderId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
            final String folderTitle = mCursorAdapter.getFolderTitle(cursor);
            mCursorAdapter.moveToChildFolder(folderId, folderTitle);
        } else {
            
            final String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            OnUrlOpenListener listener = getOnUrlOpenListener();
            if (listener != null) {
                listener.onUrlOpen(url);
            }
        }
    }

    private void refreshListWithCursor(Cursor cursor) {
        
        setAdapter(null);

        
        if (mFolderId == Bookmarks.FIXED_ROOT_ID) {
            if (mHasFolderHeader) {
                removeHeaderView(mFolderView);
                mHasFolderHeader = false;
            }
        } else {
            if (!mHasFolderHeader) {
                addHeaderView(mFolderView, null, true);
                mHasFolderHeader = true;
            }

            mFolderView.setText(mFolderTitle);
        }

        
        mCursorAdapter.changeCursor(cursor);
        setAdapter(mCursorAdapter);

        
        mQueryTask = null;
    }

    


    private class BookmarksListAdapter extends CursorAdapter {
        private static final int VIEW_TYPE_ITEM = 0;
        private static final int VIEW_TYPE_FOLDER = 1;

        private static final int VIEW_TYPE_COUNT = 2;

        
        
        private LinkedList<Pair<Integer, String>> mParentStack;

        public BookmarksListAdapter(Context context, Cursor cursor) {
            
            super(context, cursor);

            mParentStack = new LinkedList<Pair<Integer, String>>();

            
            Pair<Integer, String> rootFolder = new Pair<Integer, String>(mFolderId, mFolderTitle);
            mParentStack.addFirst(rootFolder);
        }

        
        private void refreshCurrentFolder() {
            
            if (mQueryTask != null) {
                mQueryTask.cancel(false);
            }

            Pair<Integer, String> folderPair = mParentStack.getFirst();
            mFolderId = folderPair.first;
            mFolderTitle = folderPair.second;

            mQueryTask = new BookmarksQueryTask();
            mQueryTask.execute(new Integer(mFolderId));
        }

        


        public void moveToParentFolder() {
            
            if (mParentStack.size() != 1) {
                mParentStack.removeFirst();
                refreshCurrentFolder();
            }
        }

        





        public void moveToChildFolder(int folderId, String folderTitle) {
            Pair<Integer, String> folderPair = new Pair<Integer, String>(folderId, folderTitle);
            mParentStack.addFirst(folderPair);
            refreshCurrentFolder();
        }

        


        @Override
        public int getItemViewType(int position) {
            Cursor c = getCursor();

            if (!c.moveToPosition(position)) {
                throw new IllegalStateException("Couldn't move cursor to position " + position);
            }

            return getItemViewType(c);
        }

        





        public int getItemViewType(Cursor cursor) {
            if (cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks.TYPE)) == Bookmarks.TYPE_FOLDER) {
                return VIEW_TYPE_FOLDER;
            }

            
            return VIEW_TYPE_ITEM;
        }

        


        @Override
        public int getViewTypeCount() {
            return VIEW_TYPE_COUNT;
        }

        





        public String getFolderTitle(Cursor c) {
            String guid = c.getString(c.getColumnIndexOrThrow(Bookmarks.GUID));

            
            if (guid == null || guid.length() == 12) {
                return c.getString(c.getColumnIndexOrThrow(Bookmarks.TITLE));
            }

            
            if (guid.equals(Bookmarks.FAKE_DESKTOP_FOLDER_GUID)) {
                return getResources().getString(R.string.bookmarks_folder_desktop);
            } else if (guid.equals(Bookmarks.MENU_FOLDER_GUID)) {
                return getResources().getString(R.string.bookmarks_folder_menu);
            } else if (guid.equals(Bookmarks.TOOLBAR_FOLDER_GUID)) {
                return getResources().getString(R.string.bookmarks_folder_toolbar);
            } else if (guid.equals(Bookmarks.UNFILED_FOLDER_GUID)) {
                return getResources().getString(R.string.bookmarks_folder_unfiled);
            }

            
            
            return c.getString(c.getColumnIndexOrThrow(Bookmarks.TITLE));
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            final int viewType = getItemViewType(cursor);

            if (viewType == VIEW_TYPE_ITEM) {
                TwoLinePageRow row = (TwoLinePageRow) view;
                row.updateFromCursor(cursor);
            } else {
                BookmarkFolderView row = (BookmarkFolderView) view;
                row.setText(getFolderTitle(cursor));
            }
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            final int viewType = getItemViewType(cursor);

            final int resId;
            if (viewType == VIEW_TYPE_ITEM) {
                resId = R.layout.home_item_row;
            } else {
                resId = R.layout.bookmark_folder_row;
            }

            return LayoutInflater.from(parent.getContext()).inflate(resId, null);
        }
    }

    


    private class BookmarksQueryTask extends AsyncTask<Integer, Void, Cursor> {
        @Override
        protected Cursor doInBackground(Integer... folderIds) {
            int folderId = Bookmarks.FIXED_ROOT_ID;

            if (folderIds.length != 0) {
                folderId = folderIds[0].intValue();
            }

            return BrowserDB.getBookmarksInFolder(getContext().getContentResolver(), folderId);
        }

        @Override
        protected void onPostExecute(final Cursor cursor) {
            refreshListWithCursor(cursor);
        }
    }
}
