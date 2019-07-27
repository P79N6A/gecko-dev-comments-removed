




package org.mozilla.gecko.home;

import java.util.List;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.home.BookmarksListAdapter.FolderInfo;
import org.mozilla.gecko.home.BookmarksListAdapter.OnRefreshFolderListener;
import org.mozilla.gecko.home.BookmarksListAdapter.RefreshType;
import org.mozilla.gecko.home.HomeContextMenuInfo.RemoveItemType;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.ImageView;
import android.widget.TextView;




public class BookmarksPanel extends HomeFragment {
    public static final String LOGTAG = "GeckoBookmarksPanel";

    
    private static final int LOADER_ID_BOOKMARKS_LIST = 0;

    
    private static final String BOOKMARKS_FOLDER_INFO = "folder_info";

    
    private static final String BOOKMARKS_REFRESH_TYPE = "refresh_type";

    
    private BookmarksListView mList;

    
    private BookmarksListAdapter mListAdapter;

    
    private List<FolderInfo> mSavedParentStack;

    
    private View mEmptyView;

    
    private CursorLoaderCallbacks mLoaderCallbacks;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.home_bookmarks_panel, container, false);

        mList = (BookmarksListView) view.findViewById(R.id.bookmarks_list);

        mList.setContextMenuInfoFactory(new HomeContextMenuInfo.Factory() {
            @Override
            public HomeContextMenuInfo makeInfoForCursor(View view, int position, long id, Cursor cursor) {
                final int type = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks.TYPE));
                if (type == Bookmarks.TYPE_FOLDER) {
                    
                    return null;
                }
                final HomeContextMenuInfo info = new HomeContextMenuInfo(view, position, id);
                info.url = cursor.getString(cursor.getColumnIndexOrThrow(Bookmarks.URL));
                info.title = cursor.getString(cursor.getColumnIndexOrThrow(Bookmarks.TITLE));
                info.bookmarkId = cursor.getInt(cursor.getColumnIndexOrThrow(Bookmarks._ID));
                info.itemType = RemoveItemType.BOOKMARKS;
                return info;
            }
        });

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        OnUrlOpenListener listener = null;
        try {
            listener = (OnUrlOpenListener) getActivity();
        } catch (ClassCastException e) {
            throw new ClassCastException(getActivity().toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }

        mList.setTag(HomePager.LIST_TAG_BOOKMARKS);
        mList.setOnUrlOpenListener(listener);

        registerForContextMenu(mList);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final Activity activity = getActivity();

        
        mListAdapter = new BookmarksListAdapter(activity, null, mSavedParentStack);
        mListAdapter.setOnRefreshFolderListener(new OnRefreshFolderListener() {
            @Override
            public void onRefreshFolder(FolderInfo folderInfo, RefreshType refreshType) {
                
                Bundle bundle = new Bundle();
                bundle.putParcelable(BOOKMARKS_FOLDER_INFO, folderInfo);
                bundle.putParcelable(BOOKMARKS_REFRESH_TYPE, refreshType);
                getLoaderManager().restartLoader(LOADER_ID_BOOKMARKS_LIST, bundle, mLoaderCallbacks);
            }
        });
        mList.setAdapter(mListAdapter);

        
        mLoaderCallbacks = new CursorLoaderCallbacks();
        loadIfVisible();
    }

    @Override
    public void onDestroyView() {
        mList = null;
        mListAdapter = null;
        mEmptyView = null;
        super.onDestroyView();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (isVisible()) {
            
            
            mSavedParentStack = mListAdapter.getParentStack();
        }
    }

    @Override
    protected void load() {
        getLoaderManager().initLoader(LOADER_ID_BOOKMARKS_LIST, null, mLoaderCallbacks);
    }

    private void updateUiFromCursor(Cursor c) {
        if ((c == null || c.getCount() == 0) && mEmptyView == null) {
            
            final ViewStub emptyViewStub = (ViewStub) getView().findViewById(R.id.home_empty_view_stub);
            mEmptyView = emptyViewStub.inflate();

            final ImageView emptyIcon = (ImageView) mEmptyView.findViewById(R.id.home_empty_image);
            emptyIcon.setImageResource(R.drawable.icon_bookmarks_empty);

            final TextView emptyText = (TextView) mEmptyView.findViewById(R.id.home_empty_text);
            emptyText.setText(R.string.home_bookmarks_empty);

            mList.setEmptyView(mEmptyView);
        }
    }

    


    private static class BookmarksLoader extends SimpleCursorLoader {
        private final FolderInfo mFolderInfo;
        private final RefreshType mRefreshType;

        public BookmarksLoader(Context context) {
            super(context);
            final Resources res = context.getResources();
            final String title = res.getString(R.string.bookmarks_title);
            mFolderInfo = new FolderInfo(Bookmarks.FIXED_ROOT_ID, title);
            mRefreshType = RefreshType.CHILD;
        }

        public BookmarksLoader(Context context, FolderInfo folderInfo, RefreshType refreshType) {
            super(context);
            mFolderInfo = folderInfo;
            mRefreshType = refreshType;
        }

        @Override
        public Cursor loadCursor() {
            return BrowserDB.getBookmarksInFolder(getContext().getContentResolver(), mFolderInfo.id);
        }

        @Override
        public void onContentChanged() {
            
            
            BrowserDB.invalidateCachedState();
            super.onContentChanged();
        }

        public FolderInfo getFolderInfo() {
            return mFolderInfo;
        }

        public RefreshType getRefreshType() {
            return mRefreshType;
        }
    }

    


    private class CursorLoaderCallbacks implements LoaderCallbacks<Cursor> {
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            if (args == null) {
                return new BookmarksLoader(getActivity());
            } else {
                FolderInfo folderInfo = (FolderInfo) args.getParcelable(BOOKMARKS_FOLDER_INFO);
                RefreshType refreshType = (RefreshType) args.getParcelable(BOOKMARKS_REFRESH_TYPE);
                return new BookmarksLoader(getActivity(), folderInfo, refreshType);
            }
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            BookmarksLoader bl = (BookmarksLoader) loader;
            mListAdapter.swapCursor(c, bl.getFolderInfo(), bl.getRefreshType());
            updateUiFromCursor(c);
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            if (mList != null) {
                mListAdapter.swapCursor(null);
            }
        }
    }
}
