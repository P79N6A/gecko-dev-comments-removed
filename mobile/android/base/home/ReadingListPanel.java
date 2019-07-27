




package org.mozilla.gecko.home;

import java.util.EnumSet;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.db.BrowserContract.URLColumns;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.home.HomeContextMenuInfo.RemoveItemType;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
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
import android.widget.TextView;




public class ReadingListPanel extends HomeFragment {

    
    private static final int LOADER_ID_READING_LIST = 0;

    
    private final String MATCH_STRING = "%I";

    
    private ReadingListAdapter mAdapter;

    
    private HomeListView mList;

    
    private View mEmptyView;

    
    private View mTopView;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_reading_list_panel, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mTopView = view;

        mList = (HomeListView) view.findViewById(R.id.list);
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

                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM);

                
                mUrlOpenListener.onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        mList.setContextMenuInfoFactory(new HomeContextMenuInfo.Factory() {
            @Override
            public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                info.url = cursor.getString(cursor.getColumnIndexOrThrow(ReadingListItems.URL));
                info.title = cursor.getString(cursor.getColumnIndexOrThrow(ReadingListItems.TITLE));
                info.readingListItemId = cursor.getInt(cursor.getColumnIndexOrThrow(ReadingListItems._ID));
                info.itemType = RemoveItemType.READING_LIST;
                return info;
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

            
            int imageSpanIndex = readingListHint.indexOf(MATCH_STRING);
            if (imageSpanIndex != -1) {
                final ImageSpan readingListIcon = new ImageSpan(getActivity(), R.drawable.reader_cropped, ImageSpan.ALIGN_BOTTOM);
                final SpannableStringBuilder hintBuilder = new SpannableStringBuilder(readingListHint);

                
                hintBuilder.insert(imageSpanIndex + MATCH_STRING.length(), " ");
                hintBuilder.insert(imageSpanIndex, " ");

                
                hintBuilder.setSpan(readingListIcon, imageSpanIndex + 1, imageSpanIndex + MATCH_STRING.length() + 1, Spanned.SPAN_INCLUSIVE_INCLUSIVE);

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
            return BrowserDB.getReadingList(getContext().getContentResolver());
        }
    }

    


    private class ReadingListAdapter extends CursorAdapter {
        public ReadingListAdapter(Context context, Cursor cursor) {
            super(context, cursor, 0);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            final ReadingListRow row = (ReadingListRow) view;
            row.updateFromCursor(cursor);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return LayoutInflater.from(parent.getContext()).inflate(R.layout.reading_list_item_row, parent, false);
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
