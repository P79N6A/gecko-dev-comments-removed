




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

    private SearchLoader() {
    }

    public static Loader<Cursor> createInstance(Context context, Bundle args) {
        if (args != null) {
            final String searchTerm = args.getString(KEY_SEARCH_TERM);
            return new SearchCursorLoader(context, searchTerm);
        } else {
            return new SearchCursorLoader(context, "");
        }
    }

    private static Bundle createArgs(String searchTerm) {
        Bundle args = new Bundle();
        args.putString(SearchLoader.KEY_SEARCH_TERM, searchTerm);

        return args;
    }

    public static void init(LoaderManager manager, int loaderId,
                               LoaderCallbacks<Cursor> callbacks, String searchTerm) {
        final Bundle args = createArgs(searchTerm);
        manager.initLoader(loaderId, args, callbacks);
    }

    public static void restart(LoaderManager manager, int loaderId,
                               LoaderCallbacks<Cursor> callbacks, String searchTerm) {
        final Bundle args = createArgs(searchTerm);
        manager.restartLoader(loaderId, args, callbacks);
    }

    public static class SearchCursorLoader extends SimpleCursorLoader {
        
        private static final int SEARCH_LIMIT = 100;

        
        private final String mSearchTerm;

        public SearchCursorLoader(Context context, String searchTerm) {
            super(context);
            mSearchTerm = searchTerm;
        }

        @Override
        public Cursor loadCursor() {
            return BrowserDB.filter(getContext().getContentResolver(), mSearchTerm, SEARCH_LIMIT);
        }

        public String getSearchTerm() {
            return mSearchTerm;
        }
    }

}
