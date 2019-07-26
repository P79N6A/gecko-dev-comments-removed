




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.home.TwoLinePageRow;
import org.mozilla.gecko.widget.FaviconView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.content.res.Configuration;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

import java.util.ArrayList;




public class BrowserSearch extends Fragment implements AdapterView.OnItemClickListener,
                                                       GeckoEventListener {
    
    private static final String LOGTAG = "GeckoBrowserSearch";

    
    private static final int SEARCH_LOADER_ID = 0;

    
    private static final int FAVICONS_LOADER_ID = 1;

    
    private static final int SUGGESTION_LOADER_ID = 2;

    
    private static final String FAVICONS_LOADER_URLS_ARG = "urls";

    
    private static final int SUGGESTION_TIMEOUT = 3000;

    
    private static final int SUGGESTION_MAX = 3;

    
    private String mSearchTerm;

    
    private SearchAdapter mAdapter;

    
    private ListView mList;

    
    private SuggestClient mSuggestClient;

    
    private ArrayList<SearchEngine> mSearchEngines;

    
    private boolean mSuggestionsEnabled;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private SuggestionLoaderCallbacks mSuggestionLoaderCallbacks;

    
    private LayoutInflater mInflater;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private OnSearchListener mSearchListener;

    
    private OnEditSuggestionListener mEditSuggestionListener;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
    }

    public interface OnSearchListener {
        public void onSearch(SearchEngine engine, String text);
    }

    public interface OnEditSuggestionListener {
        public void onEditSuggestion(String suggestion);
    }

    public static BrowserSearch newInstance() {
        return new BrowserSearch();
    }

    public BrowserSearch() {
        mSearchTerm = "";
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        registerEventListener("SearchEngines:Data");

        
        
        if (mSearchEngines == null) {
            mSearchEngines = new ArrayList<SearchEngine>();
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:Get", null));
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterEventListener("SearchEngines:Data");
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement BrowserSearch.OnUrlOpenListener");
        }

        try {
            mSearchListener = (OnSearchListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement BrowserSearch.OnSearchListener");
        }

        try {
            mEditSuggestionListener = (OnEditSuggestionListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement BrowserSearch.OnEditSuggestionListener");
        }

        mInflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUrlOpenListener = null;
        mSearchListener = null;
        mEditSuggestionListener = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        
        
        mList = new ListView(container.getContext());
        return mList;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mList.setOnItemClickListener(this);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mAdapter = new SearchAdapter(getActivity());
        mList.setAdapter(mAdapter);

        
        mSuggestionLoaderCallbacks = null;

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();

        
        getLoaderManager().initLoader(SEARCH_LOADER_ID, null, mCursorLoaderCallbacks);
    }

    @Override
    public void handleMessage(String event, final JSONObject message) {
        if (event.equals("SearchEngines:Data")) {
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    setSearchEngines(message);
                }
            });
        }
    }

    private void filterSuggestions() {
        if (mSuggestClient == null || !mSuggestionsEnabled) {
            return;
        }

        if (mSuggestionLoaderCallbacks == null) {
            mSuggestionLoaderCallbacks = new SuggestionLoaderCallbacks();
        }

        getLoaderManager().restartLoader(SUGGESTION_LOADER_ID, null, mSuggestionLoaderCallbacks);
    }

    private void setSuggestions(ArrayList<String> suggestions) {
        mSearchEngines.get(0).suggestions = suggestions;
        mAdapter.notifyDataSetChanged();
    }

    private void setSearchEngines(JSONObject data) {
        try {
            final JSONObject suggest = data.getJSONObject("suggest");
            final String suggestEngine = suggest.optString("engine", null);
            final String suggestTemplate = suggest.optString("template", null);
            final boolean suggestionsPrompted = suggest.getBoolean("prompted");
            final JSONArray engines = data.getJSONArray("searchEngines");

            mSuggestionsEnabled = suggest.getBoolean("enabled");

            ArrayList<SearchEngine> searchEngines = new ArrayList<SearchEngine>();
            for (int i = 0; i < engines.length(); i++) {
                final JSONObject engineJSON = engines.getJSONObject(i);
                final String name = engineJSON.getString("name");
                final String identifier = engineJSON.getString("identifier");
                final String iconURI = engineJSON.getString("iconURI");
                final Bitmap icon = BitmapUtils.getBitmapFromDataURI(iconURI);

                if (name.equals(suggestEngine) && suggestTemplate != null) {
                    
                    searchEngines.add(0, new SearchEngine(name, identifier, icon));

                    
                    
                    
                    
                    Tab tab = Tabs.getInstance().getSelectedTab();
                    if (tab == null || !tab.isPrivate()) {
                        mSuggestClient = new SuggestClient(getActivity(), suggestTemplate,
                                SUGGESTION_TIMEOUT, SUGGESTION_MAX);
                    }
                } else {
                    searchEngines.add(new SearchEngine(name, identifier, icon));
                }
            }

            mSearchEngines = searchEngines;

            if (mAdapter != null) {
                mAdapter.notifyDataSetChanged();
            }

            
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error getting search engine JSON", e);
        }

        filterSuggestions();
    }

    private void registerEventListener(String eventName) {
        GeckoAppShell.getEventDispatcher().registerEventListener(eventName, this);
    }

    private void unregisterEventListener(String eventName) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(eventName, this);
    }

    private ArrayList<String> getUrlsWithoutFavicon(Cursor c) {
        ArrayList<String> urls = new ArrayList<String>();

        if (c == null || !c.moveToFirst()) {
            return urls;
        }

        final Favicons favicons = Favicons.getInstance();

        do {
            final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));

            
            
            if (favicons.getFaviconFromMemCache(url) != null) {
                continue;
            }

            urls.add(url);
        } while (c.moveToNext());

        return urls;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        final Cursor c = mAdapter.getCursor();
        if (c == null || !c.moveToPosition(position)) {
            return;
        }

        final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));
        mUrlOpenListener.onUrlOpen(url);
    }

    public void filter(String searchTerm) {
        if (TextUtils.isEmpty(searchTerm)) {
            return;
        }

        if (TextUtils.equals(mSearchTerm, searchTerm)) {
            return;
        }

        mSearchTerm = searchTerm;

        if (isVisible()) {
            
            
            mAdapter.notifyDataSetChanged();

            
            getLoaderManager().restartLoader(SEARCH_LOADER_ID, null, mCursorLoaderCallbacks);
            filterSuggestions();
        }
    }

    private static class SearchCursorLoader extends SimpleCursorLoader {
        
        private static final int SEARCH_LIMIT = 100;

        
        private final String mSearchTerm;

        public SearchCursorLoader(Context context, String searchTerm) {
            super(context);
            mSearchTerm = searchTerm;
        }

        @Override
        public Cursor loadCursor() {
            if (TextUtils.isEmpty(mSearchTerm)) {
                return null;
            }

            final ContentResolver cr = getContext().getContentResolver();
            return BrowserDB.filter(cr, mSearchTerm, SEARCH_LIMIT);
        }
    }

    private static class FaviconsCursorLoader extends SimpleCursorLoader {
        private ArrayList<String> mUrls;

        public FaviconsCursorLoader(Context context, ArrayList<String> urls) {
            super(context);
            mUrls = urls;
        }

        @Override
        public Cursor loadCursor() {
            final ContentResolver cr = getContext().getContentResolver();

            Cursor c = BrowserDB.getFaviconsForUrls(cr, mUrls);
            storeFaviconsInMemCache(c);

            return c;
        }

        private void storeFaviconsInMemCache(Cursor c) {
            if (c == null || !c.moveToFirst()) {
                return;
            }

            final Favicons favicons = Favicons.getInstance();

            do {
                final String url = c.getString(c.getColumnIndexOrThrow(Combined.URL));
                final byte[] b = c.getBlob(c.getColumnIndexOrThrow(Combined.FAVICON));

                if (b == null || b.length == 0) {
                    continue;
                }

                Bitmap favicon = BitmapUtils.decodeByteArray(b);
                if (favicon == null) {
                    continue;
                }

                favicon = favicons.scaleImage(favicon);
                favicons.putFaviconInMemCache(url, favicon);
            } while (c.moveToNext());
        }
    }

    private static class SuggestionAsyncLoader extends AsyncTaskLoader<ArrayList<String>> {
        private final SuggestClient mSuggestClient;
        private final String mSearchTerm;
        private ArrayList<String> mSuggestions;

        public SuggestionAsyncLoader(Context context, SuggestClient suggestClient, String searchTerm) {
            super(context);
            mSuggestClient = suggestClient;
            mSearchTerm = searchTerm;
            mSuggestions = null;
        }

        @Override
        public ArrayList<String> loadInBackground() {
            return mSuggestClient.query(mSearchTerm);
        }

        @Override
        public void deliverResult(ArrayList<String> suggestions) {
            mSuggestions = suggestions;

            if (isStarted()) {
                super.deliverResult(mSuggestions);
            }
        }

        @Override
        protected void onStartLoading() {
            if (mSuggestions != null) {
                deliverResult(mSuggestions);
            }

            if (takeContentChanged() || mSuggestions == null) {
                forceLoad();
            }
        }

        @Override
        protected void onStopLoading() {
            cancelLoad();
        }

        @Override
        protected void onReset() {
            super.onReset();

            onStopLoading();
            mSuggestions = null;
        }
    }

    private class SearchAdapter extends SimpleCursorAdapter {
        private static final int ROW_SEARCH = 0;
        private static final int ROW_STANDARD = 1;
        private static final int ROW_SUGGEST = 2;

        private static final int ROW_TYPE_COUNT = 3;

        public SearchAdapter(Context context) {
            super(context, -1, null, new String[] {}, new int[] {});
        }

        @Override
        public int getItemViewType(int position) {
            final int engine = getEngineIndex(position);

            if (engine == -1) {
                return ROW_STANDARD;
            } else if (engine == 0 && mSuggestionsEnabled) {
                
                
                
                
                return ROW_SUGGEST;
            }

            return ROW_SEARCH;
        }

        @Override
        public int getViewTypeCount() {
            
            
            return ROW_TYPE_COUNT;
        }

        @Override
        public boolean isEnabled(int position) {
            
            
            
            
            final int index = getEngineIndex(position);
            if (index != -1) {
                return mSearchEngines.get(index).suggestions.isEmpty();
            }

            return true;
        }

        
        @Override
        public int getCount() {
            final int resultCount = super.getCount();

            
            if (TextUtils.isEmpty(mSearchTerm)) {
                return resultCount;
            }

            return resultCount + mSearchEngines.size();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final int type = getItemViewType(position);

            if (type == ROW_SEARCH || type == ROW_SUGGEST) {
                final SearchEngineRow row;
                if (convertView == null) {
                    row = (SearchEngineRow) mInflater.inflate(R.layout.home_search_item_row, mList, false);
                    row.setOnUrlOpenListener(mUrlOpenListener);
                    row.setOnSearchListener(mSearchListener);
                    row.setOnEditSuggestionListener(mEditSuggestionListener);
                } else {
                    row = (SearchEngineRow) convertView;
                }

                row.setSearchTerm(mSearchTerm);

                final SearchEngine engine = mSearchEngines.get(getEngineIndex(position));
                row.updateFromSearchEngine(engine);

                return row;
            } else {
                final TwoLinePageRow row;
                if (convertView == null) {
                    row = (TwoLinePageRow) mInflater.inflate(R.layout.home_item_row, mList, false);
                } else {
                    row = (TwoLinePageRow) convertView;
                }

                
                position -= getSuggestEngineCount();

                final Cursor c = getCursor();
                if (!c.moveToPosition(position)) {
                    throw new IllegalStateException("Couldn't move cursor to position " + position);
                }

                row.updateFromCursor(c);

                return row;
            }
        }

        private int getSuggestEngineCount() {
            return (TextUtils.isEmpty(mSearchTerm) || mSuggestClient == null || !mSuggestionsEnabled) ? 0 : 1;
        }

        private int getEngineIndex(int position) {
            final int resultCount = super.getCount();
            final int suggestEngineCount = getSuggestEngineCount();

            
            if (position < suggestEngineCount) {
                return position;
            }

            
            if (position - suggestEngineCount < resultCount) {
                return -1;
            }

            
            return position - resultCount;
        }
    }

    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            switch(id) {
            case SEARCH_LOADER_ID:
                return new SearchCursorLoader(getActivity(), mSearchTerm);

            case FAVICONS_LOADER_ID:
                final ArrayList<String> urls = args.getStringArrayList(FAVICONS_LOADER_URLS_ARG);
                return new FaviconsCursorLoader(getActivity(), urls);
            }

            return null;
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            final int loaderId = loader.getId();
            switch(loaderId) {
            case SEARCH_LOADER_ID:
                mAdapter.swapCursor(c);

                
                
                ArrayList<String> urls = getUrlsWithoutFavicon(c);
                if (urls.size() > 0) {
                    Bundle args = new Bundle();
                    args.putStringArrayList(FAVICONS_LOADER_URLS_ARG, urls);
                    getLoaderManager().restartLoader(FAVICONS_LOADER_ID, args, mCursorLoaderCallbacks);
                }
                break;

            case FAVICONS_LOADER_ID:
                
                
                mAdapter.notifyDataSetChanged();
                break;
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            final int loaderId = loader.getId();
            switch(loaderId) {
            case SEARCH_LOADER_ID:
                mAdapter.swapCursor(null);
                break;

            case FAVICONS_LOADER_ID:
                
                break;
            }
        }
    }

    private class SuggestionLoaderCallbacks implements LoaderCallbacks<ArrayList<String>> {
        @Override
        public Loader<ArrayList<String>> onCreateLoader(int id, Bundle args) {
            return new SuggestionAsyncLoader(getActivity(), mSuggestClient, mSearchTerm);
        }

        @Override
        public void onLoadFinished(Loader<ArrayList<String>> loader, ArrayList<String> suggestions) {
            setSuggestions(suggestions);
        }

        @Override
        public void onLoaderReset(Loader<ArrayList<String>> loader) {
            setSuggestions(new ArrayList<String>());
        }
    }
}