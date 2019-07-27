




package org.mozilla.gecko.home;

import java.util.EnumSet;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.EditBookmarkDialog;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserContract.SuggestedSites;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.home.HomeContextMenuInfo.RemoveItemType;
import org.mozilla.gecko.home.HomePager.OnUrlOpenInBackgroundListener;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.TopSitesGridView.TopSitesGridContextMenuInfo;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;







public abstract class HomeFragment extends Fragment {
    
    private static final String LOGTAG="GeckoHomeFragment";

    
    protected static final String SHARE_MIME_TYPE = "text/plain";

    
    static final boolean DEFAULT_CAN_LOAD_HINT = false;

    
    
    
    private boolean mCanLoadHint;

    
    private boolean mIsLoaded;

    
    protected OnUrlOpenListener mUrlOpenListener;

    
    private OnUrlOpenInBackgroundListener mUrlOpenInBackgroundListener;

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);

        try {
            mUrlOpenListener = (OnUrlOpenListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenListener");
        }

        try {
            mUrlOpenInBackgroundListener = (OnUrlOpenInBackgroundListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement HomePager.OnUrlOpenInBackgroundListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mUrlOpenListener = null;
        mUrlOpenInBackgroundListener = null;
    }

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
        if (!(menuInfo instanceof HomeContextMenuInfo)) {
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

        if (!StringUtils.isShareableUrl(info.url) || GeckoProfile.get(getActivity()).inGuestMode()) {
            menu.findItem(R.id.home_share).setVisible(false);
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        
        
        
        

        ContextMenuInfo menuInfo = item.getMenuInfo();
        if (!(menuInfo instanceof HomeContextMenuInfo)) {
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

            
            Favicons.getPreferredSizeFaviconForPage(context, info.url, new GeckoAppShell.CreateShortcutFaviconLoadedListener(info.url, info.getDisplayTitle()));
            return true;
        }

        if (itemId == R.id.home_open_private_tab || itemId == R.id.home_open_new_tab) {
            if (info.url == null) {
                Log.e(LOGTAG, "Can't open in new tab because URL is null");
                return false;
            }

            
            
            
            
            final String url = StringUtils.decodeUserEnteredUrl(info.isInReadingList()
                    ? ReaderModeUtils.getAboutReaderForUrl(info.url)
                    : info.url);

            final EnumSet<OnUrlOpenInBackgroundListener.Flags> flags = EnumSet.noneOf(OnUrlOpenInBackgroundListener.Flags.class);
            if (item.getItemId() == R.id.home_open_private_tab) {
                flags.add(OnUrlOpenInBackgroundListener.Flags.PRIVATE);
            }

            mUrlOpenInBackgroundListener.onUrlOpenInBackground(url, flags);

            Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, TelemetryContract.Method.CONTEXT_MENU);

            return true;
        }

        if (itemId == R.id.home_edit_bookmark) {
            
            new EditBookmarkDialog(context).show(info.url);
            return true;
        }

        if (itemId == R.id.home_remove) {
            
            final int position = info instanceof TopSitesGridContextMenuInfo ? info.position : -1;

            (new RemoveItemByUrlTask(context, info.url, info.itemType, position)).execute();
            return true;
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

        
        
        
        
        
        
        
        
        if (isVisible()) {
            getFragmentManager().beginTransaction()
                                .detach(this)
                                .attach(this)
                                .commitAllowingStateLoss();
        }
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

    protected static class RemoveItemByUrlTask extends UIAsyncTask.WithoutParams<Void> {
        private final Context mContext;
        private final String mUrl;
        private final RemoveItemType mType;
        private final int mPosition;
        private final BrowserDB mDB;

        



        public RemoveItemByUrlTask(Context context, String url, RemoveItemType type, int position) {
            super(ThreadUtils.getBackgroundHandler());

            mContext = context;
            mUrl = url;
            mType = type;
            mPosition = position;
            mDB = GeckoProfile.get(context).getDB();
        }

        @Override
        public Void doInBackground() {
            ContentResolver cr = mContext.getContentResolver();

            if (mPosition > -1) {
                mDB.unpinSite(cr, mPosition);
                if (mDB.hideSuggestedSite(mUrl)) {
                    cr.notifyChange(SuggestedSites.CONTENT_URI, null);
                }
            }

            switch(mType) {
                case BOOKMARKS:
                    mDB.removeBookmarksWithURL(cr, mUrl);
                    break;

                case HISTORY:
                    mDB.removeHistoryEntry(cr, mUrl);
                    break;

                case READING_LIST:
                    mDB.getReadingListAccessor().removeReadingListItemWithURL(cr, mUrl);
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:Removed", mUrl));
                    break;

                default:
                    Log.e(LOGTAG, "Can't remove item type " + mType.toString());
                    break;
            }
            return null;
        }

        @Override
        public void onPostExecute(Void result) {
            Toast.makeText(mContext, R.string.page_removed, Toast.LENGTH_SHORT).show();
        }
    }
}
