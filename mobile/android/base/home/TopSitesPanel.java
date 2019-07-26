




package org.mozilla.gecko.home;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserContract.TopSites;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PinSiteDialog.OnSiteSelectedListener;
import org.mozilla.gecko.home.TopSitesGridView.OnEditPinnedSiteListener;
import org.mozilla.gecko.home.TopSitesGridView.TopSitesGridContextMenuInfo;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
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
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;




public class TopSitesPanel extends HomeFragment {
    
    private static final String LOGTAG = "GeckoTopSitesPanel";

    
    private static final int LOADER_ID_TOP_SITES = 0;

    
    private static final int LOADER_ID_THUMBNAILS = 1;

    
    private static final String THUMBNAILS_URLS_KEY = "urls";

    
    private VisitedAdapter mListAdapter;

    
    private TopSitesGridAdapter mGridAdapter;

    
    private HomeListView mList;

    
    private TopSitesGridView mGrid;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private ThumbnailsLoaderCallbacks mThumbnailsLoaderCallbacks;

    
    private EditPinnedSiteListener mEditPinnedSiteListener;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private int mMaxGridEntries;

    
    private static final long PRIORITY_RESET_TIMEOUT = 10000;

    public static TopSitesPanel newInstance() {
        return new TopSitesPanel();
    }

    public TopSitesPanel() {
        mUrlOpenListener = null;
    }

    private static boolean logDebug = Log.isLoggable(LOGTAG, Log.DEBUG);
    private static boolean logVerbose = Log.isLoggable(LOGTAG, Log.VERBOSE);

    private static void debug(final String message) {
        if (logDebug) {
            Log.d(LOGTAG, message);
        }
    }

    private static void trace(final String message) {
        if (logVerbose) {
            Log.v(LOGTAG, message);
        }
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
        final View view = inflater.inflate(R.layout.home_top_sites_panel, container, false);

        mList = (HomeListView) view.findViewById(R.id.list);

        mGrid = new TopSitesGridView(getActivity());
        mList.addHeaderView(mGrid);

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mEditPinnedSiteListener = new EditPinnedSiteListener();

        mList.setTag(HomePager.LIST_TAG_TOP_SITES);
        mList.setHeaderDividersEnabled(false);

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

                final String url = c.getString(c.getColumnIndexOrThrow(TopSites.URL));

                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM);

                
                mUrlOpenListener.onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        mList.setContextMenuInfoFactory(new HomeContextMenuInfo.Factory() {
            @Override
            public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                info.url = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.URL));
                info.title = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.TITLE));
                info.historyId = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.HISTORY_ID));
                final int bookmarkIdCol = cursor.getColumnIndexOrThrow(TopSites.BOOKMARK_ID);
                if (cursor.isNull(bookmarkIdCol)) {
                    
                    
                    info.bookmarkId =  -1;
                } else {
                    info.bookmarkId = cursor.getInt(bookmarkIdCol);
                }
                return info;
            }
        });

        mGrid.setOnUrlOpenListener(mUrlOpenListener);
        mGrid.setOnEditPinnedSiteListener(mEditPinnedSiteListener);

        registerForContextMenu(mList);
        registerForContextMenu(mGrid);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        
        
        mList.setOnItemClickListener(null);
        mList = null;

        mGrid = null;
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

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        mThumbnailsLoaderCallbacks = new ThumbnailsLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        if (menuInfo == null) {
            return;
        }

        if (!(menuInfo instanceof TopSitesGridContextMenuInfo)) {
            
            
            super.onCreateContextMenu(menu, view, menuInfo);
            return;
        }

        
        MenuInflater inflater = new MenuInflater(view.getContext());
        inflater.inflate(R.menu.home_contextmenu, menu);

        
        menu.findItem(R.id.home_open_in_reader).setVisible(false);
        menu.findItem(R.id.home_edit_bookmark).setVisible(false);
        menu.findItem(R.id.home_remove).setVisible(false);

        TopSitesGridContextMenuInfo info = (TopSitesGridContextMenuInfo) menuInfo;
        menu.setHeaderTitle(info.getDisplayTitle());

        if (info.type != TopSites.TYPE_BLANK) {
            if (info.type == TopSites.TYPE_PINNED) {
                menu.findItem(R.id.top_sites_pin).setVisible(false);
            } else {
                menu.findItem(R.id.top_sites_unpin).setVisible(false);
            }
        } else {
            menu.findItem(R.id.home_open_new_tab).setVisible(false);
            menu.findItem(R.id.home_open_private_tab).setVisible(false);
            menu.findItem(R.id.top_sites_pin).setVisible(false);
            menu.findItem(R.id.top_sites_unpin).setVisible(false);
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (super.onContextItemSelected(item)) {
            
            return true;
        }

        ContextMenuInfo menuInfo = item.getMenuInfo();

        if (menuInfo == null || !(menuInfo instanceof TopSitesGridContextMenuInfo)) {
            return false;
        }

        TopSitesGridContextMenuInfo info = (TopSitesGridContextMenuInfo) menuInfo;

        final int itemId = item.getItemId();

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

            Telemetry.sendUIEvent(TelemetryContract.Event.TOP_SITES_PIN);
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

            Telemetry.sendUIEvent(TelemetryContract.Event.TOP_SITES_UNPIN);
            return true;
        }

        if (itemId == R.id.top_sites_edit) {
            
            mEditPinnedSiteListener.onEditPinnedSite(info.position, decodeUserEnteredUrl(info.url));

            Telemetry.sendUIEvent(TelemetryContract.Event.TOP_SITES_EDIT);
            return true;
        }

        return false;
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_TOP_SITES, null, mCursorLoaderCallbacks);

        
        
        
        
        
        
        ThreadUtils.reduceGeckoPriority(PRIORITY_RESET_TIMEOUT);
    }

    static String encodeUserEnteredUrl(String url) {
        return Uri.fromParts("user-entered", url, null).toString();
    }

    


    private class EditPinnedSiteListener implements OnEditPinnedSiteListener,
                                                    OnSiteSelectedListener {
        
        private static final String TAG_PIN_SITE = "pin_site";

        
        private int mPosition;

        @Override
        public void onEditPinnedSite(int position, String searchTerm) {
            mPosition = position;

            final FragmentManager manager = getChildFragmentManager();
            PinSiteDialog dialog = (PinSiteDialog) manager.findFragmentByTag(TAG_PIN_SITE);
            if (dialog == null) {
                dialog = PinSiteDialog.newInstance();
            }

            dialog.setOnSiteSelectedListener(this);
            dialog.setSearchTerm(searchTerm);
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

    private void updateUiFromCursor(Cursor c) {
        mList.setHeaderDividersEnabled(c != null && c.getCount() > mMaxGridEntries);
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
            trace("TopSitesLoader.loadCursor()");
            return BrowserDB.getTopSites(getContext().getContentResolver(), mMaxGridEntries, SEARCH_LIMIT);
        }
    }

    private class VisitedAdapter extends CursorAdapter {
        public VisitedAdapter(Context context, Cursor cursor) {
            super(context, cursor, 0);
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
            return LayoutInflater.from(context).inflate(R.layout.bookmark_item_row, parent, false);
        }
    }

    public class TopSitesGridAdapter extends CursorAdapter {
        
        
        private Map<String, Bitmap> mThumbnails;

        public TopSitesGridAdapter(Context context, Cursor cursor) {
            super(context, cursor, 0);
        }

        @Override
        public int getCount() {
            return Math.min(mMaxGridEntries, super.getCount());
        }

        @Override
        protected void onContentChanged() {
            
            
            return;
        }

        




        public void updateThumbnails(Map<String, Bitmap> thumbnails) {
            mThumbnails = thumbnails;

            final int count = mGrid.getChildCount();
            for (int i = 0; i < count; i++) {
                TopSitesGridItemView gridItem = (TopSitesGridItemView) mGrid.getChildAt(i);

                
                
                
                gridItem.markAsDirty();
            }

            notifyDataSetChanged();
        }

        @Override
        public void bindView(View bindView, Context context, Cursor cursor) {
            final String url = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.URL));
            final String title = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.TITLE));
            final int type = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.TYPE));

            final TopSitesGridItemView view = (TopSitesGridItemView) bindView;

            
            if (type == TopSites.TYPE_BLANK) {
                
                if (mThumbnails != null) {
                    view.blankOut();
                }

                return;
            }

            
            Bitmap thumbnail = (mThumbnails != null ? mThumbnails.get(url) : null);

            
            
            final boolean updated = view.updateState(title, url, type, thumbnail);

            
            
            if (mThumbnails == null || thumbnail != null) {
                return;
            }

            
            
            
            if (!updated) {
                debug("bindView called twice for same values; short-circuiting.");
                return;
            }

            final int imageUrlIndex = cursor.getColumnIndex(TopSites.IMAGE_URL);
            if (!cursor.isNull(imageUrlIndex)) {
                String imageUrl = cursor.getString(imageUrlIndex);
                String bgColor = cursor.getString(cursor.getColumnIndex(TopSites.BG_COLOR));
                view.displayThumbnail(imageUrl, Color.parseColor(bgColor));
                return;
            }

            
            LoadIDAwareFaviconLoadedListener listener = new LoadIDAwareFaviconLoadedListener(view);
            final int loadId = Favicons.getSizedFaviconForPageFromLocal(url, listener);
            if (loadId == Favicons.LOADED) {
                
                return;
            }

            
            view.displayThumbnail(R.drawable.favicon);

            
            listener.setLoadId(loadId);
            view.setLoadId(loadId);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return new TopSitesGridItemView(context);
        }
    }

    private static class LoadIDAwareFaviconLoadedListener implements OnFaviconLoadedListener {
        private volatile int loadId = Favicons.NOT_LOADING;
        private final TopSitesGridItemView view;
        public LoadIDAwareFaviconLoadedListener(TopSitesGridItemView view) {
            this.view = view;
        }

        public void setLoadId(int id) {
            this.loadId = id;
        }

        @Override
        public void onFaviconLoaded(String url, String faviconURL, Bitmap favicon) {
            if (TextUtils.equals(this.view.getUrl(), url)) {
                this.view.displayFavicon(favicon, faviconURL, this.loadId);
            }
        }
    }

    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            trace("Creating TopSitesLoader: " + id);
            return new TopSitesLoader(getActivity());
        }

        









        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            debug("onLoadFinished: " + c.getCount() + " rows.");

            mListAdapter.swapCursor(c);
            mGridAdapter.swapCursor(c);
            updateUiFromCursor(c);

            final int col = c.getColumnIndexOrThrow(TopSites.URL);

            
            
            
            
            
            if (!c.moveToFirst()) {
                return;
            }

            final ArrayList<String> urls = new ArrayList<String>();
            int i = 1;
            do {
                urls.add(c.getString(col));
            } while (i++ < mMaxGridEntries && c.moveToNext());

            if (urls.isEmpty()) {
                return;
            }

            Bundle bundle = new Bundle();
            bundle.putStringArrayList(THUMBNAILS_URLS_KEY, urls);
            getLoaderManager().restartLoader(LOADER_ID_THUMBNAILS, bundle, mThumbnailsLoaderCallbacks);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            if (mListAdapter != null) {
                mListAdapter.swapCursor(null);
            }

            if (mGridAdapter != null) {
                mGridAdapter.swapCursor(null);
            }
        }
    }

    


    private static class ThumbnailsLoader extends AsyncTaskLoader<Map<String, Bitmap>> {
        private Map<String, Bitmap> mThumbnails;
        private ArrayList<String> mUrls;

        public ThumbnailsLoader(Context context, ArrayList<String> urls) {
            super(context);
            mUrls = urls;
        }

        @Override
        public Map<String, Bitmap> loadInBackground() {
            if (mUrls == null || mUrls.size() == 0) {
                return null;
            }

            
            final ContentResolver cr = getContext().getContentResolver();
            final Cursor cursor = BrowserDB.getThumbnailsForUrls(cr, mUrls);

            if (cursor == null) {
                return null;
            }

            final Map<String, Bitmap> thumbnails = new HashMap<String, Bitmap>();

            try {
                final int urlIndex = cursor.getColumnIndexOrThrow(Thumbnails.URL);
                final int dataIndex = cursor.getColumnIndexOrThrow(Thumbnails.DATA);

                while (cursor.moveToNext()) {
                    String url = cursor.getString(urlIndex);

                    
                    final byte[] b = cursor.getBlob(dataIndex);
                    if (b == null) {
                        continue;
                    }

                    final Bitmap bitmap = BitmapUtils.decodeByteArray(b);

                    
                    
                    
                    if (bitmap == null) {
                        Log.w(LOGTAG, "Aborting thumbnail load; decode failed.");
                        break;
                    }

                    thumbnails.put(url, bitmap);
                }
            } finally {
                cursor.close();
            }

            return thumbnails;
        }

        @Override
        public void deliverResult(Map<String, Bitmap> thumbnails) {
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
        public void onCanceled(Map<String, Bitmap> thumbnails) {
            mThumbnails = null;
        }

        @Override
        protected void onReset() {
            super.onReset();

            
            onStopLoading();

            mThumbnails = null;
        }
    }

    


    private class ThumbnailsLoaderCallbacks implements LoaderCallbacks<Map<String, Bitmap>> {
        @Override
        public Loader<Map<String, Bitmap>> onCreateLoader(int id, Bundle args) {
            return new ThumbnailsLoader(getActivity(), args.getStringArrayList(THUMBNAILS_URLS_KEY));
        }

        @Override
        public void onLoadFinished(Loader<Map<String, Bitmap>> loader, Map<String, Bitmap> thumbnails) {
            if (mGridAdapter != null) {
                mGridAdapter.updateThumbnails(thumbnails);
            }

            
            
            ThreadUtils.resetGeckoPriority();
        }

        @Override
        public void onLoaderReset(Loader<Map<String, Bitmap>> loader) {
            if (mGridAdapter != null) {
                mGridAdapter.updateThumbnails(null);
            }
        }
    }
}
