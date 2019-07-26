




package org.mozilla.gecko.home;

import org.mozilla.gecko.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.BookmarksListAdapter.OnRefreshFolderListener;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.TopBookmarksView.Thumbnail;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;




public class BookmarksPage extends HomeFragment {
    public static final String LOGTAG = "GeckoBookmarksPage";

    
    private static final int BOOKMARKS_LIST_LOADER_ID = 0;

    
    private static final int TOP_BOOKMARKS_LOADER_ID = 1;

    
    private static final int FAVICONS_LOADER_ID = 2;

    
    private static final int THUMBNAILS_LOADER_ID = 3;

    
    private static final String BOOKMARKS_FOLDER_KEY = "folder_id";

    
    private static final String THUMBNAILS_URLS_KEY = "urls";

    
    private BookmarksListView mList;

    
    private TopBookmarksView mTopBookmarks;

    
    private BookmarksListAdapter mListAdapter;

    
    private TopBookmarksAdapter mTopBookmarksAdapter;

    
    private CursorLoaderCallbacks mLoaderCallbacks;

    
    private ThumbnailsLoaderCallbacks mThumbnailsLoaderCallbacks;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        BookmarksListView list = (BookmarksListView) inflater.inflate(R.layout.home_bookmarks_page, container, false);

        mTopBookmarks = new TopBookmarksView(getActivity());
        list.addHeaderView(mTopBookmarks);

        return list;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        OnUrlOpenListener listener = null;
        try {
            listener = (OnUrlOpenListener) getActivity();
        } catch (ClassCastException e) {
            throw new ClassCastException(getActivity().toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }

        mList = (BookmarksListView) view.findViewById(R.id.bookmarks_list);
        mList.setOnUrlOpenListener(listener);

        registerForContextMenu(mList);

        mTopBookmarks.setOnUrlOpenListener(listener);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mTopBookmarksAdapter = new TopBookmarksAdapter(getActivity(), null);
        mTopBookmarks.setAdapter(mTopBookmarksAdapter);

        
        mListAdapter = new BookmarksListAdapter(getActivity(), null);
        mListAdapter.setOnRefreshFolderListener(new OnRefreshFolderListener() {
            @Override
            public void onRefreshFolder(int folderId) {
                
                Bundle bundle = new Bundle();
                bundle.putInt(BOOKMARKS_FOLDER_KEY, folderId);
                getLoaderManager().restartLoader(BOOKMARKS_LIST_LOADER_ID, bundle, mLoaderCallbacks);
            }
        });
        mList.setAdapter(mListAdapter);

        
        mLoaderCallbacks = new CursorLoaderCallbacks();
        mThumbnailsLoaderCallbacks = new ThumbnailsLoaderCallbacks();

        
        final LoaderManager manager = getLoaderManager();
        manager.initLoader(BOOKMARKS_LIST_LOADER_ID, null, mLoaderCallbacks);
        manager.initLoader(TOP_BOOKMARKS_LOADER_ID, null, mLoaderCallbacks);
    }

    @Override
    public void onDestroyView() {
        mList = null;
        mListAdapter = null;
        mTopBookmarks = null;
        mTopBookmarksAdapter = null;
        super.onDestroyView();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        
        
        
        
        
        
        
        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
    }

    


    private static class BookmarksLoader extends SimpleCursorLoader {
        private final int mFolderId;

        public BookmarksLoader(Context context) {
            this(context, Bookmarks.FIXED_ROOT_ID);
        }

        public BookmarksLoader(Context context, int folderId) {
            super(context);
            mFolderId = folderId;
        }

        @Override
        public Cursor loadCursor() {
            return BrowserDB.getBookmarksInFolder(getContext().getContentResolver(), mFolderId);
        }
    }

    


    private static class TopBookmarksLoader extends SimpleCursorLoader {
        public TopBookmarksLoader(Context context) {
            super(context);
        }

        @Override
        public Cursor loadCursor() {
            final int max = getContext().getResources().getInteger(R.integer.number_of_top_sites);
            return BrowserDB.getTopSites(getContext().getContentResolver(), max);
        }
    }

    


    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            switch(id) {
                case BOOKMARKS_LIST_LOADER_ID: {
                    if (args == null) {
                        return new BookmarksLoader(getActivity());
                    } else {
                        return new BookmarksLoader(getActivity(), args.getInt(BOOKMARKS_FOLDER_KEY));
                    }
                }

                case TOP_BOOKMARKS_LOADER_ID: {
                    return new TopBookmarksLoader(getActivity());
                }

                case FAVICONS_LOADER_ID: {
                    return FaviconsLoader.createInstance(getActivity(), args);
                }
            }

            return null;
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            final int loaderId = loader.getId();
            switch(loaderId) {
                case BOOKMARKS_LIST_LOADER_ID: {
                    mListAdapter.swapCursor(c);
                    FaviconsLoader.restartFromCursor(getLoaderManager(), FAVICONS_LOADER_ID,
                            mLoaderCallbacks, c);
                    break;
                }

                case TOP_BOOKMARKS_LOADER_ID: {
                    mTopBookmarksAdapter.swapCursor(c);

                    
                    if (c.getCount() > 0 && c.moveToFirst()) {
                        final ArrayList<String> urls = new ArrayList<String>();
                        do {
                            final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));
                            urls.add(url);
                        } while (c.moveToNext());

                        if (urls.size() > 0) {
                            Bundle bundle = new Bundle();
                            bundle.putStringArrayList(THUMBNAILS_URLS_KEY, urls);
                            getLoaderManager().restartLoader(THUMBNAILS_LOADER_ID, bundle, mThumbnailsLoaderCallbacks);
                        }
                    }
                    break;
                }

                case FAVICONS_LOADER_ID: {
                    
                    mListAdapter.notifyDataSetChanged();
                    break;
                }
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            final int loaderId = loader.getId();
            switch(loaderId) {
                case BOOKMARKS_LIST_LOADER_ID: {
                    if (mList != null) {
                        mListAdapter.swapCursor(null);
                    }
                    break;
                }

                case TOP_BOOKMARKS_LOADER_ID: {
                    if (mTopBookmarks != null) {
                        mTopBookmarksAdapter.swapCursor(null);
                        break;
                    }
                }

                case FAVICONS_LOADER_ID: {
                    
                    break;
                }
            }
        }
    }

    


    private static class ThumbnailsLoader extends AsyncTaskLoader<Map<String, Thumbnail>> {
        private Map<String, Thumbnail> mThumbnails;
        private ArrayList<String> mUrls;

        public ThumbnailsLoader(Context context, ArrayList<String> urls) {
            super(context);
            mUrls = urls;
        }

        @Override
        public Map<String, Thumbnail> loadInBackground() {
            if (mUrls == null || mUrls.size() == 0) {
                return null;
            }

            final Map<String, Thumbnail> thumbnails = new HashMap<String, Thumbnail>();

            
            final ContentResolver cr = getContext().getContentResolver();
            final Cursor cursor = BrowserDB.getThumbnailsForUrls(cr, mUrls);

            try {
                if (cursor != null && cursor.moveToFirst()) {
                    do {
                        
                        String url = cursor.getString(cursor.getColumnIndexOrThrow(Thumbnails.URL));
                        final byte[] b = cursor.getBlob(cursor.getColumnIndexOrThrow(Thumbnails.DATA));
                        final Bitmap bitmap = (b == null || b.length == 0 ? null : BitmapUtils.decodeByteArray(b));

                        if (bitmap != null) {
                            thumbnails.put(url, new Thumbnail(bitmap, true));
                        }
                    } while (cursor.moveToNext());
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }

            
            for (String url : mUrls) {
                if (!thumbnails.containsKey(url)) {
                    final Bitmap bitmap = BrowserDB.getFaviconForUrl(cr, url);
                    if (bitmap != null) {
                        
                        
                        thumbnails.put(url, new Thumbnail(Favicons.getInstance().scaleImage(bitmap), false));
                    }
                }
            }

            return thumbnails;
        }

        @Override
        public void deliverResult(Map<String, Thumbnail> thumbnails) {
            if (isReset()) {
                mThumbnails = null;
                return;
            }

            mThumbnails = thumbnails;

            if (isStarted()) {
                super.deliverResult(thumbnails);
            }
        }

        @Override
        protected void onStartLoading() {
            if (mThumbnails != null) {
                deliverResult(mThumbnails);
            }

            if (takeContentChanged() || mThumbnails == null) {
                forceLoad();
            }
        }

        @Override
        protected void onStopLoading() {
            cancelLoad();
        }

        @Override
        public void onCanceled(Map<String, Thumbnail> thumbnails) {
            mThumbnails = null;
        }

        @Override
        protected void onReset() {
            super.onReset();

            
            onStopLoading();

            mThumbnails = null;
        }
    }

    


    private class ThumbnailsLoaderCallbacks implements LoaderCallbacks<Map<String, Thumbnail>> {
        @Override
        public Loader<Map<String, Thumbnail>> onCreateLoader(int id, Bundle args) {
            return new ThumbnailsLoader(getActivity(), args.getStringArrayList(THUMBNAILS_URLS_KEY));
        }

        @Override
        public void onLoadFinished(Loader<Map<String, Thumbnail>> loader, Map<String, Thumbnail> thumbnails) {
            if (mTopBookmarks != null) {
                mTopBookmarks.updateThumbnails(thumbnails);
            }
        }

        @Override
        public void onLoaderReset(Loader<Map<String, Thumbnail>> loader) {
            if (mTopBookmarks != null) {
                mTopBookmarks.updateThumbnails(null);
            }
        }
    }
}
