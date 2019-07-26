




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserDB;

import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.text.TextUtils;




class SearchLoader {
    
    private static final String KEY_SEARCH_TERM = "search_term";

    
    private static final String KEY_PERFORM_EMPTY_SEARCH = "perform_empty_search";

    private SearchLoader() {
    }

    public static Loader<Cursor> createInstance(Context context, Bundle args) {
        if (args != null) {
            final String searchTerm = args.getString(KEY_SEARCH_TERM);
            final boolean performEmptySearch = args.getBoolean(KEY_PERFORM_EMPTY_SEARCH, false);
            return new SearchCursorLoader(context, searchTerm, performEmptySearch);
        } else {
            return new SearchCursorLoader(context, "", false);
        }
    }

    private static Bundle createArgs(String searchTerm, boolean performEmptySearch) {
        Bundle args = new Bundle();
        args.putString(SearchLoader.KEY_SEARCH_TERM, searchTerm);
        args.putBoolean(SearchLoader.KEY_PERFORM_EMPTY_SEARCH, performEmptySearch);

        return args;
    }

    public static void restart(LoaderManager manager, int loaderId,
                               LoaderCallbacks<Cursor> callbacks, String searchTerm) {
        restart(manager, loaderId, callbacks, searchTerm, true);
    }

    public static void restart(LoaderManager manager, int loaderId,
                               LoaderCallbacks<Cursor> callbacks, String searchTerm, boolean performEmptySearch) {
        Bundle args = createArgs(searchTerm, performEmptySearch);
        manager.restartLoader(loaderId, args, callbacks);
    }

    public static class SearchCursorLoader extends SimpleCursorLoader {
        
        private static final int SEARCH_LIMIT = 100;

        
        private final String mSearchTerm;

        
        private final boolean mPerformEmptySearch;

        public SearchCursorLoader(Context context, String searchTerm, boolean performEmptySearch) {
            super(context);
            mSearchTerm = searchTerm;
            mPerformEmptySearch = performEmptySearch;
        }

        @Override
        public Cursor loadCursor() {
            if (!mPerformEmptySearch && TextUtils.isEmpty(mSearchTerm)) {
                return null;
            }

            return BrowserDB.filter(getContext().getContentResolver(), mSearchTerm, SEARCH_LIMIT);
        }

        public String getSearchTerm() {
            return mSearchTerm;
        }
    }

}
