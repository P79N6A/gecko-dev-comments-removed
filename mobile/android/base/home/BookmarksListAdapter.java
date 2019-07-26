




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.util.Pair;
import android.view.View;

import java.util.Collections;
import java.util.List;
import java.util.LinkedList;




class BookmarksListAdapter extends MultiTypeCursorAdapter {
    private static final int VIEW_TYPE_ITEM = 0;
    private static final int VIEW_TYPE_FOLDER = 1;

    private static final int[] VIEW_TYPES = new int[] { VIEW_TYPE_ITEM, VIEW_TYPE_FOLDER };
    private static final int[] LAYOUT_TYPES = new int[] { R.layout.bookmark_item_row, R.layout.bookmark_folder_row };

    
    
    public static interface OnRefreshFolderListener {
        
        public void onRefreshFolder(int folderId);
    }

    
    
    private LinkedList<Pair<Integer, String>> mParentStack;

    
    private OnRefreshFolderListener mListener;

    public BookmarksListAdapter(Context context, Cursor cursor, List<Pair<Integer, String>> parentStack) {
        
        super(context, cursor, VIEW_TYPES, LAYOUT_TYPES);

        if (parentStack == null) {
            mParentStack = new LinkedList<Pair<Integer, String>>();

            
            Pair<Integer, String> rootFolder = new Pair<Integer, String>(Bookmarks.FIXED_ROOT_ID, "");
            mParentStack.addFirst(rootFolder);
        } else {
            mParentStack = new LinkedList<Pair<Integer, String>>(parentStack);
        }
    }

    public List<Pair<Integer, String>> getParentStack() {
        return Collections.unmodifiableList(mParentStack);
    }

    
    private void refreshCurrentFolder() {
        if (mListener != null) {
            mListener.onRefreshFolder(mParentStack.peek().first);
        }
    }

    




    public boolean moveToParentFolder() {
        
        if (mParentStack.size() == 1) {
            return false;
        }

        mParentStack.removeFirst();
        refreshCurrentFolder();
        return true;
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

        final Cursor c = getCursor(position);
        if (c.getInt(c.getColumnIndexOrThrow(Bookmarks.TYPE)) == Bookmarks.TYPE_FOLDER) {
            return VIEW_TYPE_FOLDER;
        }

        
        return VIEW_TYPE_ITEM;
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
    public void bindView(View view, Context context, int position) {
        final int viewType = getItemViewType(position);

        final Cursor cursor;
        if (isShowingChildFolder()) {
            if (position == 0) {
                cursor = null;
            } else {
                
                position--;
                cursor = getCursor(position);
            }
        } else {
            cursor = getCursor(position);
        }

        if (viewType == VIEW_TYPE_ITEM) {
            final TwoLinePageRow row = (TwoLinePageRow) view;
            row.updateFromCursor(cursor);
        } else {
            final BookmarkFolderView row = (BookmarkFolderView) view;
            if (cursor == null) {
                row.setText(mParentStack.peek().second);
                row.open();
            } else {
                row.setText(getFolderTitle(context, cursor));
                row.close();
            }
        }
    }
}
