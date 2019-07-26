




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.Parcel;
import android.os.Parcelable;
import android.view.View;

import java.util.Collections;
import java.util.List;
import java.util.LinkedList;




class BookmarksListAdapter extends MultiTypeCursorAdapter {
    private static final int VIEW_TYPE_ITEM = 0;
    private static final int VIEW_TYPE_FOLDER = 1;

    private static final int[] VIEW_TYPES = new int[] { VIEW_TYPE_ITEM, VIEW_TYPE_FOLDER };
    private static final int[] LAYOUT_TYPES = new int[] { R.layout.bookmark_item_row, R.layout.bookmark_folder_row };

    public enum RefreshType implements Parcelable {
        PARENT,
        CHILD;

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(ordinal());
        }

        public static final Creator<RefreshType> CREATOR = new Creator<RefreshType>() {
            @Override
            public RefreshType createFromParcel(final Parcel source) {
                return RefreshType.values()[source.readInt()];
            }

            @Override
            public RefreshType[] newArray(final int size) {
                return new RefreshType[size];
            }
        };
    }

    public static class FolderInfo implements Parcelable {
        public final int id;
        public final String title;

        public FolderInfo(int id) {
            this(id, "");
        }

        public FolderInfo(Parcel in) {
            this(in.readInt(), in.readString());
        }

        public FolderInfo(int id, String title) {
            this.id = id;
            this.title = title;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(id);
            dest.writeString(title);
        }

        public static final Creator<FolderInfo> CREATOR = new Creator<FolderInfo>() {
            public FolderInfo createFromParcel(Parcel in) {
                return new FolderInfo(in);
            }

            public FolderInfo[] newArray(int size) {
                return new FolderInfo[size];
            }
        };
    }

    
    
    public static interface OnRefreshFolderListener {
        
        public void onRefreshFolder(FolderInfo folderInfo, RefreshType refreshType);
    }

    
    
    private LinkedList<FolderInfo> mParentStack;

    
    private OnRefreshFolderListener mListener;

    public BookmarksListAdapter(Context context, Cursor cursor, List<FolderInfo> parentStack) {
        
        super(context, cursor, VIEW_TYPES, LAYOUT_TYPES);

        if (parentStack == null) {
            mParentStack = new LinkedList<FolderInfo>();
        } else {
            mParentStack = new LinkedList<FolderInfo>(parentStack);
        }
    }

    public List<FolderInfo> getParentStack() {
        return Collections.unmodifiableList(mParentStack);
    }

    




    public boolean moveToParentFolder() {
        
        if (mParentStack.size() == 1) {
            return false;
        }

        if (mListener != null) {
            
            
            mListener.onRefreshFolder(mParentStack.get(1), RefreshType.PARENT);
        }

        return true;
    }

    





    public void moveToChildFolder(int folderId, String folderTitle) {
        FolderInfo folderInfo = new FolderInfo(folderId, folderTitle);

        if (mListener != null) {
            mListener.onRefreshFolder(folderInfo, RefreshType.CHILD);
        }
    }

    




    public void setOnRefreshFolderListener(OnRefreshFolderListener listener) {
        mListener = listener;
    }

    public void swapCursor(Cursor c, FolderInfo folderInfo, RefreshType refreshType) {
        switch(refreshType) {
            case PARENT:
                mParentStack.removeFirst();
                break;

            case CHILD:
                mParentStack.addFirst(folderInfo);
                break;

            default:
                
        }

        swapCursor(c);
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
        if (mParentStack.size() == 0) {
            return false;
        }

        return (mParentStack.peek().id != Bookmarks.FIXED_ROOT_ID);
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
                row.setText(mParentStack.peek().title);
                row.open();
            } else {
                row.setText(getFolderTitle(context, cursor));
                row.close();
            }
        }
    }
}
