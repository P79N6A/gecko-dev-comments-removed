




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.TwoLinePageRow;
import org.mozilla.gecko.ReaderModeUtils;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ImageSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

import java.util.EnumSet;




public class ReadingListPage extends HomeFragment {
    
    private static final int LOADER_ID_READING_LIST = 0;

    
    private ReadingListAdapter mAdapter;

    
    private ListView mList;

    
    private View mEmptyView;

    
    private View mTopView;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private OnUrlOpenListener mUrlOpenListener;

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mUrlOpenListener = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_reading_list_page, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mTopView = view;

        mList = (ListView) view.findViewById(R.id.list);
        mList.setTag(HomePager.LIST_TAG_READING_LIST);

        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final Cursor c = mAdapter.getCursor();
                if (c == null || !c.moveToPosition(position)) {
                    return;
                }

                String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));
                url = ReaderModeUtils.getAboutReaderForUrl(url);

                
                mUrlOpenListener.onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        registerForContextMenu(mList);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
        mTopView = null;
        mEmptyView = null;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mAdapter = new ReadingListAdapter(getActivity(), null);
        mList.setAdapter(mAdapter);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_READING_LIST, null, mCursorLoaderCallbacks);
    }

    private void updateUiFromCursor(Cursor c) {
        
        
        if ((c == null || c.getCount() == 0) && mEmptyView == null) {
            final ViewStub emptyViewStub = (ViewStub) mTopView.findViewById(R.id.home_empty_view_stub);
            mEmptyView = emptyViewStub.inflate();

            final TextView emptyHint = (TextView) mEmptyView.findViewById(R.id.home_empty_hint);
            String readingListHint = emptyHint.getText().toString();

            
            int imageSpanIndex = readingListHint.indexOf("%I");
            if (imageSpanIndex != -1) {
                final ImageSpan readingListIcon = new ImageSpan(getActivity(), R.drawable.reader_cropped, ImageSpan.ALIGN_BOTTOM);
                final SpannableStringBuilder hintBuilder = new SpannableStringBuilder(readingListHint);

                
                hintBuilder.insert(imageSpanIndex + 2, " ");
                hintBuilder.insert(imageSpanIndex, " ");

                
                hintBuilder.setSpan(readingListIcon, imageSpanIndex + 1, imageSpanIndex + 3, Spanned.SPAN_INCLUSIVE_INCLUSIVE);

                emptyHint.setText(hintBuilder, TextView.BufferType.SPANNABLE);
            }

            mList.setEmptyView(mEmptyView);
        }
    }

    


    private static class ReadingListLoader extends SimpleCursorLoader {
        public ReadingListLoader(Context context) {
            super(context);
        }

        @Override
        public Cursor loadCursor() {
            return BrowserDB.getBookmarksInFolder(getContext().getContentResolver(), Bookmarks.FIXED_READING_LIST_ID);
        }
    }

    


    private class ReadingListAdapter extends CursorAdapter {
        public ReadingListAdapter(Context context, Cursor cursor) {
            super(context, cursor);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            final TwoLinePageRow row = (TwoLinePageRow) view;
            row.updateFromCursor(cursor);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return LayoutInflater.from(parent.getContext()).inflate(R.layout.bookmark_item_row, parent, false);
        }
    }

    


    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            return new ReadingListLoader(getActivity());
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            mAdapter.swapCursor(c);
            updateUiFromCursor(c);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            mAdapter.swapCursor(null);
        }
    }
}
