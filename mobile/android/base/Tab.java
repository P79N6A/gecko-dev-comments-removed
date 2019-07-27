




package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.URLMetadata;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.LoadFaviconTask;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.favicons.RemoteFavicon;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.toolbar.BrowserToolbar.TabEditingState;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

public class Tab {
    private static final String LOGTAG = "GeckoTab";

    private static Pattern sColorPattern;
    private final int mId;
    private final BrowserDB mDB;
    private long mLastUsed;
    private String mUrl;
    private String mBaseDomain;
    private String mUserRequested; 
    private String mTitle;
    private Bitmap mFavicon;
    private String mFaviconUrl;

    
    final TreeSet<RemoteFavicon> mAvailableFavicons = new TreeSet<>();
    private boolean mHasFeeds;
    private boolean mHasOpenSearch;
    private final SiteIdentity mSiteIdentity;
    private BitmapDrawable mThumbnail;
    private final int mParentId;
    private final boolean mExternal;
    private boolean mBookmark;
    private boolean mIsInReadingList;
    private int mFaviconLoadId;
    private String mContentType;
    private boolean mHasTouchListeners;
    private ZoomConstraints mZoomConstraints;
    private boolean mIsRTL;
    private final ArrayList<View> mPluginViews;
    private final HashMap<Object, Layer> mPluginLayers;
    private int mBackgroundColor;
    private int mState;
    private Bitmap mThumbnailBitmap;
    private boolean mDesktopMode;
    private boolean mEnteringReaderMode;
    private final Context mAppContext;
    private ErrorType mErrorType = ErrorType.NONE;
    private volatile int mLoadProgress;
    private volatile int mRecordingCount;
    private String mMostRecentHomePanel;

    private int mHistoryIndex;
    private int mHistorySize;
    private boolean mCanDoBack;
    private boolean mCanDoForward;

    private boolean mIsEditing;
    private final TabEditingState mEditingState = new TabEditingState();

    public static final int STATE_DELAYED = 0;
    public static final int STATE_LOADING = 1;
    public static final int STATE_SUCCESS = 2;
    public static final int STATE_ERROR = 3;

    public static final int LOAD_PROGRESS_INIT = 10;
    public static final int LOAD_PROGRESS_START = 20;
    public static final int LOAD_PROGRESS_LOCATION_CHANGE = 60;
    public static final int LOAD_PROGRESS_LOADED = 80;
    public static final int LOAD_PROGRESS_STOP = 100;

    private static final int DEFAULT_BACKGROUND_COLOR = Color.WHITE;

    public enum ErrorType {
        CERT_ERROR,  
        BLOCKED,     
        NET_ERROR,   
        NONE         
    }

    public Tab(Context context, int id, String url, boolean external, int parentId, String title) {
        mAppContext = context.getApplicationContext();
        mDB = GeckoProfile.get(context).getDB();
        mId = id;
        mUrl = url;
        mBaseDomain = "";
        mUserRequested = "";
        mExternal = external;
        mParentId = parentId;
        mTitle = title == null ? "" : title;
        mSiteIdentity = new SiteIdentity();
        mHistoryIndex = -1;
        mContentType = "";
        mZoomConstraints = new ZoomConstraints(false);
        mPluginViews = new ArrayList<View>();
        mPluginLayers = new HashMap<Object, Layer>();
        mState = shouldShowProgress(url) ? STATE_LOADING : STATE_SUCCESS;
        mLoadProgress = LOAD_PROGRESS_INIT;

        
        
        
        mBackgroundColor = DEFAULT_BACKGROUND_COLOR;

        updateBookmark();
        updateReadingList();
    }

    private ContentResolver getContentResolver() {
        return mAppContext.getContentResolver();
    }

    public void onDestroy() {
        Tabs.getInstance().notifyListeners(this, Tabs.TabEvents.CLOSED);
    }

    public int getId() {
        return mId;
    }

    public synchronized void onChange() {
        mLastUsed = System.currentTimeMillis();
    }

    public synchronized long getLastUsed() {
        return mLastUsed;
    }

    public int getParentId() {
        return mParentId;
    }

    
    public synchronized String getURL() {
        return mUrl;
    }

    
    public synchronized String getUserRequested() {
        return mUserRequested;
    }

    
    public synchronized String getTitle() {
        return mTitle;
    }

    public String getDisplayTitle() {
        if (mTitle != null && mTitle.length() > 0) {
            return mTitle;
        }

        return mUrl;
    }

    



    public String getBaseDomain() {
        return mBaseDomain;
    }

    public Bitmap getFavicon() {
        return mFavicon;
    }

    public BitmapDrawable getThumbnail() {
        return mThumbnail;
    }

    public String getMostRecentHomePanel() {
        return mMostRecentHomePanel;
    }

    public void setMostRecentHomePanel(String panelId) {
        mMostRecentHomePanel = panelId;
    }

    public Bitmap getThumbnailBitmap(int width, int height) {
        if (mThumbnailBitmap != null) {
            
            
            boolean honeycomb = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB
                              && Build.VERSION.SDK_INT <= Build.VERSION_CODES.HONEYCOMB_MR2);
            boolean sizeChange = mThumbnailBitmap.getWidth() != width
                              || mThumbnailBitmap.getHeight() != height;
            if (honeycomb || sizeChange) {
                mThumbnailBitmap = null;
            }
        }

        if (mThumbnailBitmap == null) {
            Bitmap.Config config = (GeckoAppShell.getScreenDepth() == 24) ?
                Bitmap.Config.ARGB_8888 : Bitmap.Config.RGB_565;
            mThumbnailBitmap = Bitmap.createBitmap(width, height, config);
        }

        return mThumbnailBitmap;
    }

    public void updateThumbnail(final Bitmap b, final ThumbnailHelper.CachePolicy cachePolicy) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                if (b != null) {
                    try {
                        mThumbnail = new BitmapDrawable(mAppContext.getResources(), b);
                        if (mState == Tab.STATE_SUCCESS && cachePolicy == ThumbnailHelper.CachePolicy.STORE) {
                            saveThumbnailToDB(mDB);
                        } else {
                            
                            
                            clearThumbnailFromDB(mDB);
                        }
                    } catch (OutOfMemoryError oom) {
                        Log.w(LOGTAG, "Unable to create/scale bitmap.", oom);
                        mThumbnail = null;
                    }
                } else {
                    mThumbnail = null;
                }

                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.THUMBNAIL);
            }
        });
    }

    public synchronized String getFaviconURL() {
        return mFaviconUrl;
    }

    public boolean hasFeeds() {
        return mHasFeeds;
    }

    public boolean hasOpenSearch() {
        return mHasOpenSearch;
    }

    public SiteIdentity getSiteIdentity() {
        return mSiteIdentity;
    }

    public boolean isBookmark() {
        return mBookmark;
    }

    public boolean isInReadingList() {
        return mIsInReadingList;
    }

    public boolean isExternal() {
        return mExternal;
    }

    public synchronized void updateURL(String url) {
        if (url != null && url.length() > 0) {
            mUrl = url;
        }
    }

    public synchronized void updateUserRequested(String userRequested) {
        mUserRequested = userRequested;
    }

    public void setErrorType(String type) {
        if ("blocked".equals(type))
            setErrorType(ErrorType.BLOCKED);
        else if ("certerror".equals(type))
            setErrorType(ErrorType.CERT_ERROR);
        else if ("neterror".equals(type))
            setErrorType(ErrorType.NET_ERROR);
        else
            setErrorType(ErrorType.NONE);
    }

    public void setErrorType(ErrorType type) {
        mErrorType = type;
    }

    public void setMetadata(JSONObject metadata) {
        if (metadata == null) {
            return;
        }

        final ContentResolver cr = mAppContext.getContentResolver();
        final URLMetadata urlMetadata = mDB.getURLMetadata();

        final Map<String, Object> data = urlMetadata.fromJSON(metadata);
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                urlMetadata.save(cr, mUrl, data);
            }
        });
    }

    public ErrorType getErrorType() {
        return mErrorType;
    }

    public void setContentType(String contentType) {
        mContentType = (contentType == null) ? "" : contentType;
    }

    public String getContentType() {
        return mContentType;
    }

    public int getHistoryIndex() {
        return mHistoryIndex;
    }

    public int getHistorySize() {
        return mHistorySize;
    }

    public synchronized void updateTitle(String title) {
        
        if (mEnteringReaderMode) {
            return;
        }

        
        if (mTitle != null &&
            TextUtils.equals(mTitle, title)) {
            return;
        }

        mTitle = (title == null ? "" : title);
        Tabs.getInstance().notifyListeners(this, Tabs.TabEvents.TITLE);
    }

    public void setState(int state) {
        mState = state;

        if (mState != Tab.STATE_LOADING)
            mEnteringReaderMode = false;
    }

    public int getState() {
        return mState;
    }

    public void setZoomConstraints(ZoomConstraints constraints) {
        mZoomConstraints = constraints;
    }

    public ZoomConstraints getZoomConstraints() {
        return mZoomConstraints;
    }

    public void setIsRTL(boolean aIsRTL) {
        mIsRTL = aIsRTL;
    }

    public boolean getIsRTL() {
        return mIsRTL;
    }

    public void setHasTouchListeners(boolean aValue) {
        mHasTouchListeners = aValue;
    }

    public boolean getHasTouchListeners() {
        return mHasTouchListeners;
    }

    public synchronized void addFavicon(String faviconURL, int faviconSize, String mimeType) {
        RemoteFavicon favicon = new RemoteFavicon(faviconURL, faviconSize, mimeType);

        
        synchronized (mAvailableFavicons) {
            mAvailableFavicons.add(favicon);
        }
    }

    public void loadFavicon() {
        
        if (!mAvailableFavicons.isEmpty()) {
            RemoteFavicon newFavicon = mAvailableFavicons.first();

            
            if (newFavicon.faviconUrl.equals(mFaviconUrl)) {
                return;
            }

            Favicons.cancelFaviconLoad(mFaviconLoadId);
            mFaviconUrl = newFavicon.faviconUrl;
        } else {
            
            mFaviconUrl = null;
        }

        int flags = (isPrivate() || mErrorType != ErrorType.NONE) ? 0 : LoadFaviconTask.FLAG_PERSIST;
        mFaviconLoadId = Favicons.getSizedFavicon(mAppContext, mUrl, mFaviconUrl, Favicons.browserToolbarFaviconSize, flags,
                new OnFaviconLoadedListener() {
                    @Override
                    public void onFaviconLoaded(String pageUrl, String faviconURL, Bitmap favicon) {
                        
                        
                        if (!pageUrl.equals(mUrl)) {
                            return;
                        }

                        
                        if (favicon == null) {
                            
                            if (!mAvailableFavicons.isEmpty()) {
                                
                                mAvailableFavicons.remove(mAvailableFavicons.first());

                                
                                
                                loadFavicon();

                                return;
                            }

                            
                            favicon = Favicons.defaultFavicon;
                        }

                        mFavicon = favicon;
                        mFaviconLoadId = Favicons.NOT_LOADING;
                        Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.FAVICON);
                    }
                }
        );
    }

    public synchronized void clearFavicon() {
        
        
        Favicons.cancelFaviconLoad(mFaviconLoadId);

        
        if (mEnteringReaderMode)
            return;

        mFavicon = null;
        mFaviconUrl = null;
        mAvailableFavicons.clear();
    }

    public void setHasFeeds(boolean hasFeeds) {
        mHasFeeds = hasFeeds;
    }

    public void setHasOpenSearch(boolean hasOpenSearch) {
        mHasOpenSearch = hasOpenSearch;
    }

    public void updateIdentityData(JSONObject identityData) {
        mSiteIdentity.update(identityData);
    }

    void updateBookmark() {
        if (getURL() == null) {
            return;
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final String url = getURL();
                if (url == null) {
                    return;
                }

                mBookmark = mDB.isBookmark(getContentResolver(), url);
                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.MENU_UPDATED);
            }
        });
    }

    void updateReadingList() {
        if (getURL() == null) {
            return;
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final String url = getURL();
                if (url == null) {
                    return;
                }

                mIsInReadingList = mDB.getReadingListAccessor().isReadingListItem(getContentResolver(), url);
                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.MENU_UPDATED);
            }
        });
    }

    public void addBookmark() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                String url = getURL();
                if (url == null)
                    return;

                mDB.addBookmark(getContentResolver(), mTitle, url);
                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.BOOKMARK_ADDED);
            }
        });
    }

    public void removeBookmark() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                String url = getURL();
                if (url == null)
                    return;

                mDB.removeBookmarksWithURL(getContentResolver(), url);
                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.BOOKMARK_REMOVED);
            }
        });
    }

    public void addToReadingList() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                String url = getURL();
                if (url == null) {
                    return;
                }

                mDB.getReadingListAccessor().addBasicReadingListItem(getContentResolver(), url, mTitle);
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mAppContext, R.string.reading_list_added, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
    }

    public void removeFromReadingList() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                String url = getURL();
                if (url == null) {
                    return;
                }
                if (AboutPages.isAboutReader(url)) {
                    url = ReaderModeUtils.getUrlFromAboutReader(url);
                }
                mDB.getReadingListAccessor().removeReadingListItemWithURL(getContentResolver(), url);
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mAppContext, R.string.reading_list_removed, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
    }

    public void toggleReaderMode() {
        if (AboutPages.isAboutReader(mUrl)) {
            Tabs.getInstance().loadUrl(ReaderModeUtils.getUrlFromAboutReader(mUrl));
        } else {
            mEnteringReaderMode = true;
            Tabs.getInstance().loadUrl(ReaderModeUtils.getAboutReaderForUrl(mUrl, mId));
        }
    }

    public boolean isEnteringReaderMode() {
        return mEnteringReaderMode;
    }

    public void doReload() {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Reload", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    
    public boolean canDoBack() {
        return mCanDoBack;
    }

    public boolean doBack() {
        if (!canDoBack())
            return false;

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Back", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    public void doStop() {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Stop", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    
    public boolean canDoForward() {
        return mCanDoForward;
    }

    public boolean doForward() {
        if (!canDoForward())
            return false;

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Forward", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    void handleLocationChange(JSONObject message) throws JSONException {
        final String uri = message.getString("uri");
        final String oldUrl = getURL();
        final boolean sameDocument = message.getBoolean("sameDocument");
        mEnteringReaderMode = ReaderModeUtils.isEnteringReaderMode(oldUrl, uri);
        mHistoryIndex = message.getInt("historyIndex");
        mHistorySize = message.getInt("historySize");
        mCanDoBack = message.getBoolean("canGoBack");
        mCanDoForward = message.getBoolean("canGoForward");

        if (!TextUtils.equals(oldUrl, uri)) {
            updateURL(uri);
            updateBookmark();
            updateReadingList();
            if (!sameDocument) {
                
                
                
                
                clearFavicon();
                updateTitle(null);
            }
        }

        if (sameDocument) {
            
            
            Tabs.getInstance().notifyListeners(this, Tabs.TabEvents.LOCATION_CHANGE, oldUrl);
            return;
        }

        setContentType(message.getString("contentType"));
        updateUserRequested(message.getString("userRequested"));
        mBaseDomain = message.optString("baseDomain");

        setHasFeeds(false);
        setHasOpenSearch(false);
        mSiteIdentity.reset();
        setZoomConstraints(new ZoomConstraints(true));
        setHasTouchListeners(false);
        setBackgroundColor(DEFAULT_BACKGROUND_COLOR);
        setErrorType(ErrorType.NONE);
        setLoadProgressIfLoading(LOAD_PROGRESS_LOCATION_CHANGE);

        Tabs.getInstance().notifyListeners(this, Tabs.TabEvents.LOCATION_CHANGE, oldUrl);
    }

    private static boolean shouldShowProgress(final String url) {
        return !AboutPages.isAboutPage(url);
    }

    void handleDocumentStart(boolean restoring, String url) {
        setLoadProgress(LOAD_PROGRESS_START);
        setState((!restoring && shouldShowProgress(url)) ? STATE_LOADING : STATE_SUCCESS);
        mSiteIdentity.reset();
    }

    void handleDocumentStop(boolean success) {
        setState(success ? STATE_SUCCESS : STATE_ERROR);

        final String oldURL = getURL();
        final Tab tab = this;
        tab.setLoadProgress(LOAD_PROGRESS_STOP);

        ThreadUtils.getBackgroundHandler().postDelayed(new Runnable() {
            @Override
            public void run() {
                
                if (!TextUtils.equals(oldURL, getURL()))
                    return;

                ThumbnailHelper.getInstance().getAndProcessThumbnailFor(tab);
            }
        }, 500);
    }

    void handleContentLoaded() {
        setLoadProgressIfLoading(LOAD_PROGRESS_LOADED);
    }

    protected void saveThumbnailToDB(final BrowserDB db) {
        final BitmapDrawable thumbnail = mThumbnail;
        if (thumbnail == null) {
            return;
        }

        try {
            final String url = getURL();
            if (url == null) {
                return;
            }

            db.updateThumbnailForUrl(getContentResolver(), url, thumbnail);
        } catch (Exception e) {
            
        }
    }

    public void loadThumbnailFromDB(final BrowserDB db) {
        try {
            final String url = getURL();
            if (url == null) {
                return;
            }

            byte[] thumbnail = db.getThumbnailForUrl(getContentResolver(), url);
            if (thumbnail == null) {
                return;
            }

            Bitmap bitmap = BitmapUtils.decodeByteArray(thumbnail);
            mThumbnail = new BitmapDrawable(mAppContext.getResources(), bitmap);

            Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.THUMBNAIL);
        } catch (Exception e) {
            
        }
    }

    private void clearThumbnailFromDB(final BrowserDB db) {
        try {
            final String url = getURL();
            if (url == null) {
                return;
            }

            
            db.updateThumbnailForUrl(getContentResolver(), url, null);
        } catch (Exception e) {
            
        }
    }

    public void addPluginView(View view) {
        mPluginViews.add(view);
    }

    public void removePluginView(View view) {
        mPluginViews.remove(view);
    }

    public View[] getPluginViews() {
        return mPluginViews.toArray(new View[mPluginViews.size()]);
    }

    public void addPluginLayer(Object surfaceOrView, Layer layer) {
        synchronized(mPluginLayers) {
            mPluginLayers.put(surfaceOrView, layer);
        }
    }

    public Layer getPluginLayer(Object surfaceOrView) {
        synchronized(mPluginLayers) {
            return mPluginLayers.get(surfaceOrView);
        }
    }

    public Collection<Layer> getPluginLayers() {
        synchronized(mPluginLayers) {
            return new ArrayList<Layer>(mPluginLayers.values());
        }
    }

    public Layer removePluginLayer(Object surfaceOrView) {
        synchronized(mPluginLayers) {
            return mPluginLayers.remove(surfaceOrView);
        }
    }

    public int getBackgroundColor() {
        return mBackgroundColor;
    }

    
    public void setBackgroundColor(int color) {
        mBackgroundColor = color;
    }

    
    public void setBackgroundColor(String newColor) {
        setBackgroundColor(parseColorFromGecko(newColor));
    }

    
    
    private static int parseColorFromGecko(String string) {
        if (sColorPattern == null) {
            sColorPattern = Pattern.compile("rgb\\((\\d+),\\s*(\\d+),\\s*(\\d+)\\)");
        }

        Matcher matcher = sColorPattern.matcher(string);
        if (!matcher.matches()) {
            return Color.WHITE;
        }

        int r = Integer.parseInt(matcher.group(1));
        int g = Integer.parseInt(matcher.group(2));
        int b = Integer.parseInt(matcher.group(3));
        return Color.rgb(r, g, b);
    }

    public void setDesktopMode(boolean enabled) {
        mDesktopMode = enabled;
    }

    public boolean getDesktopMode() {
        return mDesktopMode;
    }

    public boolean isPrivate() {
        return false;
    }

    




    void setLoadProgress(int progressPercentage) {
        mLoadProgress = progressPercentage;
    }

    









    void setLoadProgressIfLoading(int progressPercentage) {
        if (getState() == STATE_LOADING) {
            setLoadProgress(progressPercentage);
        }
    }

    




    public int getLoadProgress() {
        return mLoadProgress;
    }

    public void setRecording(boolean isRecording) {
        if (isRecording) {
            mRecordingCount++;
        } else {
            mRecordingCount--;
        }
    }

    public boolean isRecording() {
        return mRecordingCount > 0;
    }

    public boolean isEditing() {
        return mIsEditing;
    }

    public void setIsEditing(final boolean isEditing) {
        this.mIsEditing = isEditing;
    }

    public TabEditingState getEditingState() {
        return mEditingState;
    }
}
