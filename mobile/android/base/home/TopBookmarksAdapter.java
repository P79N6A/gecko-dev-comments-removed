




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserDB.TopSitesCursorWrapper;
import org.mozilla.gecko.db.BrowserDB.URLColumns;

import android.content.Context;
import android.database.Cursor;
import android.support.v4.widget.CursorAdapter;
import android.view.View;
import android.view.ViewGroup;




public class TopBookmarksAdapter extends CursorAdapter {
    public TopBookmarksAdapter(Context context, Cursor cursor) {
        super(context, cursor);
    }

    


    @Override
    protected void onContentChanged () {
        
        
        return;
    }

    


    @Override
    public void bindView(View bindView, Context context, Cursor cursor) {
        String url = "";
        String title = "";
        boolean pinned = false;

        
        if (!cursor.isAfterLast()) {
            url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            title = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE));
            pinned = ((TopSitesCursorWrapper) cursor).isPinned();
        }

        TopBookmarkItemView view = (TopBookmarkItemView) bindView;
        view.setTitle(title);
        view.setUrl(url);
        view.setPinned(pinned);
    }

    


    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
        return new TopBookmarkItemView(context);
    }
}