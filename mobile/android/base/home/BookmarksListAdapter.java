




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.support.v4.widget.CursorAdapter;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.LinkedList;




class BookmarksListAdapter extends CursorAdapter {
    private static final int VIEW_TYPE_ITEM = 0;
    private static final int VIEW_TYPE_FOLDER = 1;

    private static final int VIEW_TYPE_COUNT = 2;

    
    
    public static interface OnRefreshFolderListener {
        
        public void onRefreshFolder(int folderId);
    }

    
    
    private LinkedList<Pair<Integer, String>> mParentStack;

    
    private OnRefreshFolderListener mListener;

    public BookmarksListAdapter(Context context, Cursor cursor) {
        
        super(context, cursor);

        mParentStack = new LinkedList<Pair<Integer, String>>();

        
        Pair<Integer, String> rootFolder = new Pair<Integer, String>(Bookmarks.FIXED_ROOT_ID, "");
        mParentStack.addFirst(rootFolder);
    }

    
    private void refreshCurrentFolder() {
        if (mListener != null) {
            mListener.onRefreshFolder(mParentStack.peek().first);
        }
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

    




    public void setOnRefreshFolderListener(OnRefreshFolderListener listener) {
        mListener = listener;
    }

    @Override
    public int getItemViewType(int position) {
        
        if (isShowingChildFolder()) {
            if (position == 0) {
                return VIEW_TYPE_FOLDER;
            }

            
            position--;
        }

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

    






    public String getFolderTitle(Context context, Cursor c) {
        String guid = c.getString(c.getColumnIndexOrThrow(Bookmarks.GUID));

        
        if (guid == null || guid.length() == 12) {
            return c.getString(c.getColumnIndexOrThrow(Bookmarks.TITLE));
        }

        Resources res = context.getResources();

        
        if (guid.equals(Bookmarks.FAKE_DESKTOP_FOLDER_GUID)) {
            return res.getString(R.string.bookmarks_folder_desktop);
        } else if (guid.equals(Bookmarks.MENU_FOLDER_GUID)) {
            return res.getString(R.string.bookmarks_folder_menu);
        } else if (guid.equals(Bookmarks.TOOLBAR_FOLDER_GUID)) {
            return res.getString(R.string.bookmarks_folder_toolbar);
        } else if (guid.equals(Bookmarks.UNFILED_FOLDER_GUID)) {
            return res.getString(R.string.bookmarks_folder_unfiled);
        }

        
        
        return c.getString(c.getColumnIndexOrThrow(Bookmarks.TITLE));
    }

    


    public boolean isShowingChildFolder() {
        return (mParentStack.peek().first != Bookmarks.FIXED_ROOT_ID);
    }

    @Override
    public int getCount() {
        return super.getCount() + (isShowingChildFolder() ? 1 : 0);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        
        if (isShowingChildFolder()) {
            if (position == 0) {
                BookmarkFolderView folder = (BookmarkFolderView) LayoutInflater.from(parent.getContext()).inflate(R.layout.bookmark_folder_row, null);
                folder.setText(mParentStack.peek().second);
                folder.open();
                return folder;
            }

            
            position--;
        }

        return super.getView(position, convertView, parent);
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
        final int viewType = getItemViewType(cursor);

        if (viewType == VIEW_TYPE_ITEM) {
            TwoLinePageRow row = (TwoLinePageRow) view;
            row.updateFromCursor(cursor);
        } else {
            BookmarkFolderView row = (BookmarkFolderView) view;
            row.setText(getFolderTitle(context, cursor));
            row.close();
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
