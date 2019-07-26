




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB.URLColumns;

import android.app.Activity;
import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ListView;




class PinBookmarkDialog extends DialogFragment {
    
    public static interface OnBookmarkSelectedListener {
        public void onBookmarkSelected(String url, String title);
    }

    
    private static final int LOADER_ID_SEARCH = 0;

    
    private static final int LOADER_ID_FAVICONS = 1;

    
    private String mSearchTerm;

    
    private SearchAdapter mAdapter;

    
    private EditText mSearch;

    
    private ListView mList;

    
    private CursorLoaderCallbacks mLoaderCallbacks;

    
    private OnBookmarkSelectedListener mOnBookmarkSelectedListener;

    public static PinBookmarkDialog newInstance() {
        return new PinBookmarkDialog();
    }

    private PinBookmarkDialog() {
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setStyle(DialogFragment.STYLE_NO_TITLE, android.R.style.Theme_Holo_Light_Dialog);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        
        
        return inflater.inflate(R.layout.pin_bookmark_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mSearch = (EditText) view.findViewById(R.id.search);
        mSearch.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                filter(mSearch.getText().toString());
            }
        });

        mList = (HomeListView) view.findViewById(R.id.list);
        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (mOnBookmarkSelectedListener != null) {
                    final Cursor c = mAdapter.getCursor();
                    if (c == null || !c.moveToPosition(position)) {
                        return;
                    }

                    final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));
                    final String title = c.getString(c.getColumnIndexOrThrow(URLColumns.TITLE));
                    mOnBookmarkSelectedListener.onBookmarkSelected(url, title);
                }

                
                dismiss();
            }
        });
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final Activity activity = getActivity();
        final LoaderManager manager = getLoaderManager();

        
        mAdapter = new SearchAdapter(activity);
        mList.setAdapter(mAdapter);

        
        mLoaderCallbacks = new CursorLoaderCallbacks(activity, manager);

        
        manager.initLoader(LOADER_ID_SEARCH, null, mLoaderCallbacks);

        
        filter("");
    }

    private void filter(String searchTerm) {
        if (!TextUtils.isEmpty(searchTerm) &&
            TextUtils.equals(mSearchTerm, searchTerm)) {
            return;
        }

        mSearchTerm = searchTerm;

        
        SearchLoader.restart(getLoaderManager(), LOADER_ID_SEARCH, mLoaderCallbacks, mSearchTerm);
    }

    public void setOnBookmarkSelectedListener(OnBookmarkSelectedListener listener) {
        mOnBookmarkSelectedListener = listener;
    }

    private static class SearchAdapter extends CursorAdapter {
        private LayoutInflater mInflater;

        public SearchAdapter(Context context) {
            super(context, null);
            mInflater = LayoutInflater.from(context);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            TwoLinePageRow row = (TwoLinePageRow) view;
            row.setShowIcons(false);
            row.updateFromCursor(cursor);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return (TwoLinePageRow) mInflater.inflate(R.layout.home_item_row, parent, false);
        }
    }

    private class CursorLoaderCallbacks extends HomeCursorLoaderCallbacks {
        public CursorLoaderCallbacks(Context context, LoaderManager loaderManager) {
            super(context, loaderManager);
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            if (id == LOADER_ID_SEARCH) {
                return SearchLoader.createInstance(getActivity(), args);
            } else {
                return super.onCreateLoader(id, args);
            }
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            if (loader.getId() == LOADER_ID_SEARCH) {
                mAdapter.swapCursor(c);
                loadFavicons(c);
            } else {
                super.onLoadFinished(loader, c);
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            if (loader.getId() == LOADER_ID_SEARCH) {
                mAdapter.swapCursor(null);
            } else {
                super.onLoaderReset(loader);
            }
        }

        @Override
        public void onFaviconsLoaded() {
            mAdapter.notifyDataSetChanged();
        }
    }
}
