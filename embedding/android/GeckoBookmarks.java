




































package org.mozilla.gecko;

import android.app.ListActivity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Browser;
import android.util.Log;
import android.view.View;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;

public class GeckoBookmarks extends ListActivity {
    private static final String LOG_NAME = "GeckoBookmarks";
    private static final String TITLE_KEY = "title";
    private static final String kBookmarksWhereClause = Browser.BookmarkColumns.BOOKMARK + " = 1";

    private Cursor mCursor;
    private Uri mUri;
    private String mTitle;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.bookmarks);
        mCursor = managedQuery(Browser.BOOKMARKS_URI,
                               null, kBookmarksWhereClause, null, null);
        startManagingCursor(mCursor);

        ListAdapter adapter =
          new SimpleCursorAdapter(this, R.layout.bookmark_list_row, mCursor,
        		      new String[] {Browser.BookmarkColumns.TITLE,
        				    Browser.BookmarkColumns.URL},
        		      new int[] {R.id.bookmark_title, R.id.bookmark_url});
        setListAdapter(adapter);
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        mCursor.moveToPosition(position);
        String spec = mCursor.getString(mCursor.getColumnIndex(Browser.BookmarkColumns.URL));
        Log.i(LOG_NAME, "clicked: " + spec);
        Intent intent = new Intent(this, GeckoApp.class);
        intent.setAction(Intent.ACTION_VIEW);
        intent.setData(Uri.parse(spec));
        startActivity(intent);
    }

    public void addBookmark(View v) {
        Browser.saveBookmark(this, mTitle, mUri.toString());
        finish();
    }

    @Override
    protected void onNewIntent(Intent intent) {
      
      mUri = intent.getData();
      mTitle = intent.getStringExtra(TITLE_KEY);
    }
}
