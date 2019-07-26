




package org.mozilla.gecko.home;

import org.mozilla.gecko.AutocompleteHandler;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.SearchLoader.SearchCursorLoader;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.WindowManager.LayoutParams;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.AdapterView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;




public class BrowserSearch extends HomeFragment
                           implements GeckoEventListener {
    
    private static final String LOGTAG = "GeckoBrowserSearch";

    
    private static final int LOADER_ID_SEARCH = 0;

    
    private static final int LOADER_ID_SUGGESTION = 1;

    
    private static final int SUGGESTION_TIMEOUT = 3000;

    
    private static final int SUGGESTION_MAX = 3;

    
    
    private static final int MAX_AUTOCOMPLETE_SEARCH = 20;

    
    private static final int ANIMATION_DURATION = 250;

    
    private String mSearchTerm;

    
    private SearchAdapter mAdapter;

    
    private LinearLayout mView;

    
    private HomeListView mList;

    
    private volatile SuggestClient mSuggestClient;

    
    private ArrayList<SearchEngine> mSearchEngines;

    
    private boolean mSuggestionsEnabled;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private SuggestionLoaderCallbacks mSuggestionLoaderCallbacks;

    
    private LayoutInflater mInflater;

    
    private AutocompleteHandler mAutocompleteHandler;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private OnSearchListener mSearchListener;

    
    private OnEditSuggestionListener mEditSuggestionListener;

    
    private boolean mAnimateSuggestions;

    
    private View mSuggestionsOptInPrompt;

    public interface OnSearchListener {
        public void onSearch(String engineId, String text);
    }

    public interface OnEditSuggestionListener {
        public void onEditSuggestion(String suggestion);
    }

    public static BrowserSearch newInstance() {
        BrowserSearch browserSearch = new BrowserSearch();

        final Bundle args = new Bundle();
        args.putBoolean(HomePager.CAN_LOAD_ARG, true);
        browserSearch.setArguments(args);

        return browserSearch;
    }

    public BrowserSearch() {
        mSearchTerm = "";
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

        mInflater = null;
        mAutocompleteHandler = null;
        mUrlOpenListener = null;
        mSearchListener = null;
        mEditSuggestionListener = null;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mSearchEngines = new ArrayList<SearchEngine>();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        mSearchEngines = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        
        
        mView = (LinearLayout) inflater.inflate(R.layout.browser_search, container, false);
        mList = (HomeListView) mView.findViewById(R.id.home_list_view);

        return mView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        unregisterEventListener("SearchEngines:Data");

        mList.setAdapter(null);
        mList = null;

        mView = null;
        mSuggestionsOptInPrompt = null;
        mSuggestClient = null;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mList.setTag(HomePager.LIST_TAG_BROWSER_SEARCH);

        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                
                
                if (view instanceof SearchEngineRow) {
                    ((SearchEngineRow) view).performUserEnteredSearch();
                    return;
                }

                
                position -= getSuggestEngineCount();
                final Cursor c = mAdapter.getCursor(position);
                final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));

                
                mUrlOpenListener.onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        mList.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                
                if (view instanceof SearchEngineRow) {
                    return true;
                }

                
                position -= getSuggestEngineCount();
                return mList.onItemLongClick(parent, view, position, id);
            }
        });

        final ListSelectionListener listener = new ListSelectionListener();
        mList.setOnItemSelectedListener(listener);
        mList.setOnFocusChangeListener(listener);

        mList.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, android.view.KeyEvent event) {
                final View selected = mList.getSelectedView();

                if (selected instanceof SearchEngineRow) {
                    return selected.onKeyDown(keyCode, event);
                }
                return false;
            }
        });

        registerForContextMenu(mList);
        registerEventListener("SearchEngines:Data");

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:GetVisible", null));
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mAdapter = new SearchAdapter(getActivity());
        mList.setAdapter(mAdapter);

        
        mSuggestionLoaderCallbacks = null;

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
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

    @Override
    protected void load() {
        SearchLoader.init(getLoaderManager(), LOADER_ID_SEARCH, mCursorLoaderCallbacks, mSearchTerm);
    }

    private void handleAutocomplete(String searchTerm, Cursor c) {
        if (TextUtils.isEmpty(mSearchTerm) || c == null || mAutocompleteHandler == null) {
            return;
        }

        
        
        final boolean searchPath = (searchTerm.indexOf("/") > 0);
        final String autocompletion = findAutocompletion(searchTerm, c, searchPath);

        if (autocompletion != null && mAutocompleteHandler != null) {
            mAutocompleteHandler.onAutocomplete(autocompletion);
            mAutocompleteHandler = null;
        }
    }

    private String findAutocompletion(String searchTerm, Cursor c, boolean searchPath) {
        if (!c.moveToFirst()) {
            return null;
        }

        final int urlIndex = c.getColumnIndexOrThrow(URLColumns.URL);
        int searchCount = 0;

        do {
            final Uri url = Uri.parse(c.getString(urlIndex));
            final String host = StringUtils.stripCommonSubdomains(url.getHost());

            
            if (host == null) {
                continue;
            }

            final StringBuilder hostBuilder = new StringBuilder(host);
            if (hostBuilder.indexOf(searchTerm) == 0) {
                return hostBuilder.append("/").toString();
            }

            if (searchPath) {
                final List<String> path = url.getPathSegments();

                for (String s : path) {
                    hostBuilder.append("/").append(s);

                    if (hostBuilder.indexOf(searchTerm) == 0) {
                        return hostBuilder.append("/").toString();
                    }
                }
            }

            searchCount++;
        } while (searchCount < MAX_AUTOCOMPLETE_SEARCH && c.moveToNext());

        return null;
    }

    private void filterSuggestions() {
        if (mSuggestClient == null || !mSuggestionsEnabled) {
            return;
        }

        if (mSuggestionLoaderCallbacks == null) {
            mSuggestionLoaderCallbacks = new SuggestionLoaderCallbacks();
        }

        getLoaderManager().restartLoader(LOADER_ID_SUGGESTION, null, mSuggestionLoaderCallbacks);
    }

    private void setSuggestions(ArrayList<String> suggestions) {
        mSearchEngines.get(0).suggestions = suggestions;
        mAdapter.notifyDataSetChanged();
    }

    private void setSearchEngines(JSONObject data) {
        
        
        
        if (mView == null) {
            return;
        }

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
                    final boolean isPrivate = (tab != null && tab.isPrivate());

                    
                    
                    
                    if (mSuggestClient == null && !isPrivate) {
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

            
            
            if (!suggestionsPrompted && mSuggestClient != null) {
                showSuggestionsOptIn();
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error getting search engine JSON", e);
        }

        filterSuggestions();
    }

    private void showSuggestionsOptIn() {
        
        
        if (mSuggestionsOptInPrompt != null) {
            return;
        }

        mSuggestionsOptInPrompt = ((ViewStub) mView.findViewById(R.id.suggestions_opt_in_prompt)).inflate();

        TextView promptText = (TextView) mSuggestionsOptInPrompt.findViewById(R.id.suggestions_prompt_title);
        promptText.setText(getResources().getString(R.string.suggestions_prompt, mSearchEngines.get(0).name));

        final View yesButton = mSuggestionsOptInPrompt.findViewById(R.id.suggestions_prompt_yes);
        final View noButton = mSuggestionsOptInPrompt.findViewById(R.id.suggestions_prompt_no);

        final OnClickListener listener = new OnClickListener() {
            @Override
            public void onClick(View v) {
                
                yesButton.setOnClickListener(null);
                noButton.setOnClickListener(null);

                setSuggestionsEnabled(v == yesButton);
            }
        };

        yesButton.setOnClickListener(listener);
        noButton.setOnClickListener(listener);

        
        
        final View prompt = mSuggestionsOptInPrompt.findViewById(R.id.prompt);
        prompt.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (hasFocus) {
                    yesButton.requestFocus();
                }
            }
        });
    }

    private void setSuggestionsEnabled(final boolean enabled) {
        
        
        
        
        if (mSuggestionsOptInPrompt == null) {
            return;
        }

        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SuggestClient client = mSuggestClient;
                if (client != null) {
                    client.query(mSearchTerm);
                }
            }
        });

        
        PrefsHelper.setPref("browser.search.suggest.enabled", enabled);

        TranslateAnimation slideAnimation = new TranslateAnimation(0, mSuggestionsOptInPrompt.getWidth(), 0, 0);
        slideAnimation.setDuration(ANIMATION_DURATION);
        slideAnimation.setInterpolator(new AccelerateInterpolator());
        slideAnimation.setFillAfter(true);
        final View prompt = mSuggestionsOptInPrompt.findViewById(R.id.prompt);

        TranslateAnimation shrinkAnimation = new TranslateAnimation(0, 0, 0, -1 * mSuggestionsOptInPrompt.getHeight());
        shrinkAnimation.setDuration(ANIMATION_DURATION);
        shrinkAnimation.setFillAfter(true);
        shrinkAnimation.setStartOffset(slideAnimation.getDuration());
        shrinkAnimation.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation a) {
                
                mView.getLayoutParams().height = mView.getHeight() +
                        mSuggestionsOptInPrompt.getHeight();
                mView.requestLayout();
            }

            @Override
            public void onAnimationRepeat(Animation a) {}

            @Override
            public void onAnimationEnd(Animation a) {
                
                
                
                
                mView.post(new Runnable() {
                    @Override
                    public void run() {
                        mView.removeView(mSuggestionsOptInPrompt);
                        mList.clearAnimation();
                        mSuggestionsOptInPrompt = null;

                        if (enabled) {
                            
                            mView.getLayoutParams().height = LayoutParams.MATCH_PARENT;

                            mSuggestionsEnabled = enabled;
                            mAnimateSuggestions = true;
                            mAdapter.notifyDataSetChanged();
                            filterSuggestions();
                        }
                    }
                });
            }
        });

        prompt.startAnimation(slideAnimation);
        mSuggestionsOptInPrompt.startAnimation(shrinkAnimation);
        mList.startAnimation(shrinkAnimation);
    }

    private int getSuggestEngineCount() {
        return (TextUtils.isEmpty(mSearchTerm) || mSuggestClient == null || !mSuggestionsEnabled) ? 0 : 1;
    }

    private void registerEventListener(String eventName) {
        GeckoAppShell.registerEventListener(eventName, this);
    }

    private void unregisterEventListener(String eventName) {
        GeckoAppShell.unregisterEventListener(eventName, this);
    }

    public void filter(String searchTerm, AutocompleteHandler handler) {
        if (TextUtils.isEmpty(searchTerm)) {
            return;
        }

        if (TextUtils.equals(mSearchTerm, searchTerm)) {
            return;
        }

        mSearchTerm = searchTerm;
        mAutocompleteHandler = handler;

        if (isVisible()) {
            
            
            mAdapter.notifyDataSetChanged();

            
            SearchLoader.restart(getLoaderManager(), LOADER_ID_SEARCH, mCursorLoaderCallbacks, mSearchTerm);
            filterSuggestions();
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

    private class SearchAdapter extends MultiTypeCursorAdapter {
        private static final int ROW_SEARCH = 0;
        private static final int ROW_STANDARD = 1;
        private static final int ROW_SUGGEST = 2;

        public SearchAdapter(Context context) {
            super(context, null, new int[] { ROW_STANDARD,
                                             ROW_SEARCH,
                                             ROW_SUGGEST },
                                 new int[] { R.layout.home_item_row,
                                             R.layout.home_search_item_row,
                                             R.layout.home_search_item_row });
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
        public boolean isEnabled(int position) {
            
            
            if (!mList.isInTouchMode()) {
                return true;
            }

            
            
            
            
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
        public void bindView(View view, Context context, int position) {
            final int type = getItemViewType(position);

            if (type == ROW_SEARCH || type == ROW_SUGGEST) {
                final SearchEngineRow row = (SearchEngineRow) view;
                row.setOnUrlOpenListener(mUrlOpenListener);
                row.setOnSearchListener(mSearchListener);
                row.setOnEditSuggestionListener(mEditSuggestionListener);
                row.setSearchTerm(mSearchTerm);

                final SearchEngine engine = mSearchEngines.get(getEngineIndex(position));
                final boolean animate = (mAnimateSuggestions && engine.suggestions.size() > 0);
                row.updateFromSearchEngine(engine, animate);
                if (animate) {
                    
                    mAnimateSuggestions = false;
                }
            } else {
                
                position -= getSuggestEngineCount();

                final Cursor c = getCursor(position);
                final TwoLinePageRow row = (TwoLinePageRow) view;
                row.updateFromCursor(c);
            }
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
            return SearchLoader.createInstance(getActivity(), args);
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            mAdapter.swapCursor(c);

            
            
            
            SearchCursorLoader searchLoader = (SearchCursorLoader) loader;
            handleAutocomplete(searchLoader.getSearchTerm(), c);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            mAdapter.swapCursor(null);
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

    private static class ListSelectionListener implements View.OnFocusChangeListener,
                                                          AdapterView.OnItemSelectedListener {
        private SearchEngineRow mSelectedEngineRow;

        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            if (hasFocus) {
                View selectedRow = ((ListView) v).getSelectedView();
                if (selectedRow != null) {
                    selectRow(selectedRow);
                }
            } else {
                deselectRow();
            }
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            deselectRow();
            selectRow(view);
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            deselectRow();
        }

        private void selectRow(View row) {
            if (row instanceof SearchEngineRow) {
                mSelectedEngineRow = (SearchEngineRow) row;
                mSelectedEngineRow.onSelected();
            }
        }

        private void deselectRow() {
            if (mSelectedEngineRow != null) {
                mSelectedEngineRow.onDeselected();
                mSelectedEngineRow = null;
            }
        }
    }

    


    public static class HomeSearchListView extends HomeListView {
        public HomeSearchListView(Context context, AttributeSet attrs) {
            this(context, attrs, R.attr.homeListViewStyle);
        }

        public HomeSearchListView(Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, defStyle);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                
                requestFocus();
            }

            return super.onTouchEvent(event);
        }
    }
}
