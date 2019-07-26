




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.TopSitesCursorWrapper;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.CursorAdapter;
import android.widget.GridView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;





public class TopBookmarksView extends GridView {
    private static final String LOGTAG = "GeckoTopBookmarksView";

    
    private int mMaxBookmarks;

    
    private int mNumColumns;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    protected TopBookmarksAdapter mAdapter;

    
    private Map<String, Bitmap> mThumbnailsCache;

    public TopBookmarksView(Context context) {
        this(context, null);
    }

    public TopBookmarksView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.topBookmarksViewStyle);
    }

    public TopBookmarksView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mMaxBookmarks = getResources().getInteger(R.integer.number_of_top_sites);
        mNumColumns = getResources().getInteger(R.integer.number_of_top_sites_cols);
        setNumColumns(mNumColumns);
    }

    


    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        
        mAdapter = new TopBookmarksAdapter(getContext(), null);
        setAdapter(mAdapter);

        setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TopBookmarkItemView row = (TopBookmarkItemView) view;
                String url = row.getUrl();

                if (mUrlOpenListener != null && !TextUtils.isEmpty(url)) {
                    mUrlOpenListener.onUrlOpen(url);
                }
            }
        });

        
        new LoadBookmarksTask().execute();
    }

    


    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (mAdapter != null) {
            setAdapter(null);
            final Cursor cursor = mAdapter.getCursor();

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                if (cursor != null && !cursor.isClosed())
                    cursor.close();
                }
            });
        }
    }

    


    @Override
    public int getColumnWidth() {
        
        
        return (getMeasuredWidth() - getPaddingLeft() - getPaddingRight()) / mNumColumns;
    }

    


    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        final int measuredWidth = getMeasuredWidth();
        final int childWidth = getColumnWidth();
        int childHeight = 0;

        
        ThumbnailHelper.getInstance().setThumbnailWidth(childWidth);

        
        final TopBookmarksAdapter adapter = (TopBookmarksAdapter) getAdapter();
        final int count = (adapter == null ? 0 : adapter.getCount());

        if (adapter != null && count > 0) {
            
            final View child = adapter.getView(0, null, this);
            if (child != null) {
                
                AbsListView.LayoutParams params = (AbsListView.LayoutParams) child.getLayoutParams();
                if (params == null) {
                    params = new AbsListView.LayoutParams(AbsListView.LayoutParams.WRAP_CONTENT,
                                                          AbsListView.LayoutParams.WRAP_CONTENT);
                    child.setLayoutParams(params);
                }

                
                
                int childWidthSpec = MeasureSpec.makeMeasureSpec(childWidth, MeasureSpec.EXACTLY);
                int childHeightSpec = MeasureSpec.makeMeasureSpec(0,  MeasureSpec.UNSPECIFIED);
                child.measure(childWidthSpec, childHeightSpec);
                childHeight = child.getMeasuredHeight();
            }
        }

        
        final int total = Math.min(count > 0 ? count : Integer.MAX_VALUE, mMaxBookmarks);

        
        final int rows = (int) Math.ceil((double) total / mNumColumns);
        final int childrenHeight = childHeight * rows;

        
        final int measuredHeight = childrenHeight + getPaddingTop() + getPaddingBottom();
        setMeasuredDimension(measuredWidth, measuredHeight);
    }

    


    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        
        if (mThumbnailsCache != null) {
            updateThumbnails(mThumbnailsCache);
            mThumbnailsCache = null;
        }
    }

    




    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    




    private void updateThumbnails(Map<String, Bitmap> thumbnails) {
        final int count = mAdapter.getCount();
        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);

            
            
            if (child == null) {
                continue;
            }

            TopBookmarkItemView view = (TopBookmarkItemView) child;
            final String url = view.getUrl();

            
            if (TextUtils.isEmpty(url)) {
                view.displayThumbnail(R.drawable.abouthome_thumbnail_add);
            } else {
                
                Bitmap bitmap = (thumbnails != null ? thumbnails.get(url) : null);
                view.displayThumbnail(bitmap);
            }
        }
    }

    


    public class TopBookmarksAdapter extends CursorAdapter {
        public TopBookmarksAdapter(Context context, Cursor cursor) {
            super(context, cursor);
        }

        


        @Override
        public int getCount() {
            return Math.min(super.getCount(), mMaxBookmarks);
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

    


    private class LoadBookmarksTask extends UiAsyncTask<Void, Void, Cursor> {
        public LoadBookmarksTask() {
            super(ThreadUtils.getBackgroundHandler());
        }

        @Override
        protected Cursor doInBackground(Void... params) {
            return BrowserDB.getTopSites(getContext().getContentResolver(), mMaxBookmarks);
        }

        @Override
        public void onPostExecute(Cursor cursor) {
            
            mAdapter.changeCursor(cursor);

            
            if (mAdapter.getCount() > 0) {
                new LoadThumbnailsTask().execute(cursor);
            }
        }
    }

    


    private class LoadThumbnailsTask extends UiAsyncTask<Cursor, Void, Map<String,Bitmap>> {
        public LoadThumbnailsTask() {
            super(ThreadUtils.getBackgroundHandler());
        }

        @Override
        protected Map<String, Bitmap> doInBackground(Cursor... params) {
            
            final Cursor adapterCursor = params[0];
            if (adapterCursor == null || !adapterCursor.moveToFirst()) {
                return null;
            }

            final List<String> urls = new ArrayList<String>();
            do {
                final String url = adapterCursor.getString(adapterCursor.getColumnIndexOrThrow(URLColumns.URL));
                urls.add(url);
            } while(adapterCursor.moveToNext());

            if (urls.size() == 0) {
                return null;
            }

            Map<String, Bitmap> thumbnails = new HashMap<String, Bitmap>();
            Cursor cursor = BrowserDB.getThumbnailsForUrls(getContext().getContentResolver(), urls);
            if (cursor == null || !cursor.moveToFirst()) {
                return null;
            }

            try {
                do {
                    final String url = cursor.getString(cursor.getColumnIndexOrThrow(Thumbnails.URL));
                    final byte[] b = cursor.getBlob(cursor.getColumnIndexOrThrow(Thumbnails.DATA));
                    if (b == null) {
                        continue;
                    }

                    Bitmap thumbnail = BitmapUtils.decodeByteArray(b);
                    if (thumbnail == null) {
                        continue;
                    }

                    thumbnails.put(url, thumbnail);
                } while (cursor.moveToNext());
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }

            return thumbnails;
        }

        @Override
        public void onPostExecute(Map<String, Bitmap> thumbnails) {
            
            
            if (isLayoutRequested()) {
                mThumbnailsCache = thumbnails;
            } else {
                updateThumbnails(thumbnails);
            }
        }
    }
}
