




package org.mozilla.gecko.home;

import org.mozilla.gecko.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.BookmarksListAdapter.OnRefreshFolderListener;
import org.mozilla.gecko.home.HomeListView.HomeContextMenuInfo;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PinBookmarkDialog.OnBookmarkSelectedListener;
import org.mozilla.gecko.home.TopBookmarksAdapter.Thumbnail;
import org.mozilla.gecko.home.TopBookmarksView.OnPinBookmarkListener;
import org.mozilla.gecko.home.TopBookmarksView.TopBookmarksContextMenuInfo;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;




public class BookmarksPage extends HomeFragment {
    public static final String LOGTAG = "GeckoBookmarksPage";

    
    private static final int LOADER_ID_BOOKMARKS_LIST = 0;

    
    private static final int LOADER_ID_TOP_BOOKMARKS = 1;

    
    private static final int LOADER_ID_THUMBNAILS = 2;

    
    private static final String BOOKMARKS_FOLDER_KEY = "folder_id";

    
    private static final String THUMBNAILS_URLS_KEY = "urls";

    
    private BookmarksListView mList;

    
    private TopBookmarksView mTopBookmarks;

    
    private HomeBanner mBanner;

    
    private BookmarksListAdapter mListAdapter;

    
    private TopBookmarksAdapter mTopBookmarksAdapter;

    
    private CursorLoaderCallbacks mLoaderCallbacks;

    
    private ThumbnailsLoaderCallbacks mThumbnailsLoaderCallbacks;

    
    private PinBookmarkListener mPinBookmarkListener;

    
    private float mListTouchY = -1;

    
    private boolean mSnapBannerToTop;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.home_bookmarks_page, container, false);

        mList = (BookmarksListView) view.findViewById(R.id.bookmarks_list);

        mTopBookmarks = new TopBookmarksView(getActivity());
        mList.addHeaderView(mTopBookmarks);

        return view;
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

        mPinBookmarkListener = new PinBookmarkListener();

        mList.setTag(HomePager.LIST_TAG_BOOKMARKS);
        mList.setOnUrlOpenListener(listener);
        mList.setHeaderDividersEnabled(false);

        mTopBookmarks.setOnUrlOpenListener(listener);
        mTopBookmarks.setOnPinBookmarkListener(mPinBookmarkListener);

        registerForContextMenu(mList);
        registerForContextMenu(mTopBookmarks);

        mBanner = (HomeBanner) view.findViewById(R.id.home_banner);
        mList.setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                BookmarksPage.this.handleListTouchEvent(event);
                return false;
            }
        });
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final Activity activity = getActivity();

        
        mTopBookmarksAdapter = new TopBookmarksAdapter(activity, null);
        mTopBookmarks.setAdapter(mTopBookmarksAdapter);

        
        mListAdapter = new BookmarksListAdapter(activity, null);
        mListAdapter.setOnRefreshFolderListener(new OnRefreshFolderListener() {
            @Override
            public void onRefreshFolder(int folderId) {
                
                Bundle bundle = new Bundle();
                bundle.putInt(BOOKMARKS_FOLDER_KEY, folderId);
                getLoaderManager().restartLoader(LOADER_ID_BOOKMARKS_LIST, bundle, mLoaderCallbacks);
            }
        });
        mList.setAdapter(mListAdapter);

        
        
        BrowserDB.invalidateCachedState();

        
        mLoaderCallbacks = new CursorLoaderCallbacks(activity, getLoaderManager());
        mThumbnailsLoaderCallbacks = new ThumbnailsLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    public void onDestroyView() {
        mList = null;
        mListAdapter = null;
        mTopBookmarks = null;
        mTopBookmarksAdapter = null;
        mPinBookmarkListener = null;
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

    private void handleListTouchEvent(MotionEvent event) {
        
        if (mBanner.isDismissed()) {
            return;
        }

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                mListTouchY = event.getRawY();
                break;
             }

            case MotionEvent.ACTION_MOVE: {
                
                
                if (mListTouchY == -1) {
                    mListTouchY = event.getRawY();
                    return;
                }

                final float curY = event.getRawY();
                final float delta = mListTouchY - curY;
                mSnapBannerToTop = (delta > 0.0f) ? false : true;

                final float height = mBanner.getHeight();
                float newTranslationY = ViewHelper.getTranslationY(mBanner) + delta;

                
                if (newTranslationY < 0.0f) {
                    newTranslationY = 0.0f;
                } else if (newTranslationY > height) {
                    newTranslationY = height;
                }

                ViewHelper.setTranslationY(mBanner, newTranslationY);
                mListTouchY = curY;
                break;
            }

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL: {
                mListTouchY = -1;
                final float y = ViewHelper.getTranslationY(mBanner);
                final float height = mBanner.getHeight();
                if (y > 0.0f && y < height) {
                    final PropertyAnimator animator = new PropertyAnimator(100);
                    animator.attach(mBanner, Property.TRANSLATION_Y, mSnapBannerToTop ? 0 : height);
                    animator.start();
                }
                break;
            }
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        if (menuInfo == null) {
            return;
        }

        
        if (menuInfo instanceof HomeContextMenuInfo) {
            super.onCreateContextMenu(menu, view, menuInfo);
        }

        if (!(menuInfo instanceof TopBookmarksContextMenuInfo)) {
            return;
        }

        MenuInflater inflater = new MenuInflater(view.getContext());
        inflater.inflate(R.menu.top_bookmarks_contextmenu, menu);

        TopBookmarksContextMenuInfo info = (TopBookmarksContextMenuInfo) menuInfo;
        menu.setHeaderTitle(info.getDisplayTitle());

        if (!TextUtils.isEmpty(info.url)) {
            if (info.isPinned) {
                menu.findItem(R.id.top_bookmarks_pin).setVisible(false);
            } else {
                menu.findItem(R.id.top_bookmarks_unpin).setVisible(false);
            }
        } else {
            menu.findItem(R.id.top_bookmarks_open_new_tab).setVisible(false);
            menu.findItem(R.id.top_bookmarks_open_private_tab).setVisible(false);
            menu.findItem(R.id.top_bookmarks_pin).setVisible(false);
            menu.findItem(R.id.top_bookmarks_unpin).setVisible(false);
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        ContextMenuInfo menuInfo = item.getMenuInfo();

        
        if (menuInfo == null || !(menuInfo instanceof TopBookmarksContextMenuInfo)) {
            return false;
        }

        TopBookmarksContextMenuInfo info = (TopBookmarksContextMenuInfo) menuInfo;
        final Activity activity = getActivity();

        final int itemId = item.getItemId();
        if (itemId == R.id.top_bookmarks_open_new_tab || itemId == R.id.top_bookmarks_open_private_tab) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't open in new tab because URL is null");
                return false;
            }

            int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_BACKGROUND;
            if (item.getItemId() == R.id.top_bookmarks_open_private_tab)
                flags |= Tabs.LOADURL_PRIVATE;

            Tabs.getInstance().loadUrl(info.url, flags);
            Toast.makeText(activity, R.string.new_tab_opened, Toast.LENGTH_SHORT).show();
            return true;
        }

        if (itemId == R.id.top_bookmarks_pin) {
            final String url = info.url;
            final String title = info.title;
            final int position = info.position;
            final Context context = getActivity().getApplicationContext();

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    BrowserDB.pinSite(context.getContentResolver(), url, title, position);
                }
            });

            return true;
        }

        if (itemId == R.id.top_bookmarks_unpin) {
            final int position = info.position;
            final Context context = getActivity().getApplicationContext();

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    BrowserDB.unpinSite(context.getContentResolver(), position);
                }
            });

            return true;
        }

        if (itemId == R.id.top_bookmarks_edit) {
            mPinBookmarkListener.onPinBookmark(info.position);
            return true;
        }

        return false;
    }

    @Override
    protected void load() {
        final LoaderManager manager = getLoaderManager();
        manager.initLoader(LOADER_ID_BOOKMARKS_LIST, null, mLoaderCallbacks);
        manager.initLoader(LOADER_ID_TOP_BOOKMARKS, null, mLoaderCallbacks);
    }

    


    private class PinBookmarkListener implements OnPinBookmarkListener,
                                                 OnBookmarkSelectedListener {
        
        private static final String TAG_PIN_BOOKMARK = "pin_bookmark";

        
        private int mPosition;

        @Override
        public void onPinBookmark(int position) {
            mPosition = position;

            final FragmentManager manager = getActivity().getSupportFragmentManager();
            PinBookmarkDialog dialog = (PinBookmarkDialog) manager.findFragmentByTag(TAG_PIN_BOOKMARK);
            if (dialog == null) {
                dialog = PinBookmarkDialog.newInstance();
            }

            dialog.setOnBookmarkSelectedListener(this);
            dialog.show(manager, TAG_PIN_BOOKMARK);
        }

        @Override
        public void onBookmarkSelected(final String url, final String title) {
            final int position = mPosition;
            final Context context = getActivity().getApplicationContext();
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    BrowserDB.pinSite(context.getContentResolver(), url, title, position);
                }
            });
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
            return BrowserDB.getTopBookmarks(getContext().getContentResolver(), max);
        }
    }

    


    private class CursorLoaderCallbacks extends HomeCursorLoaderCallbacks {
        public CursorLoaderCallbacks(Context context, LoaderManager loaderManager) {
            super(context, loaderManager);
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            switch(id) {
                case LOADER_ID_BOOKMARKS_LIST: {
                    if (args == null) {
                        return new BookmarksLoader(getActivity());
                    } else {
                        return new BookmarksLoader(getActivity(), args.getInt(BOOKMARKS_FOLDER_KEY));
                    }
                }

                case LOADER_ID_TOP_BOOKMARKS: {
                    return new TopBookmarksLoader(getActivity());
                }

                default: {
                    return super.onCreateLoader(id, args);
                }
            }
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            final int loaderId = loader.getId();
            switch(loaderId) {
                case LOADER_ID_BOOKMARKS_LIST: {
                    mListAdapter.swapCursor(c);
                    loadFavicons(c);
                    mList.setHeaderDividersEnabled(c != null && c.getCount() > 0);
                    break;
                }

                case LOADER_ID_TOP_BOOKMARKS: {
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
                            getLoaderManager().restartLoader(LOADER_ID_THUMBNAILS, bundle, mThumbnailsLoaderCallbacks);
                        }
                    }
                    break;
                }

                default: {
                    super.onLoadFinished(loader, c);
                    break;
                }
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            final int loaderId = loader.getId();
            switch(loaderId) {
                case LOADER_ID_BOOKMARKS_LIST: {
                    if (mList != null) {
                        mListAdapter.swapCursor(null);
                    }
                    break;
                }

                case LOADER_ID_TOP_BOOKMARKS: {
                    if (mTopBookmarks != null) {
                        mTopBookmarksAdapter.swapCursor(null);
                        break;
                    }
                }

                default: {
                    super.onLoaderReset(loader);
                    break;
                }
            }
        }

        @Override
        public void onFaviconsLoaded() {
            mListAdapter.notifyDataSetChanged();
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
                        final Bitmap bitmap = (b == null ? null : BitmapUtils.decodeByteArray(b));

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
            if (mTopBookmarksAdapter != null) {
                mTopBookmarksAdapter.updateThumbnails(thumbnails);
            }
        }

        @Override
        public void onLoaderReset(Loader<Map<String, Thumbnail>> loader) {
            if (mTopBookmarksAdapter != null) {
                mTopBookmarksAdapter.updateThumbnails(null);
            }
        }
    }
}
