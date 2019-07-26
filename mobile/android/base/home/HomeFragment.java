




package org.mozilla.gecko.home;

import org.mozilla.gecko.EditBookmarkDialog;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.home.HomeListView.HomeContextMenuInfo;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.Loader;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;





abstract class HomeFragment extends Fragment {
    
    private static final String LOGTAG="GeckoHomeFragment";

    
    private static final String SHARE_MIME_TYPE = "text/plain";

    
    private static final String REGEX_URL_TO_TITLE = "^([a-z]+://)?(www\\.)?";

    
    private static final int LOADER_ID_FAVICONS = 100;

    protected void showSubPage(Fragment subPage) {
        getActivity().getSupportFragmentManager().beginTransaction()
                .addToBackStack(null).replace(R.id.home_pager_container, subPage, HomePager.SUBPAGE_TAG)
                .commitAllowingStateLoss();
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
        if (menuInfo == null || !(menuInfo instanceof HomeContextMenuInfo)) {
            return;
        }

        HomeContextMenuInfo info = (HomeContextMenuInfo) menuInfo;

        
        if (info.isFolder) {
            return;
        }

        MenuInflater inflater = new MenuInflater(view.getContext());
        inflater.inflate(R.menu.home_contextmenu, menu);

        menu.setHeaderTitle(info.title);

        
        if (info.bookmarkId < 0) {
            menu.findItem(R.id.home_edit_bookmark).setVisible(false);
        }

        
        if (info.bookmarkId < 0 && info.historyId < 0) {
            menu.findItem(R.id.home_remove).setVisible(false);
        }

        final boolean canOpenInReader = (info.display == Combined.DISPLAY_READER);
        menu.findItem(R.id.home_open_in_reader).setVisible(canOpenInReader);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        
        
        
        

        ContextMenuInfo menuInfo = item.getMenuInfo();
        if (menuInfo == null || !(menuInfo instanceof HomeContextMenuInfo)) {
            return false;
        }

        HomeContextMenuInfo info = (HomeContextMenuInfo) menuInfo;
        final Context context = getActivity().getApplicationContext();

        switch(item.getItemId()) {
            case R.id.home_share: {
                if (info.url == null) {
                    Log.e(LOGTAG, "Can't share because URL is null");
                    break;
                }

                GeckoAppShell.openUriExternal(info.url, SHARE_MIME_TYPE, "", "",
                                              Intent.ACTION_SEND, info.title);
                return true;
            }

            case R.id.home_add_to_launcher: {
                if (info.url == null) {
                    Log.e(LOGTAG, "Can't add to home screen because URL is null");
                    break;
                }

                
                Bitmap bitmap = null;
                if (info.favicon != null) {
                    bitmap = BitmapUtils.decodeByteArray(info.favicon);
                }

                String shortcutTitle = TextUtils.isEmpty(info.title) ? info.url.replaceAll(REGEX_URL_TO_TITLE, "") : info.title;
                GeckoAppShell.createShortcut(shortcutTitle, info.url, bitmap, "");
                return true;
            }

            case R.id.home_open_private_tab:
            case R.id.home_open_new_tab: {
                if (info.url == null) {
                    Log.e(LOGTAG, "Can't open in new tab because URL is null");
                    break;
                }

                int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_BACKGROUND;
                if (item.getItemId() == R.id.home_open_private_tab)
                    flags |= Tabs.LOADURL_PRIVATE;

                final String url = (info.inReadingList ? ReaderModeUtils.getAboutReaderForUrl(info.url, true) : info.url);
                Tabs.getInstance().loadUrl(url, flags);
                Toast.makeText(context, R.string.new_tab_opened, Toast.LENGTH_SHORT).show();
                return true;
            }

            case R.id.home_edit_bookmark: {
                new EditBookmarkDialog(context).show(info.url);
                return true;
            }

            case R.id.home_open_in_reader: {
                final String url = ReaderModeUtils.getAboutReaderForUrl(info.url, true);
                Tabs.getInstance().loadUrl(url, Tabs.LOADURL_NONE);
                return true;
            }

            case R.id.home_remove: {
                
                final int historyId = info.historyId;
                if (historyId > -1) {
                    new RemoveHistoryTask(context, historyId).execute();
                    return true;
                }

                final int bookmarkId = info.bookmarkId;
                if (bookmarkId > -1) {
                    new RemoveBookmarkTask(context, bookmarkId, info.url, info.inReadingList).execute();
                    return true;
                }
            }
        }
        return false;
    }

    private static class RemoveBookmarkTask extends UiAsyncTask<Void, Void, Void> {
        private final Context mContext;
        private final int mId;
        private final String mUrl;
        private final boolean mInReadingList;

        public RemoveBookmarkTask(Context context, int id, String url, boolean inReadingList) {
            super(ThreadUtils.getBackgroundHandler());

            mContext = context;
            mId = id;
            mUrl = url;
            mInReadingList = inReadingList;
        }

        @Override
        public Void doInBackground(Void... params) {
            ContentResolver cr = mContext.getContentResolver();
            BrowserDB.removeBookmark(cr, mId);
            if (mInReadingList) {
                GeckoEvent e = GeckoEvent.createBroadcastEvent("Reader:Remove", mUrl);
                GeckoAppShell.sendEventToGecko(e);

                int count = BrowserDB.getReadingListCount(cr);
                e = GeckoEvent.createBroadcastEvent("Reader:ListCountUpdated", Integer.toString(count));
                GeckoAppShell.sendEventToGecko(e);
            }
            return null;
        }

        @Override
        public void onPostExecute(Void result) {
            int messageId = mInReadingList ? R.string.reading_list_removed : R.string.bookmark_removed;
            Toast.makeText(mContext, messageId, Toast.LENGTH_SHORT).show();
        }
    }

    private static class RemoveHistoryTask extends UiAsyncTask<Void, Void, Void> {
        private final Context mContext;
        private final int mId;

        public RemoveHistoryTask(Context context, int id) {
            super(ThreadUtils.getBackgroundHandler());

            mContext = context;
            mId = id;
        }

        @Override
        public Void doInBackground(Void... params) {
            BrowserDB.removeHistoryEntry(mContext.getContentResolver(), mId);
            return null;
        }

        @Override
        public void onPostExecute(Void result) {
            Toast.makeText(mContext, R.string.history_removed, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void setUserVisibleHint (boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        loadIfVisible();
    }

    protected abstract void load();

    protected void loadIfVisible() {
        if (!isVisible() || !getUserVisibleHint()) {
            return;
        }

        load();
    }

    


    abstract class HomeCursorLoaderCallbacks implements LoaderCallbacks<Cursor> {

        
        public abstract void onFaviconsLoaded();

        public void loadFavicons(Cursor cursor) {
            FaviconsLoader.restartFromCursor(getLoaderManager(), LOADER_ID_FAVICONS, this, cursor);
        }

        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            if (id == LOADER_ID_FAVICONS) {
                return FaviconsLoader.createInstance(getActivity(), args);
            }

            return null;
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, Cursor c) {
            if (loader.getId() == LOADER_ID_FAVICONS) {
                onFaviconsLoaded();
            }
        }

        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            
        }
    }
}
