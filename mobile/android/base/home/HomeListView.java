




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.URLColumns;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.StringUtils;

import android.content.Context;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AbsListView.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ListView;





public class HomeListView extends ListView
                          implements OnItemLongClickListener {

    
    private HomeContextMenuInfo mContextMenuInfo;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private boolean mShowTopDivider;

    public HomeListView(Context context) {
        this(context, null);
    }

    public HomeListView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.homeListViewStyle);
    }

    public HomeListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.HomeListView, defStyle, 0);
        mShowTopDivider = a.getBoolean(R.styleable.HomeListView_topDivider, false);
        a.recycle();

        setOnItemLongClickListener(this);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        final Drawable divider = getDivider();
        if (mShowTopDivider && divider != null) {
            final int dividerHeight = getDividerHeight();
            final View view = new View(getContext());
            view.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, dividerHeight));
            addHeaderView(view);
        }
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        mUrlOpenListener = null;
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        Object item = parent.getItemAtPosition(position);

        
        if (item instanceof Cursor) {
            Cursor cursor = (Cursor) item;
            mContextMenuInfo = new HomeContextMenuInfo(view, position, id, cursor);
            return showContextMenuForChild(HomeListView.this);
        } else {
            mContextMenuInfo = null;
            return false;
        }
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    @Override
    public void setOnItemClickListener(final AdapterView.OnItemClickListener listener) {
        super.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (mShowTopDivider) {
                    position--;
                }

                listener.onItemClick(parent, view, position, id);
            }
        });
    }

    public OnUrlOpenListener getOnUrlOpenListener() {
        return mUrlOpenListener;
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    


    public static class HomeContextMenuInfo extends AdapterContextMenuInfo {

        public int bookmarkId;
        public int historyId;
        public String url;
        public String title;
        public int display;
        public boolean isFolder;
        public boolean inReadingList;

        



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

            url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            title = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE));

            final int bookmarkIdCol = cursor.getColumnIndex(Combined.BOOKMARK_ID);
            if (bookmarkIdCol == -1) {
                
                
                bookmarkId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
            } else if (cursor.isNull(bookmarkIdCol)) {
                
                
                bookmarkId = -1;
            } else {
                bookmarkId = cursor.getInt(bookmarkIdCol);
            }

            final int historyIdCol = cursor.getColumnIndex(Combined.HISTORY_ID);
            if (historyIdCol != -1) {
                historyId = cursor.getInt(historyIdCol);
            } else {
                historyId = -1;
            }

            
            final int parentCol = cursor.getColumnIndex(Bookmarks.PARENT);
            if (parentCol != -1) {
                inReadingList = (cursor.getInt(parentCol) == Bookmarks.FIXED_READING_LIST_ID);
            } else {
                inReadingList = false;
            }

            final int displayCol = cursor.getColumnIndex(Combined.DISPLAY);
            if (displayCol != -1) {
                display = cursor.getInt(displayCol);
            } else {
                display = Combined.DISPLAY_NORMAL;
            }
        }

        public String getDisplayTitle() {
            return TextUtils.isEmpty(title) ?
                StringUtils.stripCommonSubdomains(StringUtils.stripScheme(url, StringUtils.UrlFlags.STRIP_HTTPS)) : title;
        }
    }
}
