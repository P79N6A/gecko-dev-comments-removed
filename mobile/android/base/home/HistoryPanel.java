




package org.mozilla.gecko.home;

import java.util.Date;
import java.util.EnumSet;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.History;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.util.Log;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.TextView;




public class HistoryPanel extends HomeFragment {
    
    private static final String LOGTAG = "GeckoHistoryPanel";

    
    private static final int LOADER_ID_HISTORY = 0;

    
    private HistoryAdapter mAdapter;

    
    private HomeListView mList;

    
    private View mClearHistoryButton;

    
    private View mEmptyView;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_history_panel, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mList = (HomeListView) view.findViewById(R.id.list);
        mList.setTag(HomePager.LIST_TAG_HISTORY);

        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                position -= mAdapter.getMostRecentSectionsCountBefore(position);
                final Cursor c = mAdapter.getCursor(position);
                final String url = c.getString(c.getColumnIndexOrThrow(History.URL));

                Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.LIST_ITEM);

                
                mUrlOpenListener.onUrlOpen(url, EnumSet.of(OnUrlOpenListener.Flags.ALLOW_SWITCH_TO_TAB));
            }
        });

        mList.setContextMenuInfoFactory(new HomeContextMenuInfo.Factory() {
            @Override
            public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                info.url = cursor.getString(cursor.getColumnIndexOrThrow(Combined.URL));
                info.title = cursor.getString(cursor.getColumnIndexOrThrow(Combined.TITLE));
                info.historyId = cursor.getInt(cursor.getColumnIndexOrThrow(Combined.HISTORY_ID));
                final int bookmarkIdCol = cursor.getColumnIndexOrThrow(Combined.BOOKMARK_ID);
                if (cursor.isNull(bookmarkIdCol)) {
                    
                    
                    info.bookmarkId =  -1;
                } else {
                    info.bookmarkId = cursor.getInt(bookmarkIdCol);
                }
                return info;
            }
        });
        registerForContextMenu(mList);

        mClearHistoryButton = view.findViewById(R.id.clear_history_button);
        mClearHistoryButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final Context context = getActivity();

                final AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(context);
                dialogBuilder.setMessage(R.string.home_clear_history_confirm);

                dialogBuilder.setNegativeButton(R.string.button_cancel, new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(final DialogInterface dialog, final int which) {
                        dialog.dismiss();
                    }
                });

                dialogBuilder.setPositiveButton(R.string.button_ok, new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(final DialogInterface dialog, final int which) {
                        dialog.dismiss();

                        
                        final JSONObject json = new JSONObject();
                        try {
                            json.put("history", true);
                        } catch (JSONException e) {
                            Log.e(LOGTAG, "JSON error", e);
                        }

                        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Sanitize:ClearData", json.toString()));

                        Telemetry.sendUIEvent(TelemetryContract.Event.SANITIZE, TelemetryContract.Method.BUTTON, "history");
                    }
                });

                dialogBuilder.show();
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
        mEmptyView = null;
        mClearHistoryButton = null;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        
        mAdapter = new HistoryAdapter(getActivity());
        mList.setAdapter(mAdapter);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_HISTORY, null, mCursorLoaderCallbacks);
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

    private void updateUiFromCursor(Cursor c) {
        if (c != null && c.getCount() > 0) {
            mClearHistoryButton.setVisibility(View.VISIBLE);
            return;
        }

        
        
        mClearHistoryButton.setVisibility(View.GONE);

        if (mEmptyView == null) {
            
            final ViewStub emptyViewStub = (ViewStub) getView().findViewById(R.id.home_empty_view_stub);
            mEmptyView = emptyViewStub.inflate();

            final ImageView emptyIcon = (ImageView) mEmptyView.findViewById(R.id.home_empty_image);
            emptyIcon.setImageResource(R.drawable.icon_most_recent_empty);

            final TextView emptyText = (TextView) mEmptyView.findViewById(R.id.home_empty_text);
            emptyText.setText(R.string.home_most_recent_empty);

            mList.setEmptyView(mEmptyView);
        }
    }

    private static class HistoryAdapter extends MultiTypeCursorAdapter {
        private static final int ROW_HEADER = 0;
        private static final int ROW_STANDARD = 1;

        private static final int[] VIEW_TYPES = new int[] { ROW_STANDARD, ROW_HEADER };
        private static final int[] LAYOUT_TYPES = new int[] { R.layout.home_item_row, R.layout.home_header_row };

        
        private static final long MS_PER_DAY = 86400000;
        private static final long MS_PER_WEEK = MS_PER_DAY * 7;

        
        private static enum MostRecentSection {
            TODAY,
            YESTERDAY,
            WEEK,
            OLDER
        };

        private final Context mContext;

        
        private final SparseArray<MostRecentSection> mMostRecentSections;

        public HistoryAdapter(Context context) {
            super(context, null, VIEW_TYPES, LAYOUT_TYPES);

            mContext = context;

            
            mMostRecentSections = new SparseArray<MostRecentSection>();
        }

        @Override
        public Object getItem(int position) {
            final int type = getItemViewType(position);

            
            if (type == ROW_HEADER) {
                return null;
            }

            return super.getItem(position - getMostRecentSectionsCountBefore(position));
        }

        @Override
        public int getItemViewType(int position) {
            if (mMostRecentSections.get(position) != null) {
                return ROW_HEADER;
            }

            return ROW_STANDARD;
        }

        @Override
        public boolean isEnabled(int position) {
            return (getItemViewType(position) == ROW_STANDARD);
        }

        @Override
        public int getCount() {
            
            return super.getCount() + mMostRecentSections.size();
        }

        @Override
        public Cursor swapCursor(Cursor cursor) {
            loadMostRecentSections(cursor);
            Cursor oldCursor = super.swapCursor(cursor);
            return oldCursor;
        }

        @Override
        public void bindView(View view, Context context, int position) {
            final int type = getItemViewType(position);

            if (type == ROW_HEADER) {
                final MostRecentSection section = mMostRecentSections.get(position);
                final TextView row = (TextView) view;
                row.setText(getMostRecentSectionTitle(section));
            } else {
                
                position -= getMostRecentSectionsCountBefore(position);
                final Cursor c = getCursor(position);
                final TwoLinePageRow row = (TwoLinePageRow) view;
                row.updateFromCursor(c);
            }
        }

        private String getMostRecentSectionTitle(MostRecentSection section) {
            switch (section) {
            case TODAY:
                return mContext.getString(R.string.history_today_section);
            case YESTERDAY:
                return mContext.getString(R.string.history_yesterday_section);
            case WEEK:
                return mContext.getString(R.string.history_week_section);
            case OLDER:
                return mContext.getString(R.string.history_older_section);
            }

            throw new IllegalStateException("Unrecognized history section");
        }

        private int getMostRecentSectionsCountBefore(int position) {
            
            int sectionsBefore = 0;

            final int historySectionsCount = mMostRecentSections.size();
            for (int i = 0; i < historySectionsCount; i++) {
                final int sectionPosition = mMostRecentSections.keyAt(i);
                if (sectionPosition > position) {
                    break;
                }

                sectionsBefore++;
            }

            return sectionsBefore;
        }

        private static MostRecentSection getMostRecentSectionForTime(long from, long time) {
            long delta = from - time;

            if (delta < 0) {
                return MostRecentSection.TODAY;
            }

            if (delta < MS_PER_DAY) {
                return MostRecentSection.YESTERDAY;
            }

            if (delta < MS_PER_WEEK) {
                return MostRecentSection.WEEK;
            }

            return MostRecentSection.OLDER;
        }

        private void loadMostRecentSections(Cursor c) {
            
            mMostRecentSections.clear();

            if (c == null || !c.moveToFirst()) {
                return;
            }

            final Date now = new Date();
            now.setHours(0);
            now.setMinutes(0);
            now.setSeconds(0);

            final long today = now.getTime();
            MostRecentSection section = null;

            do {
                final int position = c.getPosition();
                final long time = c.getLong(c.getColumnIndexOrThrow(History.DATE_LAST_VISITED));
                final MostRecentSection itemSection = HistoryAdapter.getMostRecentSectionForTime(today, time);

                if (section != itemSection) {
                    section = itemSection;
                    mMostRecentSections.append(position + mMostRecentSections.size(), section);
                }

                
                if (section == MostRecentSection.OLDER) {
                    break;
                }
            } while (c.moveToNext());
        }
    }

    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            return new HistoryCursorLoader(getActivity());
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
