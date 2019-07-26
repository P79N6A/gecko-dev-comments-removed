




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB.URLColumns;

import android.content.Context;
import android.database.Cursor;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MotionEvent;
import android.view.View;

import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ListView;





public class HomeListView extends ListView
                          implements OnItemLongClickListener {

    
    private HomeContextMenuInfo mContextMenuInfo;

    public HomeListView(Context context) {
        this(context, null);
    }

    public HomeListView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.listViewStyle);
    }

    public HomeListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setOnItemLongClickListener(this);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            
            requestFocus();
        }

        return false;
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        Cursor cursor = (Cursor) parent.getItemAtPosition(position);
        mContextMenuInfo = new HomeContextMenuInfo(view, position, id, cursor);
        return showContextMenuForChild(HomeListView.this);
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    


    public static class HomeContextMenuInfo extends AdapterContextMenuInfo {
        public int rowId;
        public String url;
        public byte[] favicon;
        public String title;
        public String keyword;
        public int display;
        public boolean isFolder;

        public HomeContextMenuInfo(View targetView, int position, long id, Cursor cursor) {
            super(targetView, position, id);

            if (cursor == null) {
                return;
            }

            final int typeCol = cursor.getColumnIndex(Bookmarks.TYPE);
            if (typeCol != -1) {
                isFolder = (cursor.getInt(typeCol) == Bookmarks.TYPE_FOLDER);
            } else {
                isFolder = false;
            }

            if (isFolder) {
                return;
            }

            int keywordCol = cursor.getColumnIndex(URLColumns.KEYWORD);
            if (keywordCol != -1) {
                keyword = cursor.getString(keywordCol);
            } else {
                keyword = null;
            }

            final int faviconCol = cursor.getColumnIndex(URLColumns.FAVICON);
            if (faviconCol != -1) {
                favicon = cursor.getBlob(faviconCol);
            } else {
                favicon = null;
            }

            rowId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
            url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            title = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE));
            display = Combined.DISPLAY_NORMAL;
        }
    }
}
