




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.home.HomeListView.HomeContextMenuInfo;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.GamepadUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Pair;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;

import java.util.LinkedList;




public class BookmarksPage extends HomeFragment {
    public static final String LOGTAG = "GeckoBookmarksPage";

    private int mFolderId = Bookmarks.FIXED_ROOT_ID;
    private String mFolderTitle = "";

    private BookmarksListAdapter mCursorAdapter = null;
    private BookmarksQueryTask mQueryTask = null;

    
    private HomeListView mList;

    
    private BookmarkFolderView mFolderView;

    
    private OnUrlOpenListener mUrlOpenListener;

    public BookmarksPage() {
        mUrlOpenListener = null;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }

        
        mCursorAdapter = new BookmarksListAdapter(getActivity());
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUrlOpenListener = null;

        
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
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        super.onCreateView(inflater, container, savedInstanceState);

        
        
        mList = new HomeListView(container.getContext());
        return mList;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        
        mFolderView = (BookmarkFolderView) LayoutInflater.from(getActivity()).inflate(R.layout.bookmark_folder_row, null);
        mFolderView.open();

        
        refreshListWithCursor(null);

        EventHandlers eventHandlers = new EventHandlers();
        mList.setOnTouchListener(eventHandlers);
        mList.setOnItemClickListener(eventHandlers);
        mList.setOnCreateContextMenuListener(eventHandlers);
        mList.setOnKeyListener(GamepadUtils.getListItemClickDispatcher());

        mQueryTask = new BookmarksQueryTask();
        mQueryTask.execute();
    }

    @Override
    public void onDestroyView() {
        mList = null;
        mFolderView = null;
        super.onDestroyView();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        
        
        
        
        
        
        
        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
    }

    private void refreshListWithCursor(Cursor cursor) {
        
        mList.setAdapter(null);

        
        if (mFolderId == Bookmarks.FIXED_ROOT_ID) {
            if (mList.getHeaderViewsCount() == 1) {
                mList.removeHeaderView(mFolderView);
            }
        } else {
            if (mList.getHeaderViewsCount() == 0) {
                mList.addHeaderView(mFolderView, null, true);
            }

            mFolderView.setText(mFolderTitle);
        }

        
        mCursorAdapter.changeCursor(cursor);
        mList.setAdapter(mCursorAdapter);

        
        mQueryTask = null;
    }

    


    private class EventHandlers implements AdapterView.OnItemClickListener,
                                           View.OnCreateContextMenuListener,
                                           View.OnTouchListener {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            final ListView list = (ListView) parent;
            final int headerCount = list.getHeaderViewsCount();

            
            if (headerCount == 1 && position == 0) {
                mCursorAdapter.moveToParentFolder();
                return;
            }

            Cursor cursor = mCursorAdapter.getCursor();
            if (cursor == null) {
                return;
            }

            
            if (headerCount == 1) {
                position--;
            }

            cursor.moveToPosition(position);

            int type = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks.TYPE));
            if (type == Bookmarks.TYPE_FOLDER) {
                
                int folderId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
                String folderTitle = mCursorAdapter.getFolderTitle(position);
                mCursorAdapter.moveToChildFolder(folderId, folderTitle);
             } else {
                
                String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
                if (mUrlOpenListener != null) {
                    mUrlOpenListener.onUrlOpen(url);
                }
             }
         }

        @Override
        public boolean onTouch(View view, MotionEvent event) {
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                
                view.requestFocus();
            }
            return false;
        }

        @Override
        public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
            if (!(menuInfo instanceof HomeContextMenuInfo)) {
                return;
            }

            HomeContextMenuInfo info = (HomeContextMenuInfo) menuInfo;

            
            if (info.isFolder) {
                return;
            }

            MenuInflater inflater = new MenuInflater(view.getContext());
            inflater.inflate(R.menu.awesomebar_contextmenu, menu);

            
            boolean isPrivate = false;
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                isPrivate = tab.isPrivate();
            }
            menu.findItem(R.id.open_new_tab).setVisible(!isPrivate);
            menu.findItem(R.id.open_private_tab).setVisible(isPrivate);

            
            if (info.rowId < 0) {
                menu.findItem(R.id.remove_history).setVisible(false);
            }
            menu.setHeaderTitle(info.title);
 
            menu.findItem(R.id.remove_history).setVisible(false);
            menu.findItem(R.id.open_in_reader).setVisible(false);
            return;
        }
    }

    


    private class BookmarksListAdapter extends SimpleCursorAdapter {
        private static final int VIEW_TYPE_ITEM = 0;
        private static final int VIEW_TYPE_FOLDER = 1;

        private static final int VIEW_TYPE_COUNT = 2;

        
        
        private LinkedList<Pair<Integer, String>> mParentStack;

        public BookmarksListAdapter(Context context) {
            
            super(context, -1, null, new String[] {}, new int[] {});

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

            if (c.moveToPosition(position) &&
                c.getInt(c.getColumnIndexOrThrow(Bookmarks.TYPE)) == Bookmarks.TYPE_FOLDER) {
                return VIEW_TYPE_FOLDER;
            }

            
            return VIEW_TYPE_ITEM;
        }

        


        @Override
        public int getViewTypeCount() {
            return VIEW_TYPE_COUNT;
        }

        





        public String getFolderTitle(int position) {
            Cursor c = getCursor();
            if (!c.moveToPosition(position)) {
                return "";
            }

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
        public View getView(int position, View convertView, ViewGroup parent) {
            final int viewType = getItemViewType(position);

            if (convertView == null) {
                if (viewType == VIEW_TYPE_ITEM) {
                    convertView = LayoutInflater.from(parent.getContext()).inflate(R.layout.home_item_row, null);
                } else {
                    convertView = LayoutInflater.from(parent.getContext()).inflate(R.layout.bookmark_folder_row, null);
                }
            }

            Cursor cursor = getCursor();
            if (!cursor.moveToPosition(position)) {
                throw new IllegalStateException("Couldn't move cursor to position " + position);
            }

            if (viewType == VIEW_TYPE_ITEM) {
                TwoLinePageRow row = (TwoLinePageRow) convertView;
                row.updateFromCursor(cursor);
            } else {
                BookmarkFolderView row = (BookmarkFolderView) convertView;
                row.setText(getFolderTitle(position));
            }

            return convertView;
        }
    }

    


    private class BookmarksQueryTask extends AsyncTask<Integer, Void, Cursor> {
        @Override
        protected Cursor doInBackground(Integer... folderIds) {
            int folderId = Bookmarks.FIXED_ROOT_ID;

            if (folderIds.length != 0) {
                folderId = folderIds[0].intValue();
            }

            return BrowserDB.getBookmarksInFolder(getActivity().getContentResolver(), folderId);
        }

        @Override
        protected void onPostExecute(final Cursor cursor) {
            refreshListWithCursor(cursor);
        }
    }
}
