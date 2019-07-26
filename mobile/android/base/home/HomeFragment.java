




package org.mozilla.gecko.home;

import org.mozilla.gecko.EditBookmarkDialog;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.home.TopSitesGridView.TopSitesGridContextMenuInfo;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;
import org.mozilla.gecko.widget.ButtonToast;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;





abstract class HomeFragment extends Fragment {
    
    private static final String LOGTAG="GeckoHomeFragment";

    
    protected static final String SHARE_MIME_TYPE = "text/plain";

    
    static final boolean DEFAULT_CAN_LOAD_HINT = false;

    
    
    
    private boolean mCanLoadHint;

    
    private boolean mIsLoaded;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Bundle args = getArguments();
        if (args != null) {
            mCanLoadHint = args.getBoolean(HomePager.CAN_LOAD_ARG, DEFAULT_CAN_LOAD_HINT);
        } else {
            mCanLoadHint = DEFAULT_CAN_LOAD_HINT;
        }

        mIsLoaded = false;
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

        menu.setHeaderTitle(info.getDisplayTitle());

        
        menu.findItem(R.id.top_sites_edit).setVisible(false);
        menu.findItem(R.id.top_sites_pin).setVisible(false);
        menu.findItem(R.id.top_sites_unpin).setVisible(false);

        
        
        if (!info.hasBookmarkId() || info.isInReadingList()) {
            menu.findItem(R.id.home_edit_bookmark).setVisible(false);
        }

        
        if (!info.canRemove()) {
            menu.findItem(R.id.home_remove).setVisible(false);
        }

        menu.findItem(R.id.home_share).setVisible(!GeckoProfile.get(getActivity()).inGuestMode());

        final boolean canOpenInReader = (info.display == Combined.DISPLAY_READER);
        menu.findItem(R.id.home_open_in_reader).setVisible(canOpenInReader);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        
        
        
        

        ContextMenuInfo menuInfo = item.getMenuInfo();
        if (menuInfo == null || !(menuInfo instanceof HomeContextMenuInfo)) {
            return false;
        }

        final HomeContextMenuInfo info = (HomeContextMenuInfo) menuInfo;
        final Context context = getActivity();

        final int itemId = item.getItemId();

        
        
        Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, TelemetryContract.Method.CONTEXT_MENU, getResources().getResourceEntryName(itemId));

        if (itemId == R.id.home_copyurl) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't copy address because URL is null");
                return false;
            }

            Clipboard.setText(info.url);
            return true;
        }

        if (itemId == R.id.home_share) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't share because URL is null");
                return false;
            } else {
                GeckoAppShell.openUriExternal(info.url, SHARE_MIME_TYPE, "", "",
                                              Intent.ACTION_SEND, info.getDisplayTitle());

                
                Telemetry.sendUIEvent(TelemetryContract.Event.SHARE, TelemetryContract.Method.LIST);
                return true;
            }
        }

        if (itemId == R.id.home_add_to_launcher) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't add to home screen because URL is null");
                return false;
            }

            
            Favicons.getPreferredSizeFaviconForPage(info.url, new GeckoAppShell.CreateShortcutFaviconLoadedListener(info.url, info.getDisplayTitle()));
            return true;
        }

        if (itemId == R.id.home_open_private_tab || itemId == R.id.home_open_new_tab) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't open in new tab because URL is null");
                return false;
            }

            int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_BACKGROUND;
            final boolean isPrivate = (item.getItemId() == R.id.home_open_private_tab);
            if (isPrivate) {
                flags |= Tabs.LOADURL_PRIVATE;
            }

            Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.CONTEXT_MENU);

            final String url = (info.isInReadingList() ? ReaderModeUtils.getAboutReaderForUrl(info.url) : info.url);

            
            
            final Tab newTab = Tabs.getInstance().loadUrl(decodeUserEnteredUrl(url), flags);
            final int newTabId = newTab.getId(); 

            final String message = isPrivate ?
                    getResources().getString(R.string.new_private_tab_opened) :
                    getResources().getString(R.string.new_tab_opened);
            final GeckoApp geckoApp = (GeckoApp) context;
            geckoApp.getButtonToast().show(false,
                    message,
                    null,
                    R.drawable.select_opened_tab,
                    new ButtonToast.ToastListener() {
                        @Override
                        public void onButtonClicked() {
                            Tabs.getInstance().selectTab(newTabId);
                        }

                        @Override
                        public void onToastHidden(ButtonToast.ReasonHidden reason) { }
                    });
            return true;
        }

        if (itemId == R.id.home_edit_bookmark) {
            
            new EditBookmarkDialog(context).show(info.url);
            return true;
        }

        if (itemId == R.id.home_open_in_reader) {
            final String url = ReaderModeUtils.getAboutReaderForUrl(info.url);
            Tabs.getInstance().loadUrl(url, Tabs.LOADURL_NONE);
            return true;
        }

        if (itemId == R.id.home_remove) {
            if (info instanceof TopSitesGridContextMenuInfo) {
                (new RemoveItemByUrlTask(context, info.url, info.position)).execute();
                return true;
            }

            if (info.isInReadingList() || info.hasBookmarkId() || info.hasHistoryId()) {
                (new RemoveItemByUrlTask(context, info.url)).execute();
                return true;
            }
        }

        return false;
    }

    @Override
    public void setUserVisibleHint (boolean isVisibleToUser) {
        if (isVisibleToUser == getUserVisibleHint()) {
            return;
        }

        super.setUserVisibleHint(isVisibleToUser);
        loadIfVisible();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    void setCanLoadHint(boolean canLoadHint) {
        if (mCanLoadHint == canLoadHint) {
            return;
        }

        mCanLoadHint = canLoadHint;
        loadIfVisible();
    }

    boolean getCanLoadHint() {
        return mCanLoadHint;
    }

    








    public static String decodeUserEnteredUrl(String url) {
        Uri uri = Uri.parse(url);
        if ("user-entered".equals(uri.getScheme())) {
            return uri.getSchemeSpecificPart();
        }
        return url;
    }

    protected abstract void load();

    protected boolean canLoad() {
        return (mCanLoadHint && isVisible() && getUserVisibleHint());
    }

    protected void loadIfVisible() {
        if (!canLoad() || mIsLoaded) {
            return;
        }

        load();
        mIsLoaded = true;
    }

    private static class RemoveItemByUrlTask extends UiAsyncTask<Void, Void, Void> {
        private final Context mContext;
        private final String mUrl;
        private final int mPosition;

        


        public RemoveItemByUrlTask(Context context, String url) {
            this(context, url, -1);
        }

        



        public RemoveItemByUrlTask(Context context, String url, int position) {
            super(ThreadUtils.getBackgroundHandler());

            mContext = context;
            mUrl = url;
            mPosition = position;
        }

        @Override
        public Void doInBackground(Void... params) {
            ContentResolver cr = mContext.getContentResolver();

            if (mPosition > -1) {
                BrowserDB.unpinSite(cr, mPosition);
            }

            BrowserDB.removeBookmarksWithURL(cr, mUrl);
            BrowserDB.removeHistoryEntry(cr, mUrl);

            BrowserDB.removeReadingListItemWithURL(cr, mUrl);
            GeckoEvent e = GeckoEvent.createBroadcastEvent("Reader:Remove", mUrl);
            GeckoAppShell.sendEventToGecko(e);

            return null;
        }

        @Override
        public void onPostExecute(Void result) {
            Toast.makeText(mContext, R.string.page_removed, Toast.LENGTH_SHORT).show();
        }
    }
}
