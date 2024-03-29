




package org.mozilla.gecko.home;

import java.util.EnumSet;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
import android.database.Cursor;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.HeaderViewListAdapter;
import android.widget.ListAdapter;




public class BookmarksListView extends HomeListView
                               implements AdapterView.OnItemClickListener{
    public static final String LOGTAG = "GeckoBookmarksListView";

    public BookmarksListView(Context context) {
        this(context, null);
    }

    public BookmarksListView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.bookmarksListViewStyle);
    }

    public BookmarksListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnItemClickListener(this);

        setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                final int action = event.getAction();

                
                if (action == KeyEvent.ACTION_UP && keyCode == KeyEvent.KEYCODE_BACK) {
                    return getBookmarksListAdapter().moveToParentFolder();
                }
                return false;
            }
        });
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
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
            
            final String url = cursor.getString(cursor.getColumnIndexOrThrow(Bookmarks.URL));

            Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM);

            
            getOnUrlOpenListener().onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
        }
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        
        
        final BookmarksListAdapter adapter = getBookmarksListAdapter();
        if (adapter.isShowingChildFolder()) {
            position--;
        }

        return super.onItemLongClick(parent, view, position, id);
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
