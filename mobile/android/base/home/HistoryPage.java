




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.TwoLinePageRow;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.widget.SimpleCursorAdapter;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

import java.util.Date;




public class HistoryPage extends HomeFragment {
    
    private static final String LOGTAG = "GeckoHistoryPage";

    
    private static final int HISTORY_LOADER_ID = 0;

    
    private static final long MS_PER_DAY = 86400000;
    private static final long MS_PER_WEEK = MS_PER_DAY * 7;

    
    private static enum HistorySection {
        TODAY,
        YESTERDAY,
        WEEK,
        OLDER
    };

    
    private SparseArray<HistorySection> mHistorySections;

    
    private HistoryAdapter mAdapter;

    
    private ListView mList;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private LayoutInflater mInflater;

    
    private OnUrlOpenListener mUrlOpenListener;

    public static HistoryPage newInstance() {
        return new HistoryPage();
    }

    public HistoryPage() {
        mUrlOpenListener = null;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }

        mInflater = (LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mHistorySections = null;
        mInflater = null;
        mUrlOpenListener = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_history_page, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        final TextView title = (TextView) view.findViewById(R.id.title);
        title.setText(R.string.history_title);

        mList = (ListView) view.findViewById(R.id.list);

        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                position -= getHistorySectionsCountBefore(position);

                final Cursor c = mAdapter.getCursor();
                if (c == null || !c.moveToPosition(position)) {
                    return;
                }

                final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));
                mUrlOpenListener.onUrlOpen(url);
            }
        });

        registerForContextMenu(mList);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mHistorySections = new SparseArray<HistorySection>();

        
        mAdapter = new HistoryAdapter(getActivity());
        mList.setAdapter(mAdapter);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();

        
        getLoaderManager().initLoader(HISTORY_LOADER_ID, null, mCursorLoaderCallbacks);
    }

    private String getHistorySectionTitle(HistorySection section) {
        final Resources resources = getActivity().getResources();

        switch (section) {
        case TODAY:
            return resources.getString(R.string.history_today_section);
        case YESTERDAY:
            return resources.getString(R.string.history_yesterday_section);
        case WEEK:
            return resources.getString(R.string.history_week_section);
        case OLDER:
            return resources.getString(R.string.history_older_section);
        }

        throw new IllegalStateException("Unrecognized history section");
    }

    private int getHistorySectionsCountBefore(int position) {
        
        int sectionsBefore = 0;

        final int historySectionsCount = mHistorySections.size();
        for (int i = 0; i < historySectionsCount; i++) {
            final int sectionPosition = mHistorySections.keyAt(i);
            if (sectionPosition > position) {
                break;
            }

            sectionsBefore++;
        }

        return sectionsBefore;
    }

    private HistorySection getHistorySectionForTime(long from, long time) {
        long delta = from - time;

        if (delta < 0) {
            return HistorySection.TODAY;
        }

        if (delta < MS_PER_DAY) {
            return HistorySection.YESTERDAY;
        }

        if (delta < MS_PER_WEEK) {
            return HistorySection.WEEK;
        }

        return HistorySection.OLDER;
    }

    private void loadHistorySections(Cursor c) {
        if (c == null || !c.moveToFirst()) {
            return;
        }

        final Date now = new Date();
        now.setHours(0);
        now.setMinutes(0);
        now.setSeconds(0);

        final long today = now.getTime();
        HistorySection section = null;

        do {
            final int position = c.getPosition();
            final long time = c.getLong(c.getColumnIndexOrThrow(URLColumns.DATE_LAST_VISITED));
            final HistorySection itemSection = getHistorySectionForTime(today, time);

            if (section != itemSection) {
                section = itemSection;
                mHistorySections.append(position + mHistorySections.size(), section);
            }

            
            if (section == HistorySection.OLDER) {
                break;
            }
        } while (c.moveToNext());
    }

    private static class HistoryCursorLoader extends SimpleCursorLoader {
        
        private static final int HISTORY_LIMIT = 100;

        public HistoryCursorLoader(Context context) {
            super(context);
        }

        @Override
        public Cursor loadCursor() {
            final ContentResolver cr = getContext().getContentResolver();
            return BrowserDB.getRecentHistory(cr, HISTORY_LIMIT);
        }
    }

    private class HistoryAdapter extends SimpleCursorAdapter {
        private static final int ROW_HEADER = 0;
        private static final int ROW_STANDARD = 1;

        private static final int ROW_TYPE_COUNT = 2;

        public HistoryAdapter(Context context) {
            super(context, -1, null, new String[] {}, new int[] {});
        }

        @Override
        public Object getItem(int position) {
            final int type = getItemViewType(position);

            
            if (type == ROW_HEADER) {
                return null;
            }

            return super.getItem(position - getHistorySectionsCountBefore(position));
        }

        @Override
        public int getItemViewType(int position) {
            if (mHistorySections.get(position) != null) {
                return ROW_HEADER;
            }

            return ROW_STANDARD;
        }

        @Override
        public int getViewTypeCount() {
            
            return ROW_TYPE_COUNT;
        }

        @Override
        public boolean isEnabled(int position) {
            return (getItemViewType(position) == ROW_STANDARD);
        }

        @Override
        public int getCount() {
            
            return super.getCount() + mHistorySections.size();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final int type = getItemViewType(position);

            if (type == ROW_HEADER) {
                final TextView row;
                if (convertView == null) {
                    row = (TextView) mInflater.inflate(R.layout.home_header_row, mList, false);
                } else {
                    row = (TextView) convertView;
                }

                final HistorySection section = mHistorySections.get(position);
                row.setText(getHistorySectionTitle(section));

                return row;
            } else {
                final TwoLinePageRow row;
                if (convertView == null) {
                    row = (TwoLinePageRow) mInflater.inflate(R.layout.home_item_row, mList, false);
                } else {
                    row = (TwoLinePageRow) convertView;
                }

                
                position -= getHistorySectionsCountBefore(position);

                final Cursor c = getCursor();
                if (!c.moveToPosition(position)) {
                    throw new IllegalStateException("Couldn't move cursor to position " + position);
                }

                row.updateFromCursor(c);

                return row;
            }
        }
    }

    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            return new HistoryCursorLoader(getActivity());
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            loadHistorySections(c);
            mAdapter.swapCursor(c);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            mAdapter.swapCursor(null);
        }
    }
}
