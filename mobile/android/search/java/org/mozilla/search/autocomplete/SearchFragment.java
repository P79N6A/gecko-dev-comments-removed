



package org.mozilla.search.autocomplete;


import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TextView;

import org.mozilla.search.R;

import java.util.List;







public class SearchFragment extends Fragment
        implements TextView.OnEditorActionListener, AcceptsJumpTaps {

    private static final int LOADER_ID_SUGGESTION = 0;
    private static final String KEY_SEARCH_TERM = "search_term";

    
    private static final int SUGGESTION_TIMEOUT = 3000;

    
    private static final int SUGGESTION_MAX = 5;

    private View mainView;
    private FrameLayout backdropFrame;
    private EditText searchBar;
    private ListView suggestionDropdown;
    private InputMethodManager inputMethodManager;

    private AutoCompleteAdapter autoCompleteAdapter;

    private SuggestClient suggestClient;
    private SuggestionLoaderCallbacks suggestionLoaderCallbacks;

    private State state;

    private enum State {
        WAITING,  
        RUNNING   
    }

    public SearchFragment() {
        
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        mainView = inflater.inflate(R.layout.search_auto_complete, container, false);
        backdropFrame = (FrameLayout) mainView.findViewById(R.id.auto_complete_backdrop);
        searchBar = (EditText) mainView.findViewById(R.id.auto_complete_search_bar);
        suggestionDropdown = (ListView) mainView.findViewById(R.id.auto_complete_dropdown);

        inputMethodManager =
                (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);

        
        searchBar.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                final Bundle args = new Bundle();
                args.putString(KEY_SEARCH_TERM, s.toString());
                getLoaderManager().restartLoader(LOADER_ID_SUGGESTION, args, suggestionLoaderCallbacks);
            }
        });
        searchBar.setOnEditorActionListener(this);
        searchBar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (v.hasFocus()) {
                    return;
                }
                transitionToRunning();
            }
        });

        final Button clearButton = (Button) mainView.findViewById(R.id.clear_button);
        clearButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                searchBar.setText("");
            }
        });

        backdropFrame.setOnClickListener(new BackdropClickListener());

        autoCompleteAdapter = new AutoCompleteAdapter(getActivity(), this);
        suggestionDropdown.setAdapter(autoCompleteAdapter);

        
        transitionToWaiting();

        
        suggestionDropdown.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String query = (String) suggestionDropdown.getItemAtPosition(position);
                startSearch(query);
            }
        });

        
        final String template = "https://search.yahoo.com/sugg/ff?" +
                "output=fxjson&appid=ffm&command=__searchTerms__&nresults=" + SUGGESTION_MAX;

        suggestClient = new SuggestClient(getActivity(), template, SUGGESTION_TIMEOUT, SUGGESTION_MAX);
        suggestionLoaderCallbacks = new SuggestionLoaderCallbacks();

        return mainView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        inputMethodManager = null;
        mainView = null;
        searchBar = null;
        if (null != suggestionDropdown) {
            suggestionDropdown.setOnItemClickListener(null);
            suggestionDropdown.setAdapter(null);
            suggestionDropdown = null;
        }
        autoCompleteAdapter = null;
        suggestClient = null;
        suggestionLoaderCallbacks = null;
    }

    


    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_SEARCH) {
            startSearch(v.getText().toString());
            return true;
        }
        return false;
    }

    


    private void startSearch(String queryString) {
        if (getActivity() instanceof AcceptsSearchQuery) {
            searchBar.setText(queryString);
            searchBar.setSelection(queryString.length());
            transitionToWaiting();
            ((AcceptsSearchQuery) getActivity()).onSearch(queryString);
        } else {
            throw new RuntimeException("Parent activity does not implement AcceptsSearchQuery.");
        }
    }

    private void transitionToWaiting() {
        if (state == State.WAITING) {
            return;
        }
        searchBar.setFocusable(false);
        searchBar.setFocusableInTouchMode(false);
        searchBar.clearFocus();
        inputMethodManager.hideSoftInputFromWindow(searchBar.getWindowToken(), 0);
        suggestionDropdown.setVisibility(View.GONE);
        backdropFrame.setVisibility(View.GONE);
        state = State.WAITING;
    }

    private void transitionToRunning() {
        if (state == State.RUNNING) {
            return;
        }
        searchBar.setFocusable(true);
        searchBar.setFocusableInTouchMode(true);
        searchBar.requestFocus();
        inputMethodManager.showSoftInput(searchBar, InputMethodManager.SHOW_IMPLICIT);
        suggestionDropdown.setVisibility(View.VISIBLE);
        backdropFrame.setVisibility(View.VISIBLE);
        state = State.RUNNING;
    }

    @Override
    public void onJumpTap(String suggestion) {
        searchBar.setText(suggestion);
        
        searchBar.setSelection(suggestion.length());
    }

    private class SuggestionLoaderCallbacks implements LoaderManager.LoaderCallbacks<List<String>> {
        @Override
        public Loader<List<String>> onCreateLoader(int id, Bundle args) {
            
            
            
            return new SuggestionAsyncLoader(getActivity(), suggestClient, args.getString(KEY_SEARCH_TERM));
        }

        @Override
        public void onLoadFinished(Loader<List<String>> loader, List<String> suggestions) {
            autoCompleteAdapter.update(suggestions);
        }

        @Override
        public void onLoaderReset(Loader<List<String>> loader) {
            if (autoCompleteAdapter != null) {
                autoCompleteAdapter.update(null);
            }
        }
    }

    private static class SuggestionAsyncLoader extends AsyncTaskLoader<List<String>> {
        private final SuggestClient suggestClient;
        private final String searchTerm;
        private List<String> suggestions;

        public SuggestionAsyncLoader(Context context, SuggestClient suggestClient, String searchTerm) {
            super(context);
            this.suggestClient = suggestClient;
            this.searchTerm = searchTerm;
            this.suggestions = null;
        }

        @Override
        public List<String> loadInBackground() {
            return suggestClient.query(searchTerm);
        }

        @Override
        public void deliverResult(List<String> suggestions) {
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

    






    private class BackdropClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            transitionToWaiting();
        }
    }
}
