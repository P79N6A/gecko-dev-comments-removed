



package org.mozilla.search.autocomplete;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import org.mozilla.search.R;

import java.util.ArrayList;
import java.util.List;







public class SearchFragment extends Fragment implements AcceptsJumpTaps {

    private static final int LOADER_ID_SUGGESTION = 0;
    private static final String KEY_SEARCH_TERM = "search_term";

    
    private static final int SUGGESTION_TIMEOUT = 3000;

    
    private static final int SUGGESTION_MAX = 5;

    
    private static final int SUGGESTION_HIGHLIGHT_COLOR = 0xFF999999;

    private AcceptsSearchQuery searchListener;
    private SuggestClient suggestClient;
    private SuggestionLoaderCallbacks suggestionLoaderCallbacks;

    private AutoCompleteAdapter autoCompleteAdapter;

    private View mainView;
    private ClearableEditText editText;
    private ListView suggestionDropdown;

    private State state = State.WAITING;

    private static enum State {
        WAITING,  
        RUNNING   
    }

    public SearchFragment() {
        
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        if (activity instanceof AcceptsSearchQuery) {
            searchListener = (AcceptsSearchQuery) activity;
        } else {
            throw new ClassCastException(activity.toString() + " must implement AcceptsSearchQuery.");
        }

        
        final String template = "https://search.yahoo.com/sugg/ff?" +
                "output=fxjson&appid=ffm&command=__searchTerms__&nresults=" + SUGGESTION_MAX;

        suggestClient = new SuggestClient(activity, template, SUGGESTION_TIMEOUT, SUGGESTION_MAX);
        suggestionLoaderCallbacks = new SuggestionLoaderCallbacks();

        autoCompleteAdapter = new AutoCompleteAdapter(activity, this);
    }

    @Override
    public void onDetach() {
        super.onDetach();

        searchListener = null;
        suggestClient = null;
        suggestionLoaderCallbacks = null;
        autoCompleteAdapter = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        mainView = inflater.inflate(R.layout.search_auto_complete, container, false);

        
        mainView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transitionToWaiting();
            }
        });

        
        
        
        mainView.setClickable(false);

        editText = (ClearableEditText) mainView.findViewById(R.id.auto_complete_edit_text);
        editText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transitionToRunning();
            }
        });

        editText.setTextListener(new ClearableEditText.TextListener() {
            @Override
            public void onChange(String text) {
                if (state == State.RUNNING) {
                    final Bundle args = new Bundle();
                    args.putString(KEY_SEARCH_TERM, text);
                    getLoaderManager().restartLoader(LOADER_ID_SUGGESTION, args, suggestionLoaderCallbacks);
                }
            }

            @Override
            public void onSubmit(String text) {
                transitionToWaiting();
                searchListener.onSearch(text);
            }
        });

        suggestionDropdown = (ListView) mainView.findViewById(R.id.auto_complete_dropdown);
        suggestionDropdown.setAdapter(autoCompleteAdapter);

        
        suggestionDropdown.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final Suggestion suggestion = (Suggestion) suggestionDropdown.getItemAtPosition(position);

                transitionToWaiting();
                searchListener.onSearch(suggestion.value);
            }
        });

        return mainView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        editText = null;

        if (null != suggestionDropdown) {
            suggestionDropdown.setOnItemClickListener(null);
            suggestionDropdown.setAdapter(null);
            suggestionDropdown = null;
        }
    }

    @Override
    public void onJumpTap(String suggestion) {
        setSearchTerm(suggestion);
    }

    





    public void setSearchTerm(String searchTerm) {
        editText.setText(searchTerm);
    }

    private void transitionToWaiting() {
        if (state == State.WAITING) {
            return;
        }
        state = State.WAITING;

        mainView.setClickable(false);
        editText.setActive(false);
        suggestionDropdown.setVisibility(View.GONE);
    }

    private void transitionToRunning() {
        if (state == State.RUNNING) {
            return;
        }
        state = State.RUNNING;

        mainView.setClickable(true);
        editText.setActive(true);
        suggestionDropdown.setVisibility(View.VISIBLE);
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
                display.setSpan(COLOR_SPAN, start, searchTerm.length(), 0);
            }
        }
    }

    private class SuggestionLoaderCallbacks implements LoaderManager.LoaderCallbacks<List<Suggestion>> {
        @Override
        public Loader<List<Suggestion>> onCreateLoader(int id, Bundle args) {
            return new SuggestionAsyncLoader(getActivity(), suggestClient, args.getString(KEY_SEARCH_TERM));
        }

        @Override
        public void onLoadFinished(Loader<List<Suggestion>> loader, List<Suggestion> suggestions) {
            autoCompleteAdapter.update(suggestions);
        }

        @Override
        public void onLoaderReset(Loader<List<Suggestion>> loader) {
            autoCompleteAdapter.update(null);
        }
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
