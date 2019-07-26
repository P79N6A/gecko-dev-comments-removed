




package org.mozilla.gecko.home;

import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.db.BrowserDB.TopSitesCursorWrapper;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.HomeListView.HomeContextMenuInfo;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PinSiteDialog.OnSiteSelectedListener;
import org.mozilla.gecko.home.TopSitesGridView.OnPinSiteListener;
import org.mozilla.gecko.home.TopSitesGridView.TopSitesGridContextMenuInfo;
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
import android.support.v4.widget.CursorAdapter;
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
import android.view.ViewStub;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.EnumSet;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;




public class TopSitesPage extends HomeFragment {
    
    private static final String LOGTAG = "GeckoTopSitesPage";

    
    private static final int LOADER_ID_TOP_SITES = 0;

    
    private static final int LOADER_ID_THUMBNAILS = 1;

    
    private static final String THUMBNAILS_URLS_KEY = "urls";

    
    private VisitedAdapter mListAdapter;

    
    private TopSitesGridAdapter mGridAdapter;

    
    private ListView mList;

    
    private TopSitesGridView mGrid;

    
    private View mEmptyView;

    
    private HomeBanner mBanner;

    
    private float mListTouchY = -1;

    
    private boolean mSnapBannerToTop;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private ThumbnailsLoaderCallbacks mThumbnailsLoaderCallbacks;

    
    private PinSiteListener mPinSiteListener;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private int mMaxGridEntries;

    


    public static class Thumbnail {
        
        private final boolean isThumbnail;

        
        private final Bitmap bitmap;

        public Thumbnail(Bitmap bitmap, boolean isThumbnail) {
            this.bitmap = bitmap;
            this.isThumbnail = isThumbnail;
        }
    }

    public static TopSitesPage newInstance() {
        return new TopSitesPage();
    }

    public TopSitesPage() {
        mUrlOpenListener = null;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        mMaxGridEntries = activity.getResources().getInteger(R.integer.number_of_top_sites);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUrlOpenListener = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.home_top_sites_page, container, false);

        mList = (HomeListView) view.findViewById(R.id.list);

        mGrid = new TopSitesGridView(getActivity());
        mList.addHeaderView(mGrid);

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mPinSiteListener = new PinSiteListener();

        mList.setTag(HomePager.LIST_TAG_MOST_VISITED);

        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final ListView list = (ListView) parent;
                final int headerCount = list.getHeaderViewsCount();
                if (position < headerCount) {
                    
                    return;
                }

                
                position += (mGridAdapter.getCount() - headerCount);

                final Cursor c = mListAdapter.getCursor();
                if (c == null || !c.moveToPosition(position)) {
                    return;
                }

                final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));

                
                mUrlOpenListener.onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        mGrid.setOnUrlOpenListener(mUrlOpenListener);
        mGrid.setOnPinSiteListener(mPinSiteListener);

        registerForContextMenu(mList);
        registerForContextMenu(mGrid);

        mBanner = (HomeBanner) view.findViewById(R.id.home_banner);
        mList.setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                TopSitesPage.this.handleListTouchEvent(event);
                return false;
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
        mGrid = null;
        mEmptyView = null;
        mListAdapter = null;
        mGridAdapter = null;
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

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final Activity activity = getActivity();

        
        mGridAdapter = new TopSitesGridAdapter(activity, null);
        mGrid.setAdapter(mGridAdapter);

        
        mListAdapter = new VisitedAdapter(activity, null);
        mList.setAdapter(mListAdapter);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks(activity, getLoaderManager());
        mThumbnailsLoaderCallbacks = new ThumbnailsLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        if (menuInfo == null) {
            return;
        }

        
        if (menuInfo instanceof HomeContextMenuInfo) {
            super.onCreateContextMenu(menu, view, menuInfo);
        }

        if (!(menuInfo instanceof TopSitesGridContextMenuInfo)) {
            return;
        }

        MenuInflater inflater = new MenuInflater(view.getContext());
        inflater.inflate(R.menu.top_sites_contextmenu, menu);

        TopSitesGridContextMenuInfo info = (TopSitesGridContextMenuInfo) menuInfo;
        menu.setHeaderTitle(info.getDisplayTitle());

        if (!TextUtils.isEmpty(info.url)) {
            if (info.isPinned) {
                menu.findItem(R.id.top_sites_pin).setVisible(false);
            } else {
                menu.findItem(R.id.top_sites_unpin).setVisible(false);
            }
        } else {
            menu.findItem(R.id.top_sites_open_new_tab).setVisible(false);
            menu.findItem(R.id.top_sites_open_private_tab).setVisible(false);
            menu.findItem(R.id.top_sites_pin).setVisible(false);
            menu.findItem(R.id.top_sites_unpin).setVisible(false);
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        ContextMenuInfo menuInfo = item.getMenuInfo();

        
        if (menuInfo == null || !(menuInfo instanceof TopSitesGridContextMenuInfo)) {
            return false;
        }

        TopSitesGridContextMenuInfo info = (TopSitesGridContextMenuInfo) menuInfo;
        final Activity activity = getActivity();

        final int itemId = item.getItemId();
        if (itemId == R.id.top_sites_open_new_tab || itemId == R.id.top_sites_open_private_tab) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't open in new tab because URL is null");
                return false;
            }

            int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_BACKGROUND;
            if (item.getItemId() == R.id.top_sites_open_private_tab)
                flags |= Tabs.LOADURL_PRIVATE;

            Tabs.getInstance().loadUrl(info.url, flags);
            Toast.makeText(activity, R.string.new_tab_opened, Toast.LENGTH_SHORT).show();
            return true;
        }

        if (itemId == R.id.top_sites_pin) {
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

        if (itemId == R.id.top_sites_unpin) {
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

        if (itemId == R.id.top_sites_edit) {
            mPinSiteListener.onPinSite(info.position);
            return true;
        }

        return false;
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_TOP_SITES, null, mCursorLoaderCallbacks);
    }

    


    private class PinSiteListener implements OnPinSiteListener,
                                             OnSiteSelectedListener {
        
        private static final String TAG_PIN_SITE = "pin_site";

        
        private int mPosition;

        @Override
        public void onPinSite(int position) {
            mPosition = position;

            final FragmentManager manager = getActivity().getSupportFragmentManager();
            PinSiteDialog dialog = (PinSiteDialog) manager.findFragmentByTag(TAG_PIN_SITE);
            if (dialog == null) {
                dialog = PinSiteDialog.newInstance();
            }

            dialog.setOnSiteSelectedListener(this);
            dialog.show(manager, TAG_PIN_SITE);
        }

        @Override
        public void onSiteSelected(final String url, final String title) {
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

    private void updateUiFromCursor(Cursor c) {
        if (c != null && c.getCount() > 0) {
            return;
        }

        if (mEmptyView == null) {
            
            ViewStub emptyViewStub = (ViewStub) getView().findViewById(R.id.home_empty_view_stub);
            mEmptyView = emptyViewStub.inflate();

            final ImageView emptyIcon = (ImageView) mEmptyView.findViewById(R.id.home_empty_image);
            emptyIcon.setImageResource(R.drawable.icon_most_visited_empty);

            final TextView emptyText = (TextView) mEmptyView.findViewById(R.id.home_empty_text);
            emptyText.setText(R.string.home_most_visited_empty);

            mList.setEmptyView(mEmptyView);
        }
    }

    private static class TopSitesLoader extends SimpleCursorLoader {
        
        private static final int SEARCH_LIMIT = 30;
        private int mMaxGridEntries;

        public TopSitesLoader(Context context) {
            super(context);
            mMaxGridEntries = context.getResources().getInteger(R.integer.number_of_top_sites);
        }

        @Override
        public Cursor loadCursor() {
            return BrowserDB.getTopSites(getContext().getContentResolver(), mMaxGridEntries, SEARCH_LIMIT);
        }
    }

    private class VisitedAdapter extends CursorAdapter {
        public VisitedAdapter(Context context, Cursor cursor) {
            super(context, cursor);
        }

        @Override
        public int getCount() {
            return Math.max(0, super.getCount() - mMaxGridEntries);
        }

        @Override
        public Object getItem(int position) {
            return super.getItem(position + mMaxGridEntries);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            final int position = cursor.getPosition();
            cursor.moveToPosition(position + mMaxGridEntries);

            final TwoLinePageRow row = (TwoLinePageRow) view;
            row.updateFromCursor(cursor);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return LayoutInflater.from(context).inflate(R.layout.home_item_row, parent, false);
        }
    }

    public class TopSitesGridAdapter extends CursorAdapter {
        
        private Map<String, Thumbnail> mThumbnails;

        public TopSitesGridAdapter(Context context, Cursor cursor) {
            super(context, cursor);
        }

        @Override
        public int getCount() {
            return Math.min(mMaxGridEntries, super.getCount());
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

    private class CursorLoaderCallbacks extends HomeCursorLoaderCallbacks {
        public CursorLoaderCallbacks(Context context, LoaderManager loaderManager) {
            super(context, loaderManager);
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            if (id == LOADER_ID_TOP_SITES) {
                return new TopSitesLoader(getActivity());
            } else {
                return super.onCreateLoader(id, args);
            }
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            if (loader.getId() == LOADER_ID_TOP_SITES) {
                mListAdapter.swapCursor(c);
                mGridAdapter.swapCursor(c);
                updateUiFromCursor(c);
                loadFavicons(c);

                
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
            } else {
                super.onLoadFinished(loader, c);
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            if (loader.getId() == LOADER_ID_TOP_SITES) {
                if (mListAdapter != null) {
                    mListAdapter.swapCursor(null);
                }
                if (mGridAdapter != null) {
                    mGridAdapter.swapCursor(null);
                }
            } else {
                super.onLoaderReset(loader);
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
                        
                        
                        thumbnails.put(url, new Thumbnail(Favicons.scaleImage(bitmap), false));
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
            if (mGridAdapter != null) {
                mGridAdapter.updateThumbnails(thumbnails);
            }
        }

        @Override
        public void onLoaderReset(Loader<Map<String, Thumbnail>> loader) {
            if (mGridAdapter != null) {
                mGridAdapter.updateThumbnails(null);
            }
        }
    }
}
