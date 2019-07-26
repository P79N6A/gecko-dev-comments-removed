




package org.mozilla.gecko.home;

import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.SessionParser;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.home.HomePager.OnNewTabsListener;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewStub;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;




public class LastTabsPage extends HomeFragment {
    
    private static final String LOGTAG = "GeckoLastTabsPage";

    
    private static final int LOADER_ID_LAST_TABS = 0;

    
    private LastTabsAdapter mAdapter;

    
    private ListView mList;

    
    private TextView mTitle;

    
    private View mRestoreButton;

    
    private View mEmptyView;

    
    private CursorLoaderCallbacks mCursorLoaderCallbacks;

    
    private OnNewTabsListener mNewTabsListener;

    public static LastTabsPage newInstance() {
        return new LastTabsPage();
    }

    public LastTabsPage() {
        mNewTabsListener = null;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mNewTabsListener = (OnNewTabsListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnNewTabsListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();

        mNewTabsListener = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_last_tabs_page, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mTitle = (TextView) view.findViewById(R.id.title);
        if (mTitle != null) {
            mTitle.setText(R.string.home_last_tabs_title);
        }

        mList = (ListView) view.findViewById(R.id.list);
        mList.setTag(HomePager.LIST_TAG_LAST_TABS);

        mList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final Cursor c = mAdapter.getCursor();
                if (c == null || !c.moveToPosition(position)) {
                    return;
                }

                final String url = c.getString(c.getColumnIndexOrThrow(Combined.URL));
                mNewTabsListener.onNewTabs(new String[] { url });
            }
        });

        registerForContextMenu(mList);

        mRestoreButton = view.findViewById(R.id.open_all_tabs_button);
        mRestoreButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openAllTabs();
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList = null;
        mTitle = null;
        mEmptyView = null;
        mRestoreButton = null;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final Activity activity = getActivity();

        
        mAdapter = new LastTabsAdapter(activity);
        mList.setAdapter(mAdapter);

        
        mCursorLoaderCallbacks = new CursorLoaderCallbacks(activity, getLoaderManager());
        loadIfVisible();
    }

    private void updateUiFromCursor(Cursor c) {
        if (c != null && c.getCount() > 0) {
            if (mTitle != null) {
                mTitle.setVisibility(View.VISIBLE);
            }
            mRestoreButton.setVisibility(View.VISIBLE);
            return;
        }

        
        
        if (mTitle != null) {
            mTitle.setVisibility(View.GONE);
        }
        mRestoreButton.setVisibility(View.GONE);

        if (mEmptyView == null) {
            
            final ViewStub emptyViewStub = (ViewStub) getView().findViewById(R.id.home_empty_view_stub);
            mEmptyView = emptyViewStub.inflate();

            final ImageView emptyIcon = (ImageView) mEmptyView.findViewById(R.id.home_empty_image);
            emptyIcon.setImageResource(R.drawable.icon_last_tabs_empty);

            final TextView emptyText = (TextView) mEmptyView.findViewById(R.id.home_empty_text);
            emptyText.setText(R.string.home_last_tabs_empty);

            mList.setEmptyView(mEmptyView);
        }
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_LAST_TABS, null, mCursorLoaderCallbacks);
    }

    private void openAllTabs() {
        final Cursor c = mAdapter.getCursor();
        if (c == null || !c.moveToFirst()) {
            return;
        }

        final String[] urls = new String[c.getCount()];

        do {
            urls[c.getPosition()] = c.getString(c.getColumnIndexOrThrow(Combined.URL));
        } while (c.moveToNext());

        mNewTabsListener.onNewTabs(urls);
    }

    private static class LastTabsCursorLoader extends SimpleCursorLoader {
        public LastTabsCursorLoader(Context context) {
            super(context);
        }

        @Override
        public Cursor loadCursor() {
            final Context context = getContext();

            final String jsonString = GeckoProfile.get(context).readSessionFile(true);
            if (jsonString == null) {
                
                return null;
            }

            final MatrixCursor c = new MatrixCursor(new String[] { Combined._ID,
                                                                   Combined.URL,
                                                                   Combined.TITLE });

            new SessionParser() {
                @Override
                public void onTabRead(SessionTab tab) {
                    final String url = tab.getUrl();

                    
                    if (url.equals("about:home")) {
                        return;
                    }

                    final RowBuilder row = c.newRow();
                    row.add(-1);
                    row.add(url);

                    final String title = tab.getTitle();
                    row.add(title);
                }
            }.parse(jsonString);

            return c;
        }
    }

    private static class LastTabsAdapter extends CursorAdapter {
        public LastTabsAdapter(Context context) {
            super(context, null);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            ((TwoLinePageRow) view).updateFromCursor(cursor);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return LayoutInflater.from(context).inflate(R.layout.home_item_row, parent, false);
        }
    }

    private class CursorLoaderCallbacks extends HomeCursorLoaderCallbacks {
        public CursorLoaderCallbacks(Context context, LoaderManager loaderManager) {
            super(context, loaderManager);
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            if (id == LOADER_ID_LAST_TABS) {
                return new LastTabsCursorLoader(getActivity());
            } else {
                return super.onCreateLoader(id, args);
            }
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            if (loader.getId() == LOADER_ID_LAST_TABS) {
                mAdapter.swapCursor(c);
                updateUiFromCursor(c);
                loadFavicons(c);
            } else {
                super.onLoadFinished(loader, c);
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            if (loader.getId() == LOADER_ID_LAST_TABS) {
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
