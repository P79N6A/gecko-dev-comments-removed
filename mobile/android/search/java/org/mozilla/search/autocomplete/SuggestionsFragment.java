



package org.mozilla.search.autocomplete;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import org.mozilla.gecko.SuggestClient;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.search.AcceptsSearchQuery;
import org.mozilla.search.AcceptsSearchQuery.SuggestionAnimation;
import org.mozilla.search.R;
import org.mozilla.search.providers.SearchEngine;

import java.util.ArrayList;
import java.util.List;




public class SuggestionsFragment extends Fragment {

    private static final String LOG_TAG = "SuggestionsFragment";

    private static final int LOADER_ID_SUGGESTION = 0;
    private static final String KEY_SEARCH_TERM = "search_term";

    
    private static final int SUGGESTION_TIMEOUT = 3000;

    
    private static final int SUGGESTION_MAX = 5;

    
    private static final int SUGGESTION_HIGHLIGHT_COLOR = 0xFF999999;

    public static final String GECKO_SEARCH_TERMS_URL_PARAM = "__searchTerms__";

    private AcceptsSearchQuery searchListener;

    
    
    private SuggestClient suggestClient;
    private SuggestionLoaderCallbacks suggestionLoaderCallbacks;

    private AutoCompleteAdapter autoCompleteAdapter;

    
    private ListView suggestionsList;

    public SuggestionsFragment() {
        
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        if (activity instanceof AcceptsSearchQuery) {
            searchListener = (AcceptsSearchQuery) activity;
        } else {
            throw new ClassCastException(activity.toString() + " must implement AcceptsSearchQuery.");
        }

        suggestionLoaderCallbacks = new SuggestionLoaderCallbacks();
        autoCompleteAdapter = new AutoCompleteAdapter(activity);
    }

    @Override
    public void onDetach() {
        super.onDetach();

        searchListener = null;
        suggestionLoaderCallbacks = null;
        autoCompleteAdapter = null;
        suggestClient = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        suggestionsList = (ListView) inflater.inflate(R.layout.search_sugestions, container, false);
        suggestionsList.setAdapter(autoCompleteAdapter);

        
        suggestionsList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final Suggestion suggestion = (Suggestion) suggestionsList.getItemAtPosition(position);

                final Rect startBounds = new Rect();
                view.getGlobalVisibleRect(startBounds);

                
                Telemetry.sendUIEvent(TelemetryContract.Event.SEARCH, TelemetryContract.Method.SUGGESTION, "suggest");

                searchListener.onSearch(suggestion.value, new SuggestionAnimation() {
                    @Override
                    public Rect getStartBounds() {
                        return startBounds;
                    }
                });
            }
        });

        return suggestionsList;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        if (null != suggestionsList) {
            suggestionsList.setOnItemClickListener(null);
            suggestionsList.setAdapter(null);
            suggestionsList = null;
        }
    }

    public void setEngine(SearchEngine engine) {
        suggestClient = new SuggestClient(getActivity(), engine.getSuggestionTemplate(GECKO_SEARCH_TERMS_URL_PARAM),
                SUGGESTION_TIMEOUT, SUGGESTION_MAX);
    }

    public void loadSuggestions(String query) {
        final Bundle args = new Bundle();
        args.putString(KEY_SEARCH_TERM, query);
        final LoaderManager loaderManager = getLoaderManager();

        
        
        
        if (loaderManager.getLoader(LOADER_ID_SUGGESTION) == null) {
            loaderManager.initLoader(LOADER_ID_SUGGESTION, args, suggestionLoaderCallbacks);
        } else {
            loaderManager.restartLoader(LOADER_ID_SUGGESTION, args, suggestionLoaderCallbacks);
        }
    }

    public static class Suggestion {

        private static final ForegroundColorSpan COLOR_SPAN =
                new ForegroundColorSpan(SUGGESTION_HIGHLIGHT_COLOR);

        public final String value;
        public final SpannableString display;

        public Suggestion(String value, String searchTerm) {
            this.value = value;

            display = new SpannableString(value);

            
            final int start = value.toLowerCase().indexOf(searchTerm.toLowerCase());
            if (start >= 0) {
                display.setSpan(COLOR_SPAN, start, start + searchTerm.length(), 0);
            }
        }
    }

    private class SuggestionLoaderCallbacks implements LoaderManager.LoaderCallbacks<List<Suggestion>> {
        @Override
        public Loader<List<Suggestion>> onCreateLoader(int id, Bundle args) {
            
            
            if (suggestClient != null) {
                return new SuggestionAsyncLoader(getActivity(), suggestClient, args.getString(KEY_SEARCH_TERM));
            }
            Log.e(LOG_TAG, "Autocomplete setup failed; suggestClient not ready yet.");
            return null;
        }

        @Override
        public void onLoadFinished(Loader<List<Suggestion>> loader, List<Suggestion> suggestions) {
            
            if (suggestions.size() > 0) {
                autoCompleteAdapter.update(suggestions);
                suggestionsList.setVisibility(View.VISIBLE);
            } else {
                suggestionsList.setVisibility(View.INVISIBLE);
            }
        }

        @Override
        public void onLoaderReset(Loader<List<Suggestion>> loader) { }
    }

    private static class SuggestionAsyncLoader extends AsyncTaskLoader<List<Suggestion>> {
        private final SuggestClient suggestClient;
        private final String searchTerm;
        private List<Suggestion> suggestions;

        public SuggestionAsyncLoader(Context context, SuggestClient suggestClient, String searchTerm) {
            super(context);
            this.suggestClient = suggestClient;
            this.searchTerm = searchTerm;
            this.suggestions = null;
        }

        @Override
        public List<Suggestion> loadInBackground() {
            final List<String> values = suggestClient.query(searchTerm);

            final List<Suggestion> result = new ArrayList<Suggestion>(values.size());
            for (String value : values) {
                result.add(new Suggestion(value, searchTerm));
            }

            return result;
        }

        @Override
        public void deliverResult(List<Suggestion> suggestions) {
            this.suggestions = suggestions;

            if (isStarted()) {
                super.deliverResult(suggestions);
            }
        }

        @Override
        protected void onStartLoading() {
            if (suggestions != null) {
                deliverResult(suggestions);
            }

            if (takeContentChanged() || suggestions == null) {
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
            suggestions = null;
        }
    }
}
