




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB.TopSitesCursorWrapper;
import org.mozilla.gecko.db.BrowserDB.URLColumns;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.support.v4.widget.CursorAdapter;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;

import java.util.Map;




public class TopSitesGridAdapter extends CursorAdapter {
    
    private Map<String, Thumbnail> mThumbnails;

    


    public static class Thumbnail {
        
        private final boolean isThumbnail;

        
        private final Bitmap bitmap;

        public Thumbnail(Bitmap bitmap, boolean isThumbnail) {
            this.bitmap = bitmap;
            this.isThumbnail = isThumbnail;
        }
    }

    public TopSitesGridAdapter(Context context, Cursor cursor) {
        super(context, cursor);
    }

    


    @Override
    protected void onContentChanged() {
        
        
        return;
    }

    




    public void updateThumbnails(Map<String, Thumbnail> thumbnails) {
        mThumbnails = thumbnails;
        notifyDataSetChanged();
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

        TopSitesGridItemView view = (TopSitesGridItemView) bindView;
        view.setTitle(title);
        view.setUrl(url);
        view.setPinned(pinned);

        
        if (TextUtils.isEmpty(url)) {
            view.displayThumbnail(R.drawable.top_site_add);
        } else {
            
            Thumbnail thumbnail = (mThumbnails != null ? mThumbnails.get(url) : null);
            if (thumbnail == null) {
                view.displayThumbnail(null);
            } else if (thumbnail.isThumbnail) {
                view.displayThumbnail(thumbnail.bitmap);
            } else {
                view.displayFavicon(thumbnail.bitmap);
            }
        }
    }

    


    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
        return new TopSitesGridItemView(context);
    }
}
