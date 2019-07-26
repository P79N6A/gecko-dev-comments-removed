




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
import android.database.Cursor;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.widget.AdapterView;
import android.widget.HeaderViewListAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;

import java.util.EnumSet;




public class BookmarksListView extends HomeListView
                               implements AdapterView.OnItemClickListener{
    public static final String LOGTAG = "GeckoBookmarksListView";

    
    private MotionEvent mMotionEvent;

    
    private int mTouchSlop;

    public BookmarksListView(Context context) {
        this(context, null);
    }

    public BookmarksListView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.bookmarksListViewStyle);
    }

    public BookmarksListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        
        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnItemClickListener(this);

        setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                
                if (keyCode == KeyEvent.KEYCODE_BACK) {
                    return getBookmarksListAdapter().moveToParentFolder();
                }
                return false;
            }
        });
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

        if (position < headerCount) {
            
            return;
        }

        
        position -= headerCount;

        final BookmarksListAdapter adapter = getBookmarksListAdapter();
        if (adapter.isShowingChildFolder()) {
            if (position == 0) {
                
                adapter.moveToParentFolder();
                return;
            }

            
            position--;
        }

        final Cursor cursor = adapter.getCursor();
        if (cursor == null) {
            return;
        }

        cursor.moveToPosition(position);

        int type = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks.TYPE));
        if (type == Bookmarks.TYPE_FOLDER) {
            
            final int folderId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
            final String folderTitle = adapter.getFolderTitle(parent.getContext(), cursor);
            adapter.moveToChildFolder(folderId, folderTitle);
        } else {
            
            final String url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));

            
            getOnUrlOpenListener().onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
        }
    }

    private BookmarksListAdapter getBookmarksListAdapter() {
        BookmarksListAdapter adapter;
        ListAdapter listAdapter = getAdapter();
        if (listAdapter instanceof HeaderViewListAdapter) {
            adapter = (BookmarksListAdapter) ((HeaderViewListAdapter) listAdapter).getWrappedAdapter();
        } else {
            adapter = (BookmarksListAdapter) listAdapter;
        }
        return adapter;
    }
}
