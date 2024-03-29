




package org.mozilla.gecko.home;

import static org.mozilla.gecko.db.URLMetadataTable.TILE_COLOR_COLUMN;
import static org.mozilla.gecko.db.URLMetadataTable.TILE_IMAGE_URL_COLUMN;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserContract.TopSites;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.HomeContextMenuInfo.RemoveItemType;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PinSiteDialog.OnSiteSelectedListener;
import org.mozilla.gecko.home.TopSitesGridView.OnEditPinnedSiteListener;
import org.mozilla.gecko.home.TopSitesGridView.TopSitesGridContextMenuInfo;
import org.mozilla.gecko.tiles.TilesRecorder;
import org.mozilla.gecko.tiles.Tile;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.os.SystemClock;
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

    
    private int mMaxGridEntries;

    
    private static final long PRIORITY_RESET_TIMEOUT = 10000;

    public static TopSitesPanel newInstance() {
        return new TopSitesPanel();
    }

    private static final boolean logDebug = Log.isLoggable(LOGTAG, Log.DEBUG);
    private static final boolean logVerbose = Log.isLoggable(LOGTAG, Log.VERBOSE);

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
                info.itemType = RemoveItemType.HISTORY;
                final int bookmarkIdCol = cursor.getColumnIndexOrThrow(TopSites.BOOKMARK_ID);
                if (cursor.isNull(bookmarkIdCol)) {
                    
                    
                    info.bookmarkId =  -1;
                } else {
                    info.bookmarkId = cursor.getInt(bookmarkIdCol);
                }
                return info;
            }
        });

        mGrid.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TopSitesGridItemView item = (TopSitesGridItemView) view;

                
                String url = StringUtils.decodeUserEnteredUrl(item.getUrl());
                int type = item.getType();

                
                
                if (type != TopSites.TYPE_BLANK) {
                    if (mUrlOpenListener != null) {
                        final TelemetryContract.Method method;
                        if (type == TopSites.TYPE_SUGGESTED) {
                            method = TelemetryContract.Method.SUGGESTION;
                        } else {
                            method = TelemetryContract.Method.GRID_ITEM;
                        }
                        Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, method, Integer.toString(position));

                        
                        final Tab tab = Tabs.getInstance().getSelectedTab();
                        if (!tab.isPrivate()) {
                            final Locale locale = Locale.getDefault();
                            final String localeTag = Locales.getLanguageTag(locale);
                            TilesRecorder.recordAction(tab, TilesRecorder.ACTION_CLICK, position, getTilesSnapshot(), localeTag);
                        }

                        mUrlOpenListener.onUrlOpen(url, EnumSet.noneOf(OnUrlOpenListener.Flags.class));
                    }
                } else {
                    if (mEditPinnedSiteListener != null) {
                        mEditPinnedSiteListener.onEditPinnedSite(position, "");
                    }
                }
            }
        });

        mGrid.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {

                Cursor cursor = (Cursor) parent.getItemAtPosition(position);

                TopSitesGridItemView item = (TopSitesGridItemView) view;
                if (cursor == null || item.getType() == TopSites.TYPE_BLANK) {
                    mGrid.setContextMenuInfo(null);
                    return false;
                }

                TopSitesGridContextMenuInfo contextMenuInfo = new TopSitesGridContextMenuInfo(view, position, id);
                updateContextMenuFromCursor(contextMenuInfo, cursor);
                mGrid.setContextMenuInfo(contextMenuInfo);
                return mGrid.showContextMenuForChild(mGrid);
            }

            






            private void updateContextMenuFromCursor(TopSitesGridContextMenuInfo info, Cursor cursor) {
                info.url = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.URL));
                info.title = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.TITLE));
                info.type = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.TYPE));
                info.historyId = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.HISTORY_ID));
            }
        });

        registerForContextMenu(mList);
        registerForContextMenu(mGrid);
    }

    private List<Tile> getTilesSnapshot() {
        final int count = mGrid.getCount();
        final ArrayList<Tile> snapshot = new ArrayList<>();
        final BrowserDB db = GeckoProfile.get(getActivity()).getDB();
        for (int i = 0; i < count; i++) {
            final Cursor cursor = (Cursor) mGrid.getItemAtPosition(i);
            final int type = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.TYPE));

            if (type == TopSites.TYPE_BLANK) {
                snapshot.add(null);
                continue;
            }

            final String url = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.URL));
            final int id = db.getTrackingIdForUrl(url);
            final boolean pinned = (type == TopSites.TYPE_PINNED);
            snapshot.add(new Tile(id, pinned));
        }
        return snapshot;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        
        
        mList.setOnItemClickListener(null);
        mGrid.setOnItemClickListener(null);

        mList = null;
        mGrid = null;
        mListAdapter = null;
        mGridAdapter = null;
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

        
        menu.findItem(R.id.home_edit_bookmark).setVisible(false);

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

        if (!StringUtils.isShareableUrl(info.url) || GeckoProfile.get(getActivity()).inGuestMode()) {
            menu.findItem(R.id.home_share).setVisible(false);
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (super.onContextItemSelected(item)) {
            
            return true;
        }

        ContextMenuInfo menuInfo = item.getMenuInfo();

        if (!(menuInfo instanceof TopSitesGridContextMenuInfo)) {
            return false;
        }

        TopSitesGridContextMenuInfo info = (TopSitesGridContextMenuInfo) menuInfo;

        final int itemId = item.getItemId();
        final BrowserDB db = GeckoProfile.get(getActivity()).getDB();

        if (itemId == R.id.top_sites_pin) {
            final String url = info.url;
            final String title = info.title;
            final int position = info.position;
            final Context context = getActivity().getApplicationContext();

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    db.pinSite(context.getContentResolver(), url, title, position);
                }
            });

            Telemetry.sendUIEvent(TelemetryContract.Event.PIN);
            return true;
        }

        if (itemId == R.id.top_sites_unpin) {
            final int position = info.position;
            final Context context = getActivity().getApplicationContext();

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    db.unpinSite(context.getContentResolver(), position);
                }
            });

            Telemetry.sendUIEvent(TelemetryContract.Event.UNPIN);

            return true;
        }

        if (itemId == R.id.top_sites_edit) {
            
            mEditPinnedSiteListener.onEditPinnedSite(info.position,
                                                     StringUtils.decodeUserEnteredUrl(info.url));

            Telemetry.sendUIEvent(TelemetryContract.Event.EDIT);
            return true;
        }

        return false;
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_TOP_SITES, null, mCursorLoaderCallbacks);

        
        
        
        
        
        
        ThreadUtils.reduceGeckoPriority(PRIORITY_RESET_TIMEOUT);
    }

    


    private class EditPinnedSiteListener implements OnEditPinnedSiteListener,
                                                    OnSiteSelectedListener {
        
        private static final String TAG_PIN_SITE = "pin_site";

        
        private int mPosition;

        @Override
        public void onEditPinnedSite(int position, String searchTerm) {
            final FragmentManager manager = getChildFragmentManager();
            PinSiteDialog dialog = (PinSiteDialog) manager.findFragmentByTag(TAG_PIN_SITE);
            if (dialog == null) {
                mPosition = position;

                dialog = PinSiteDialog.newInstance();
                dialog.setOnSiteSelectedListener(this);
                dialog.setSearchTerm(searchTerm);
                dialog.show(manager, TAG_PIN_SITE);
            }
        }

        @Override
        public void onSiteSelected(final String url, final String title) {
            final int position = mPosition;
            final Context context = getActivity().getApplicationContext();
            final BrowserDB db = GeckoProfile.get(getActivity()).getDB();
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    db.pinSite(context.getContentResolver(), url, title, position);
                }
            });
        }
    }

    private void updateUiFromCursor(Cursor c) {
        mList.setHeaderDividersEnabled(c != null && c.getCount() > mMaxGridEntries);
    }

    private void updateUiWithThumbnails(Map<String, ThumbnailInfo> thumbnails) {
        if (mGridAdapter != null) {
            mGridAdapter.updateThumbnails(thumbnails);
        }

        
        
        ThreadUtils.resetGeckoPriority();
    }

    private static class TopSitesLoader extends SimpleCursorLoader {
        
        private static final int SEARCH_LIMIT = 30;
        private static final String TELEMETRY_HISTOGRAM_LOAD_CURSOR = "FENNEC_TOPSITES_LOADER_TIME_MS";
        private final BrowserDB mDB;
        private final int mMaxGridEntries;

        public TopSitesLoader(Context context) {
            super(context);
            mMaxGridEntries = context.getResources().getInteger(R.integer.number_of_top_sites);
            mDB = GeckoProfile.get(context).getDB();
        }

        @Override
        public Cursor loadCursor() {
            final long start = SystemClock.uptimeMillis();
            final Cursor cursor = mDB.getTopSites(getContext().getContentResolver(), mMaxGridEntries, SEARCH_LIMIT);
            final long end = SystemClock.uptimeMillis();
            final long took = end - start;
            Telemetry.addToHistogram(TELEMETRY_HISTOGRAM_LOAD_CURSOR, (int) Math.min(took, Integer.MAX_VALUE));
            return cursor;
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
        private final BrowserDB mDB;
        
        
        private Map<String, ThumbnailInfo> mThumbnailInfos;

        public TopSitesGridAdapter(Context context, Cursor cursor) {
            super(context, cursor, 0);
            mDB = GeckoProfile.get(context).getDB();
        }

        @Override
        public int getCount() {
            return Math.min(mMaxGridEntries, super.getCount());
        }

        @Override
        protected void onContentChanged() {
            
            
            return;
        }

        




        public void updateThumbnails(Map<String, ThumbnailInfo> thumbnails) {
            mThumbnailInfos = thumbnails;

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
                view.blankOut();
                return;
            }

            
            ThumbnailInfo thumbnail = (mThumbnailInfos != null ? mThumbnailInfos.get(url) : null);

            
            
            final boolean updated = view.updateState(title, url, type, thumbnail);

            
            
            
            if (!updated) {
                debug("bindView called twice for same values; short-circuiting.");
                return;
            }

            
            final String decodedUrl = StringUtils.decodeUserEnteredUrl(url);

            
            
            final String imageUrl = mDB.getSuggestedImageUrlForUrl(decodedUrl);
            if (!TextUtils.isEmpty(imageUrl)) {
                final int bgColor = mDB.getSuggestedBackgroundColorForUrl(decodedUrl);
                view.displayThumbnail(imageUrl, bgColor);
                return;
            }

            
            
            if (mThumbnailInfos == null || thumbnail != null) {
                return;
            }

            
            LoadIDAwareFaviconLoadedListener listener = new LoadIDAwareFaviconLoadedListener(view);
            final int loadId = Favicons.getSizedFaviconForPageFromLocal(context, url, listener);
            if (loadId == Favicons.LOADED) {
                
                return;
            }

            
            view.displayThumbnail(R.drawable.favicon_globe);

            
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

    private class CursorLoaderCallbacks extends TransitionAwareCursorLoaderCallbacks {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            trace("Creating TopSitesLoader: " + id);
            return new TopSitesLoader(getActivity());
        }

        









        @Override
        protected void onLoadFinishedAfterTransitions(Loader<Cursor> loader, Cursor c) {
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
                final String url = c.getString(col);

                
                
                final GeckoProfile profile = GeckoProfile.get(getActivity());
                if (TextUtils.isEmpty(url) || profile.getDB().hasSuggestedImageUrl(url)) {
                    continue;
                }

                urls.add(url);
            } while (i++ < mMaxGridEntries && c.moveToNext());

            if (urls.isEmpty()) {
                
                updateUiWithThumbnails(new HashMap<String, ThumbnailInfo>());
                return;
            }

            Bundle bundle = new Bundle();
            bundle.putStringArrayList(THUMBNAILS_URLS_KEY, urls);
            getLoaderManager().restartLoader(LOADER_ID_THUMBNAILS, bundle, mThumbnailsLoaderCallbacks);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            super.onLoaderReset(loader);

            if (mListAdapter != null) {
                mListAdapter.swapCursor(null);
            }

            if (mGridAdapter != null) {
                mGridAdapter.swapCursor(null);
            }
        }
    }

    static class ThumbnailInfo {
        public final Bitmap bitmap;
        public final String imageUrl;
        public final int bgColor;

        public ThumbnailInfo(final Bitmap bitmap) {
            this.bitmap = bitmap;
            this.imageUrl = null;
            this.bgColor = Color.TRANSPARENT;
        }

        public ThumbnailInfo(final String imageUrl, final int bgColor) {
            this.bitmap = null;
            this.imageUrl = imageUrl;
            this.bgColor = bgColor;
        }

        public static ThumbnailInfo fromMetadata(final Map<String, Object> data) {
            if (data == null) {
                return null;
            }

            final String imageUrl = (String) data.get(TILE_IMAGE_URL_COLUMN);
            if (imageUrl == null) {
                return null;
            }

            int bgColor = Color.WHITE;
            final String colorString = (String) data.get(TILE_COLOR_COLUMN);
            try {
                bgColor = Color.parseColor(colorString);
            } catch (Exception ex) {
            }

            return new ThumbnailInfo(imageUrl, bgColor);
        }
    }

    


    @SuppressWarnings("serial")
    static class ThumbnailsLoader extends AsyncTaskLoader<Map<String, ThumbnailInfo>> {
        private final BrowserDB mDB;
        private Map<String, ThumbnailInfo> mThumbnailInfos;
        private final ArrayList<String> mUrls;

        private static final ArrayList<String> COLUMNS = new ArrayList<String>() {{
            add(TILE_IMAGE_URL_COLUMN);
            add(TILE_COLOR_COLUMN);
        }};

        public ThumbnailsLoader(Context context, ArrayList<String> urls) {
            super(context);
            mUrls = urls;
            mDB = GeckoProfile.get(context).getDB();
        }

        @Override
        public Map<String, ThumbnailInfo> loadInBackground() {
            final Map<String, ThumbnailInfo> thumbnails = new HashMap<String, ThumbnailInfo>();
            if (mUrls == null || mUrls.size() == 0) {
                return thumbnails;
            }

            
            final ContentResolver cr = getContext().getContentResolver();
            final Map<String, Map<String, Object>> metadata = mDB.getURLMetadata().getForURLs(cr, mUrls, COLUMNS);

            
            final List<String> thumbnailUrls = new ArrayList<String>();
            for (String url : mUrls) {
                ThumbnailInfo info = ThumbnailInfo.fromMetadata(metadata.get(url));
                if (info == null) {
                    
                    thumbnailUrls.add(url);
                    continue;
                }

                thumbnails.put(url, info);
            }

            if (thumbnailUrls.size() == 0) {
                return thumbnails;
            }

            
            final Cursor cursor = mDB.getThumbnailsForUrls(cr, thumbnailUrls);
            if (cursor == null) {
                return thumbnails;
            }

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

                    thumbnails.put(url, new ThumbnailInfo(bitmap));
                }
            } finally {
                cursor.close();
            }

            return thumbnails;
        }

        @Override
        public void deliverResult(Map<String, ThumbnailInfo> thumbnails) {
            if (isReset()) {
                mThumbnailInfos = null;
                return;
            }

            mThumbnailInfos = thumbnails;

            if (isStarted()) {
                super.deliverResult(thumbnails);
            }
        }

        @Override
        protected void onStartLoading() {
            if (mThumbnailInfos != null) {
                deliverResult(mThumbnailInfos);
            }

            if (takeContentChanged() || mThumbnailInfos == null) {
                forceLoad();
            }
        }

        @Override
        protected void onStopLoading() {
            cancelLoad();
        }

        @Override
        public void onCanceled(Map<String, ThumbnailInfo> thumbnails) {
            mThumbnailInfos = null;
        }

        @Override
        protected void onReset() {
            super.onReset();

            
            onStopLoading();

            mThumbnailInfos = null;
        }
    }

    


    private class ThumbnailsLoaderCallbacks implements LoaderCallbacks<Map<String, ThumbnailInfo>> {
        @Override
        public Loader<Map<String, ThumbnailInfo>> onCreateLoader(int id, Bundle args) {
            return new ThumbnailsLoader(getActivity(), args.getStringArrayList(THUMBNAILS_URLS_KEY));
        }

        @Override
        public void onLoadFinished(Loader<Map<String, ThumbnailInfo>> loader, Map<String, ThumbnailInfo> thumbnails) {
            updateUiWithThumbnails(thumbnails);
        }

        @Override
        public void onLoaderReset(Loader<Map<String, ThumbnailInfo>> loader) {
            if (mGridAdapter != null) {
                mGridAdapter.updateThumbnails(null);
            }
        }
    }
}
