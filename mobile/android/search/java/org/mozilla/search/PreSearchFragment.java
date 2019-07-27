



package org.mozilla.search;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.support.v4.widget.SimpleCursorAdapter;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.SearchHistory;
import org.mozilla.search.AcceptsSearchQuery.SuggestionAnimation;




public class PreSearchFragment extends Fragment {

    private AcceptsSearchQuery searchListener;
    private SimpleCursorAdapter cursorAdapter;

    private ListView listView;

    private static final String[] PROJECTION = new String[]{ SearchHistory.QUERY, SearchHistory._ID };

    
    
    private static final Uri SEARCH_HISTORY_URI = SearchHistory.CONTENT_URI.buildUpon().
            appendQueryParameter(BrowserContract.PARAM_LIMIT, String.valueOf(Constants.SUGGESTION_MAX)).build();

    private static final int LOADER_ID_SEARCH_HISTORY = 1;

    public PreSearchFragment() {
        
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        if (activity instanceof AcceptsSearchQuery) {
            searchListener = (AcceptsSearchQuery) activity;
        } else {
            throw new ClassCastException(activity.toString() + " must implement AcceptsSearchQuery.");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        searchListener = null;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getLoaderManager().initLoader(LOADER_ID_SEARCH_HISTORY, null, new SearchHistoryLoaderCallbacks());
        cursorAdapter = new SimpleCursorAdapter(getActivity(), R.layout.search_card_history, null,
                PROJECTION, new int[]{R.id.site_name}, 0);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        getLoaderManager().destroyLoader(LOADER_ID_SEARCH_HISTORY);
        cursorAdapter.swapCursor(null);
        cursorAdapter = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedState) {
        final View mainView = inflater.inflate(R.layout.search_fragment_pre_search, container, false);

        
        listView = (ListView) mainView.findViewById(R.id.list_view);
        listView.setAdapter(cursorAdapter);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final Cursor c = cursorAdapter.getCursor();
                if (c == null || !c.moveToPosition(position)) {
                    return;
                }
                final String query = c.getString(c.getColumnIndexOrThrow(SearchHistory.QUERY));
                if (!TextUtils.isEmpty(query)) {
                    final Rect startBounds = new Rect();
                    view.getGlobalVisibleRect(startBounds);

                    searchListener.onSearch(query, new SuggestionAnimation() {
                        @Override
                        public Rect getStartBounds() {
                            return startBounds;
                        }

                        @Override
                        public void onAnimationEnd() {
                        }
                    });
                }
            }
        });

        
        mainView.findViewById(R.id.settings_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(getActivity(), SearchPreferenceActivity.class));
            }
        });
        return mainView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        listView.setAdapter(null);
        listView = null;
    }

    private class SearchHistoryLoaderCallbacks implements LoaderManager.LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            return new CursorLoader(getActivity(), SEARCH_HISTORY_URI, PROJECTION, null, null,
                    SearchHistory.DATE_LAST_VISITED + " DESC");
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
            if (cursorAdapter != null) {
                cursorAdapter.swapCursor(data);
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            if (cursorAdapter != null) {
                cursorAdapter.swapCursor(null);
            }
        }
    }
}
