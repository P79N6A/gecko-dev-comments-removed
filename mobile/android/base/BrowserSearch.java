




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.home.TwoLinePageRow;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.content.res.Configuration;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.SimpleCursorAdapter;




public class BrowserSearch extends Fragment implements LoaderCallbacks<Cursor>,
                                                       AdapterView.OnItemClickListener {
    
    private static final int SEARCH_LOADER_ID = 0;

    
    private String mSearchTerm;

    
    private SearchAdapter mAdapter;

    
    private ListView mList;

    
    private OnUrlOpenListener mUrlOpenListener;

    public interface OnUrlOpenListener {
        public void onUrlOpen(String url);
    }

    public static BrowserSearch newInstance() {
        return new BrowserSearch();
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
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUrlOpenListener = null;
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

        
        getLoaderManager().initLoader(SEARCH_LOADER_ID, null, this);
    }

    @Override
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        return new SearchCursorLoader(getActivity(), mSearchTerm);
    }

    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
        mAdapter.swapCursor(c);

        
    }

    @Override
    public void onLoaderReset(Loader<Cursor> loader) {
        mAdapter.swapCursor(null);
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
            getLoaderManager().restartLoader(SEARCH_LOADER_ID, null, this);
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

    private class SearchAdapter extends SimpleCursorAdapter {
        public SearchAdapter(Context context) {
            super(context, -1, null, new String[] {}, new int[] {});
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final TwoLinePageRow row;
            if (convertView == null) {
                row = (TwoLinePageRow) LayoutInflater.from(getActivity()).inflate(R.layout.home_item_row, null);
            } else {
                row = (TwoLinePageRow) convertView;
            }

            final Cursor c = getCursor();
            if (!c.moveToPosition(position)) {
                throw new IllegalStateException("Couldn't move cursor to position " + position);
            }

            row.updateFromCursor(c);

            

            return row;
        }
    }
}