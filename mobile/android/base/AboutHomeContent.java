




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.Thumbnails;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.db.BrowserDB.PinnedSite;
import org.mozilla.gecko.db.BrowserDB.TopSitesCursorWrapper;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.GeckoAsyncTask;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.PathShape;
import android.graphics.Path;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.TextAppearanceSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AbsListView;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class AboutHomeContent extends ScrollView
                              implements TabsAccessor.OnQueryTabsCompleteListener,
                                         LightweightTheme.OnChangeListener {
    private static final String LOGTAG = "GeckoAboutHome";

    private static final int NUMBER_OF_REMOTE_TABS = 5;

    private static int mNumberOfTopSites;
    private static int mNumberOfCols;

    public static enum UnpinFlags {
        REMOVE_PIN,
        REMOVE_HISTORY
    }

    static enum UpdateFlags {
        TOP_SITES,
        PREVIOUS_TABS,
        RECOMMENDED_ADDONS,
        REMOTE_TABS;

        public static final EnumSet<UpdateFlags> ALL = EnumSet.allOf(UpdateFlags.class);
    }

    private Context mContext;
    private BrowserApp mActivity;
    UriLoadCallback mUriLoadCallback = null;
    VoidCallback mLoadCompleteCallback = null;
    private LayoutInflater mInflater;

    private ContentObserver mTabsContentObserver = null;

    protected TopSitesCursorAdapter mTopSitesAdapter;
    protected TopSitesGridView mTopSitesGrid;

    private AboutHomePromoBox mPromoBox;
    protected AboutHomeSection mAddons;
    protected AboutHomeSection mLastTabs;
    protected AboutHomeSection mRemoteTabs;

    private View.OnClickListener mRemoteTabClickListener;

    private static Rect sIconBounds;
    private static TextAppearanceSpan sSubTitleSpan;
    private static Drawable sPinDrawable = null;
    private int mThumbnailBackground;

    public interface UriLoadCallback {
        public void callback(String uriSpec);
    }

    public interface VoidCallback {
        public void callback();
    }

    public AboutHomeContent(Context context) {
        super(context);
        mContext = context;
        mActivity = (BrowserApp) context;
    }

    public AboutHomeContent(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mActivity = (BrowserApp) context;
    }

    public void init() {
        int iconSize = mContext.getResources().getDimensionPixelSize(R.dimen.abouthome_addon_icon_size);
        sIconBounds = new Rect(0, 0, iconSize, iconSize); 
        sSubTitleSpan = new TextAppearanceSpan(mContext, R.style.AboutHome_TextAppearance_SubTitle);
        mThumbnailBackground = mContext.getResources().getColor(R.color.abouthome_thumbnail_bg);

        inflate();

        
        
        
        
        mTabsContentObserver = new ContentObserver(GeckoAppShell.getHandler()) {
            public void onChange(boolean selfChange) {
                update(EnumSet.of(AboutHomeContent.UpdateFlags.REMOTE_TABS));
            }
        };
        mActivity.getContentResolver().registerContentObserver(BrowserContract.Tabs.CONTENT_URI,
                false, mTabsContentObserver);

        mRemoteTabClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int flags = Tabs.LOADURL_NEW_TAB;
                if (Tabs.getInstance().getSelectedTab().isPrivate())
                    flags |= Tabs.LOADURL_PRIVATE;
                Tabs.getInstance().loadUrl((String) v.getTag(), flags);
            }
        };
    }

    private void inflate() {
        mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mInflater.inflate(R.layout.abouthome_content, this);

        mTopSitesGrid = (TopSitesGridView)findViewById(R.id.top_sites_grid);
        mTopSitesGrid.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
                TopSitesViewHolder holder = (TopSitesViewHolder) v.getTag();
                String spec = holder.getUrl();

                
                if (TextUtils.isEmpty(spec)) {
                    editSite(spec, position);
                    return;
                }

                if (mUriLoadCallback != null)
                    mUriLoadCallback.callback(spec);
            }
        });

        mTopSitesGrid.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo)menuInfo;
                mTopSitesGrid.setSelectedPosition(info.position);

                MenuInflater inflater = mActivity.getMenuInflater();
                inflater.inflate(R.menu.abouthome_topsites_contextmenu, menu);

                
                
                
                View view = mTopSitesGrid.getChildAt(info.position);
                TopSitesViewHolder holder = (TopSitesViewHolder) view.getTag();
                if (TextUtils.isEmpty(holder.getUrl())) {
                    menu.findItem(R.id.abouthome_topsites_pin).setVisible(false);
                    menu.findItem(R.id.abouthome_topsites_unpin).setVisible(false);
                    menu.findItem(R.id.abouthome_topsites_remove).setVisible(false);
                } else if (holder.isPinned()) {
                    menu.findItem(R.id.abouthome_topsites_pin).setVisible(false);
                } else {
                    menu.findItem(R.id.abouthome_topsites_unpin).setVisible(false);
                }
            }
        });

        mPromoBox = (AboutHomePromoBox) findViewById(R.id.promo_box);
        mAddons = (AboutHomeSection) findViewById(R.id.recommended_addons);
        mLastTabs = (AboutHomeSection) findViewById(R.id.last_tabs);
        mRemoteTabs = (AboutHomeSection) findViewById(R.id.remote_tabs);

        mAddons.setOnMoreTextClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mUriLoadCallback != null)
                    mUriLoadCallback.callback("https://addons.mozilla.org/android");
            }
        });

        mRemoteTabs.setOnMoreTextClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mActivity.showRemoteTabs();
            }
        });

        setTopSitesConstants();
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        mActivity.getLightweightTheme().addListener(this);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mActivity.getLightweightTheme().removeListener(this);
    }

    public void onDestroy() {
        if (mTopSitesAdapter != null) {
            Cursor cursor = mTopSitesAdapter.getCursor();
            if (cursor != null && !cursor.isClosed())
                cursor.close();
        }

        if (mTabsContentObserver != null) {
            mActivity.getContentResolver().unregisterContentObserver(mTabsContentObserver);
            mTabsContentObserver = null;
        }
    }

    void setLastTabsVisibility(boolean visible) {
        if (visible)
            mLastTabs.show();
        else
            mLastTabs.hide();
    }

    private void setTopSitesVisibility(boolean hasTopSites) {
        int visibility = hasTopSites ? View.VISIBLE : View.GONE;

        findViewById(R.id.top_sites_title).setVisibility(visibility);
        findViewById(R.id.top_sites_grid).setVisibility(visibility);
    }

    private void updateLayout() {
        boolean hasTopSites = mTopSitesAdapter.getCount() > 0;
        setTopSitesVisibility(hasTopSites);
        mPromoBox.showRandomPromo();
    }

    private void updateLayoutForSync() {
        final GeckoApp.StartupMode startupMode = mActivity.getStartupMode();

        post(new Runnable() {
            public void run() {
                
                
                
                if (mTopSitesAdapter != null)
                    updateLayout();
            }
        });
    }

    private void loadTopSites() {
        final ContentResolver resolver = mActivity.getContentResolver();
        Cursor old = null;
        if (mTopSitesAdapter != null) {
            old = mTopSitesAdapter.getCursor();
        }
        
        final Cursor oldCursor = old;
        final Cursor newCursor = BrowserDB.getTopSites(resolver, mNumberOfTopSites);

        post(new Runnable() {
            public void run() {
                if (mTopSitesAdapter == null) {
                    mTopSitesAdapter = new TopSitesCursorAdapter(mActivity,
                                                                 R.layout.abouthome_topsite_item,
                                                                 newCursor,
                                                                 new String[] { URLColumns.TITLE },
                                                                 new int[] { R.id.title });

                    mTopSitesGrid.setAdapter(mTopSitesAdapter);
                } else {
                    mTopSitesAdapter.changeCursor(newCursor);
                }

                if (mTopSitesAdapter.getCount() > 0)
                    loadTopSitesThumbnails(resolver);

                updateLayout();

                
                if (oldCursor != null && !oldCursor.isClosed())
                    oldCursor.close();

                
                
                
                if (mLoadCompleteCallback != null)
                    mLoadCompleteCallback.callback();
            }
        });
    }

    private List<String> getTopSitesUrls() {
        List<String> urls = new ArrayList<String>();

        Cursor c = mTopSitesAdapter.getCursor();
        if (c == null || !c.moveToFirst())
            return urls;

        do {
            final String url = c.getString(c.getColumnIndexOrThrow(URLColumns.URL));
            urls.add(url);
        } while (c.moveToNext());

        return urls;
    }

    private void displayThumbnail(View view, Bitmap thumbnail) {
        ImageView thumbnailView = (ImageView) view.findViewById(R.id.thumbnail);

        if (thumbnail == null) {
            thumbnailView.setImageResource(R.drawable.abouthome_thumbnail_bg);
            thumbnailView.setBackgroundColor(mThumbnailBackground);
            thumbnailView.setScaleType(ImageView.ScaleType.FIT_CENTER);
        } else {
            try {
                thumbnailView.setImageBitmap(thumbnail);
                thumbnailView.setBackgroundColor(0x0);
                thumbnailView.setScaleType(ImageView.ScaleType.CENTER_CROP);
            } catch (OutOfMemoryError oom) {
                Log.e(LOGTAG, "Unable to load thumbnail bitmap", oom);
                thumbnailView.setImageResource(R.drawable.abouthome_thumbnail_bg);
                thumbnailView.setScaleType(ImageView.ScaleType.FIT_CENTER);
            }
        }
    }

    private void updateTopSitesThumbnails(Map<String, Bitmap> thumbnails) {
        for (int i = 0; i < mTopSitesAdapter.getCount(); i++) {
            final View view = mTopSitesGrid.getChildAt(i);

            
            
            if (view == null)
                continue;

            TopSitesViewHolder holder = (TopSitesViewHolder)view.getTag();
            final String url = holder.getUrl();
            if (TextUtils.isEmpty(url)) {
                holder.thumbnailView.setImageResource(R.drawable.abouthome_thumbnail_add);
                holder.thumbnailView.setScaleType(ImageView.ScaleType.FIT_CENTER);
            } else {
                displayThumbnail(view, thumbnails.get(url));
            }
        }

        mTopSitesGrid.invalidate();
    }

    public Map<String, Bitmap> getThumbnailsFromCursor(Cursor c) {
        Map<String, Bitmap> thumbnails = new HashMap<String, Bitmap>();

        try {
            if (c == null || !c.moveToFirst())
                return thumbnails;

            do {
                final String url = c.getString(c.getColumnIndexOrThrow(Thumbnails.URL));
                final byte[] b = c.getBlob(c.getColumnIndexOrThrow(Thumbnails.DATA));
                if (b == null)
                    continue;

                Bitmap thumbnail = BitmapFactory.decodeByteArray(b, 0, b.length);
                if (thumbnail == null)
                    continue;

                thumbnails.put(url, thumbnail);
            } while (c.moveToNext());
        } finally {
            if (c != null)
                c.close();
        }

        return thumbnails;
    }

    private void loadTopSitesThumbnails(final ContentResolver cr) {
        final List<String> urls = getTopSitesUrls();
        if (urls.size() == 0)
            return;

        (new GeckoAsyncTask<Void, Void, Cursor>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public Cursor doInBackground(Void... params) {
                return BrowserDB.getThumbnailsForUrls(cr, urls);
            }

            @Override
            public void onPostExecute(Cursor c) {
                updateTopSitesThumbnails(getThumbnailsFromCursor(c));
            }
        }).execute();
    }

    void update(final EnumSet<UpdateFlags> flags) {
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                if (flags.contains(UpdateFlags.TOP_SITES))
                    loadTopSites();

                if (flags.contains(UpdateFlags.PREVIOUS_TABS))
                    readLastTabs();

                if (flags.contains(UpdateFlags.RECOMMENDED_ADDONS))
                    readRecommendedAddons();

                if (flags.contains(UpdateFlags.REMOTE_TABS))
                    loadRemoteTabs();
            }
        });
    }

    public void setUriLoadCallback(UriLoadCallback uriLoadCallback) {
        mUriLoadCallback = uriLoadCallback;
    }

    public void setLoadCompleteCallback(VoidCallback callback) {
        mLoadCompleteCallback = callback;
    }

    public void onActivityContentChanged() {
        update(EnumSet.of(UpdateFlags.TOP_SITES));
    }

    private void setTopSitesConstants() {
        mNumberOfTopSites = getResources().getInteger(R.integer.number_of_top_sites);
        mNumberOfCols = getResources().getInteger(R.integer.number_of_top_sites_cols);
    }

    


    public void refresh() {
        if (mTopSitesAdapter != null)
            mTopSitesAdapter.notifyDataSetChanged();

        removeAllViews(); 
        inflate();
        mTopSitesGrid.setAdapter(mTopSitesAdapter); 
        update(AboutHomeContent.UpdateFlags.ALL); 
    }

    private String readFromZipFile(String filename) {
        ZipFile zip = null;
        String str = null;
        try {
            InputStream fileStream = null;
            File applicationPackage = new File(mActivity.getApplication().getPackageResourcePath());
            zip = new ZipFile(applicationPackage);
            if (zip == null)
                return null;
            ZipEntry fileEntry = zip.getEntry(filename);
            if (fileEntry == null)
                return null;
            fileStream = zip.getInputStream(fileEntry);
            str = readStringFromStream(fileStream);
        } catch (IOException ioe) {
            Log.e(LOGTAG, "error reading zip file: " + filename, ioe);
        } finally {
            try {
                if (zip != null)
                    zip.close();
            } catch (IOException ioe) {
                
                
                Log.e(LOGTAG, "error closing zip filestream", ioe);
            }
        }
        return str;
    }

    private String readStringFromStream(InputStream fileStream) {
        String str = null;
        try {
            byte[] buf = new byte[32768];
            StringBuffer jsonString = new StringBuffer();
            int read = 0;
            while ((read = fileStream.read(buf, 0, 32768)) != -1)
                jsonString.append(new String(buf, 0, read));
            str = jsonString.toString();
        } catch (IOException ioe) {
            Log.i(LOGTAG, "error reading filestream", ioe);
        } finally {
            try {
                if (fileStream != null)
                    fileStream.close();
            } catch (IOException ioe) {
                
                
                Log.e(LOGTAG, "error closing filestream", ioe);
            }
        }
        return str;
    }

    private String getPageUrlFromIconUrl(String iconUrl) {
        
        
        
        
        String pageUrl = iconUrl;

        try {
            URL urlForIcon = new URL(iconUrl);
            URL urlForPage = new URL(urlForIcon.getProtocol(), urlForIcon.getAuthority(), urlForIcon.getPath());
            pageUrl = urlForPage.toString();
        } catch (MalformedURLException e) {
            
        }

        return pageUrl;
    }

    private void readRecommendedAddons() {
        final String addonsFilename = "recommended-addons.json";
        String jsonString;
        try {
            jsonString = mActivity.getProfile().readFile(addonsFilename);
        } catch (IOException ioe) {
            Log.i(LOGTAG, "filestream is null");
            jsonString = readFromZipFile(addonsFilename);
        }

        JSONArray addonsArray = null;
        if (jsonString != null) {
            try {
                addonsArray = new JSONObject(jsonString).getJSONArray("addons");
            } catch (JSONException e) {
                Log.i(LOGTAG, "error reading json file", e);
            }
        }

        final JSONArray array = addonsArray;
        post(new Runnable() {
            public void run() {
                try {
                    if (array == null || array.length() == 0) {
                        mAddons.hide();
                        return;
                    }

                    for (int i = 0; i < array.length(); i++) {
                        JSONObject jsonobj = array.getJSONObject(i);
                        String name = jsonobj.getString("name");
                        String version = jsonobj.getString("version");
                        String text = name + " " + version;

                        SpannableString spannable = new SpannableString(text);
                        spannable.setSpan(sSubTitleSpan, name.length() + 1, text.length(), 0);

                        final TextView row = (TextView) mInflater.inflate(R.layout.abouthome_addon_row, mAddons.getItemsContainer(), false);
                        row.setText(spannable, TextView.BufferType.SPANNABLE);

                        Drawable drawable = mContext.getResources().getDrawable(R.drawable.ic_addons_empty);
                        drawable.setBounds(sIconBounds);
                        row.setCompoundDrawables(drawable, null, null, null);

                        String iconUrl = jsonobj.getString("iconURL");
                        String pageUrl = getPageUrlFromIconUrl(iconUrl);

                        final String homepageUrl = jsonobj.getString("homepageURL");
                        row.setOnClickListener(new View.OnClickListener() {
                            public void onClick(View v) {
                                if (mUriLoadCallback != null)
                                    mUriLoadCallback.callback(homepageUrl);
                            }
                        });

                        Favicons favicons = Favicons.getInstance();
                        favicons.loadFavicon(pageUrl, iconUrl, true,
                                    new Favicons.OnFaviconLoadedListener() {
                            public void onFaviconLoaded(String url, Bitmap favicon) {
                                if (favicon != null) {
                                    Drawable drawable = new BitmapDrawable(favicon);
                                    drawable.setBounds(sIconBounds);
                                    row.setCompoundDrawables(drawable, null, null, null);
                                }
                            }
                        });

                        mAddons.addItem(row);
                    }

                    mAddons.show();
                } catch (JSONException e) {
                    Log.i(LOGTAG, "error reading json file", e);
                }
            }
        });
    }

    private void readLastTabs() {
        String jsonString = mActivity.getProfile().readSessionFile(true);
        if (jsonString == null) {
            
            return;
        }

        final ArrayList<String> lastTabUrlsList = new ArrayList<String>();
        new SessionParser() {
            @Override
            public void onTabRead(final SessionTab tab) {
                final String url = tab.getSelectedUrl();
                
                if (url.equals("about:home")) {
                    return;
                }

                ContentResolver resolver = mActivity.getContentResolver();
                final Bitmap favicon = BrowserDB.getFaviconForUrl(resolver, url);
                lastTabUrlsList.add(url);

                AboutHomeContent.this.post(new Runnable() {
                    public void run() {
                        View container = mInflater.inflate(R.layout.abouthome_last_tabs_row, mLastTabs.getItemsContainer(), false);
                        ((TextView) container.findViewById(R.id.last_tab_title)).setText(tab.getSelectedTitle());
                        ((TextView) container.findViewById(R.id.last_tab_url)).setText(tab.getSelectedUrl());
                        if (favicon != null) {
                            ((ImageView) container.findViewById(R.id.last_tab_favicon)).setImageBitmap(favicon);
                        }

                        container.setOnClickListener(new View.OnClickListener() {
                            public void onClick(View v) {
                                int flags = Tabs.LOADURL_NEW_TAB;
                                if (Tabs.getInstance().getSelectedTab().isPrivate())
                                    flags |= Tabs.LOADURL_PRIVATE;
                                Tabs.getInstance().loadUrl(url, flags);
                            }
                        });

                        mLastTabs.addItem(container);
                    }
                });
            }
        }.parse(jsonString);

        final int numLastTabs = lastTabUrlsList.size();
        if (numLastTabs >= 1) {
            post(new Runnable() {
                public void run() {
                    if (numLastTabs > 1) {
                        mLastTabs.showMoreText();
                        mLastTabs.setOnMoreTextClickListener(new View.OnClickListener() {
                            public void onClick(View v) {
                                int flags = Tabs.LOADURL_NEW_TAB;
                                if (Tabs.getInstance().getSelectedTab().isPrivate())
                                    flags |= Tabs.LOADURL_PRIVATE;
                                for (String url : lastTabUrlsList) {
                                    Tabs.getInstance().loadUrl(url, flags);
                                }
                            }
                        });
                    } else if (numLastTabs == 1) {
                        mLastTabs.hideMoreText();
                    }
                    mLastTabs.show();
                }
            });
        }
    }

    private void loadRemoteTabs() {
        if (!SyncAccounts.syncAccountsExist(mActivity)) {
            post(new Runnable() {
                public void run() {
                    mRemoteTabs.hide();
                }
            });
            return;
        }

        TabsAccessor.getTabs(getContext(), NUMBER_OF_REMOTE_TABS, this);
    }

    @Override
    public void onQueryTabsComplete(List<TabsAccessor.RemoteTab> tabsList) {
        ArrayList<TabsAccessor.RemoteTab> tabs = new ArrayList<TabsAccessor.RemoteTab> (tabsList);
        if (tabs == null || tabs.size() == 0) {
            mRemoteTabs.hide();
            return;
        }
        
        mRemoteTabs.clear();
        
        String client = null;
        
        for (TabsAccessor.RemoteTab tab : tabs) {
            if (client == null)
                client = tab.name;
            else if (!TextUtils.equals(client, tab.name))
                break;

            final TextView row = (TextView) mInflater.inflate(R.layout.abouthome_remote_tab_row, mRemoteTabs.getItemsContainer(), false);
            row.setText(TextUtils.isEmpty(tab.title) ? tab.url : tab.title);
            row.setTag(tab.url);
            mRemoteTabs.addItem(row);
            row.setOnClickListener(mRemoteTabClickListener);
        }
        
        mRemoteTabs.setSubtitle(client);
        mRemoteTabs.show();
    }

    @Override
    public void onLightweightThemeChanged() {
        LightweightThemeDrawable drawable = mActivity.getLightweightTheme().getColorDrawable(this);
        if (drawable == null)
            return;

         drawable.setAlpha(255, 0);
         setBackgroundDrawable(drawable);

         boolean isLight = mActivity.getLightweightTheme().isLightTheme();

         if (mAddons != null) {
             mAddons.setTheme(isLight);
             mLastTabs.setTheme(isLight);
             mRemoteTabs.setTheme(isLight);
             ((GeckoImageView) findViewById(R.id.abouthome_logo)).setTheme(isLight);
             ((GeckoTextView) findViewById(R.id.top_sites_title)).setTheme(isLight);
         }
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundColor(getContext().getResources().getColor(R.color.background_normal));

        if (mAddons != null) {
            mAddons.resetTheme();
            mLastTabs.resetTheme();
            mRemoteTabs.resetTheme();
            ((GeckoImageView) findViewById(R.id.abouthome_logo)).resetTheme();
            ((GeckoTextView) findViewById(R.id.top_sites_title)).resetTheme();
        }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        onLightweightThemeChanged();
    }

    public static class TopSitesGridView extends GridView {
        int mSelected = -1;

        public TopSitesGridView(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public int getColumnWidth() {
            return getColumnWidth(getWidth());
        }

        public int getColumnWidth(int width) {
            
            return (width - getPaddingLeft() - getPaddingRight()) / mNumberOfCols;
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int measuredWidth = View.MeasureSpec.getSize(widthMeasureSpec);
            int numRows;

            SimpleCursorAdapter adapter = (SimpleCursorAdapter) getAdapter();
            int nSites = Integer.MAX_VALUE;

            if (adapter != null) {
                Cursor c = adapter.getCursor();
                if (c != null)
                    nSites = c.getCount();
            }

            nSites = Math.min(nSites, mNumberOfTopSites);
            numRows = (int) Math.round((double) nSites / mNumberOfCols);
            setNumColumns(mNumberOfCols);

            
            
            int w = getColumnWidth(measuredWidth);
            ThumbnailHelper.getInstance().setThumbnailWidth(w);
            heightMeasureSpec = MeasureSpec.makeMeasureSpec((int)(w*ThumbnailHelper.THUMBNAIL_ASPECT_RATIO*numRows) + getPaddingTop() + getPaddingBottom(),
                                                                 MeasureSpec.EXACTLY);
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }

        public void setSelectedPosition(int position) {
            mSelected = position;
        }

        public int getSelectedPosition() {
            return mSelected;
        }
    }

    private class TopSitesViewHolder {
        public TextView titleView = null;
        public ImageView thumbnailView = null;
        public ImageView pinnedView = null;
        private String mTitle = null;
        private String mUrl = null;
        private boolean mIsPinned = false;

        public TopSitesViewHolder(View v) {
            titleView = (TextView) v.findViewById(R.id.title);
            thumbnailView = (ImageView) v.findViewById(R.id.thumbnail);
            pinnedView = (ImageView) v.findViewById(R.id.pinned);
        }

        public void setTitle(String title) {
            if (mTitle != null && mTitle.equals(title))
                return;
            mTitle = title;
            updateTitleView();
        }

        public String getTitle() {
            return (!TextUtils.isEmpty(mTitle) ? mTitle : mUrl);
        }

        public void setUrl(String url) {
            if (mUrl != null && mUrl.equals(url))
                return;
            mUrl = url;
            updateTitleView();
        }

        public String getUrl() {
            return mUrl;
        }

        public void updateTitleView() {
            String title = getTitle();
            if (!TextUtils.isEmpty(title)) {
                titleView.setText(title);
                titleView.setVisibility(View.VISIBLE);
            } else {
                titleView.setVisibility(View.INVISIBLE);
            }
        }

        private Drawable getPinDrawable() {
            if (sPinDrawable == null) {
                int size = mContext.getResources().getDimensionPixelSize(R.dimen.abouthome_topsite_pinsize);

                
                Path path = new Path();
                path.moveTo(0, 0);
                path.lineTo(size, 0);
                path.lineTo(size, size);
                path.close();

                sPinDrawable = new ShapeDrawable(new PathShape(path, size, size));
                Paint p = ((ShapeDrawable) sPinDrawable).getPaint();
                p.setColor(mContext.getResources().getColor(R.color.abouthome_topsite_pin));
            }
            return sPinDrawable;
        }

        public void setPinned(boolean aPinned) {
            mIsPinned = aPinned;
            pinnedView.setBackgroundDrawable(aPinned ? getPinDrawable() : null);
        }

        public boolean isPinned() {
            return mIsPinned;
        }
    }

    public class TopSitesCursorAdapter extends SimpleCursorAdapter {
        public TopSitesCursorAdapter(Context context, int layout, Cursor c,
                                     String[] from, int[] to) {
            super(context, layout, c, from, to);
        }

        @Override
        public int getCount() {
            return Math.min(super.getCount(), mNumberOfTopSites);
        }

        @Override
        protected void onContentChanged () {
            
            
            return;
        }

        private View buildView(String url, String title, boolean pinned, View convertView) {
            TopSitesViewHolder viewHolder;
            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.abouthome_topsite_item, null);

                viewHolder = new TopSitesViewHolder(convertView);
                convertView.setTag(viewHolder);
            } else {
                viewHolder = (TopSitesViewHolder) convertView.getTag();
            }

            viewHolder.setTitle(title);
            viewHolder.setUrl(url);
            viewHolder.setPinned(pinned);

            
            convertView.setLayoutParams(new AbsListView.LayoutParams(mTopSitesGrid.getColumnWidth(),
                        Math.round(mTopSitesGrid.getColumnWidth()*ThumbnailHelper.THUMBNAIL_ASPECT_RATIO)));

            return convertView;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            String url = "";
            String title = "";
            boolean pinned = false;

            Cursor c = getCursor();
            c.moveToPosition(position);
            if (!c.isAfterLast()) {
                url = c.getString(c.getColumnIndex(URLColumns.URL));
                title = c.getString(c.getColumnIndex(URLColumns.TITLE));
                pinned = ((TopSitesCursorWrapper)c).isPinned();
            }

            return buildView(url, title, pinned, convertView);
        }
    }

    private void clearThumbnailsWithUrl(final String url) {
        for (int i = 0; i < mTopSitesAdapter.getCount(); i++) {
            final View view = mTopSitesGrid.getChildAt(i);
            final TopSitesViewHolder holder = (TopSitesViewHolder) view.getTag();

            if (holder.getUrl().equals(url)) {
                clearThumbnail(holder);
            }
        }
    }

    private void clearThumbnail(TopSitesViewHolder holder) {
        holder.setTitle("");
        holder.setUrl("");
        holder.thumbnailView.setImageResource(R.drawable.abouthome_thumbnail_add);
        holder.thumbnailView.setBackgroundColor(mThumbnailBackground);
        holder.thumbnailView.setScaleType(ImageView.ScaleType.FIT_CENTER);
        holder.setPinned(false);
    }

    public void unpinSite(final UnpinFlags flags) {
        final int position = mTopSitesGrid.getSelectedPosition();
        final View v = mTopSitesGrid.getChildAt(position);
        final TopSitesViewHolder holder = (TopSitesViewHolder) v.getTag();
        final String url = holder.getUrl();
        
        clearThumbnail(holder);
        (new GeckoAsyncTask<Void, Void, Void>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public Void doInBackground(Void... params) {
                final ContentResolver resolver = mActivity.getContentResolver();
                BrowserDB.unpinSite(resolver, position);
                if (flags == UnpinFlags.REMOVE_HISTORY) {
                    BrowserDB.removeHistoryEntry(resolver, url);
                }
                return null;
            }
        }).execute();
    }

    public void pinSite() {
        final int position = mTopSitesGrid.getSelectedPosition();
        View v = mTopSitesGrid.getChildAt(position);

        final TopSitesViewHolder holder = (TopSitesViewHolder) v.getTag();
        holder.setPinned(true);

        
        (new GeckoAsyncTask<Void, Void, Void>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public Void doInBackground(Void... params) {
                final ContentResolver resolver = mActivity.getContentResolver();
                BrowserDB.pinSite(resolver, holder.getUrl(), holder.getTitle(), position);
                return null;
            }
        }).execute();
    }

    public void editSite() {
        int position = mTopSitesGrid.getSelectedPosition();
        View v = mTopSitesGrid.getChildAt(position);

        TopSitesViewHolder holder = (TopSitesViewHolder) v.getTag();
        editSite(holder.getUrl(), position);
    }

    
    public void editSite(String url, final int position) {
        Intent intent = new Intent(mContext, AwesomeBar.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
        intent.putExtra(AwesomeBar.TARGET_KEY, AwesomeBar.Target.PICK_SITE.toString());
        if (url != null && !TextUtils.isEmpty(url)) {
            intent.putExtra(AwesomeBar.CURRENT_URL_KEY, url);
        }

        int requestCode = GeckoAppShell.sActivityHelper.makeRequestCode(new ActivityResultHandler() {
            public void onActivityResult(int resultCode, Intent data) {
                if (resultCode == Activity.RESULT_CANCELED || data == null)
                    return;

                final View v = mTopSitesGrid.getChildAt(position);
                final TopSitesViewHolder holder = (TopSitesViewHolder) v.getTag();

                final String title = data.getStringExtra(AwesomeBar.TITLE_KEY);
                final String url = data.getStringExtra(AwesomeBar.URL_KEY);
                clearThumbnailsWithUrl(url);

                holder.setUrl(url);
                holder.setTitle(title);
                holder.setPinned(true);

                
                (new GeckoAsyncTask<Void, Void, Bitmap>(GeckoApp.mAppContext, GeckoAppShell.getHandler()) {
                    @Override
                    public Bitmap doInBackground(Void... params) {
                        final ContentResolver resolver = mActivity.getContentResolver();
                        BrowserDB.pinSite(resolver, holder.getUrl(), holder.getTitle(), position);

                        List<String> urls = new ArrayList<String>();
                        urls.add(holder.getUrl());

                        Cursor c = BrowserDB.getThumbnailsForUrls(resolver, urls);
                        if (c == null || !c.moveToFirst()) {
                            return null;
                        }

                        final byte[] b = c.getBlob(c.getColumnIndexOrThrow(Thumbnails.DATA));
                        if (b != null) {
                            return BitmapFactory.decodeByteArray(b, 0, b.length);
                        }

                        return null;
                    }

                    @Override
                    public void onPostExecute(Bitmap b) {
                        displayThumbnail(v, b);
                    }
                }).execute();
            }
        });

        mActivity.startActivityForResult(intent, requestCode);
    }
}
