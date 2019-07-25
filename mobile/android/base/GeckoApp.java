







































package org.mozilla.gecko;

import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.GeckoSoftwareLayerClient;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PlaceholderLayerClient;
import org.mozilla.gecko.gfx.RectUtils;
import org.mozilla.gecko.gfx.ViewportMetrics;
import org.mozilla.gecko.Tab.HistoryEntry;

import java.io.*;
import java.util.*;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.zip.*;
import java.net.URL;
import java.nio.*;
import java.nio.channels.FileChannel;
import java.util.concurrent.*;
import java.lang.reflect.*;
import java.net.*;

import org.json.*;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.view.ViewGroup.LayoutParams;
import android.content.*;
import android.content.res.*;
import android.graphics.*;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.BitmapDrawable;
import android.widget.*;
import android.hardware.*;
import android.location.*;

import android.util.*;
import android.net.*;
import android.database.*;
import android.database.sqlite.*;
import android.provider.*;
import android.content.pm.*;
import android.content.pm.PackageManager.*;
import dalvik.system.*;

abstract public class GeckoApp
    extends Activity implements GeckoEventListener, SensorEventListener, LocationListener
{
    private static final String LOGTAG = "GeckoApp";

    public static enum StartupMode {
        NORMAL,
        NEW_VERSION,
        NEW_PROFILE
    }

    public static final String ACTION_ALERT_CLICK   = "org.mozilla.gecko.ACTION_ALERT_CLICK";
    public static final String ACTION_ALERT_CLEAR   = "org.mozilla.gecko.ACTION_ALERT_CLEAR";
    public static final String ACTION_WEBAPP        = "org.mozilla.gecko.WEBAPP";
    public static final String ACTION_DEBUG         = "org.mozilla.gecko.DEBUG";
    public static final String ACTION_BOOKMARK      = "org.mozilla.gecko.BOOKMARK";
    public static final String ACTION_LOAD          = "org.mozilla.gecko.LOAD";
    public static final String ACTION_UPDATE        = "org.mozilla.gecko.UPDATE";
    public static final String SAVED_STATE_URI      = "uri";
    public static final String SAVED_STATE_TITLE    = "title";
    public static final String SAVED_STATE_VIEWPORT = "viewport";
    public static final String SAVED_STATE_SCREEN   = "screen";
    public static final String SAVED_STATE_SESSION  = "session";

    StartupMode mStartupMode = null;
    private LinearLayout mMainLayout;
    private RelativeLayout mGeckoLayout;
    public static SurfaceView cameraView;
    public static GeckoApp mAppContext;
    public static boolean mDOMFullScreen = false;
    public static File sGREDir = null;
    public static Menu sMenu;
    private static GeckoThread sGeckoThread = null;
    public Handler mMainHandler;
    private File mProfileDir;
    public static boolean sIsGeckoReady = false;
    public static int mOrientation;

    private IntentFilter mConnectivityFilter;

    private BroadcastReceiver mConnectivityReceiver;
    private BroadcastReceiver mBatteryReceiver;

    public static BrowserToolbar mBrowserToolbar;
    public static DoorHangerPopup mDoorHangerPopup;
    public static AutoCompletePopup mAutoCompletePopup;
    public Favicons mFavicons;

    private Geocoder mGeocoder;
    private Address  mLastGeoAddress;
    private static LayerController mLayerController;
    private static PlaceholderLayerClient mPlaceholderLayerClient;
    private static GeckoSoftwareLayerClient mSoftwareLayerClient;
    private AboutHomeContent mAboutHomeContent;
    private static AbsoluteLayout mPluginContainer;

    public String mLastTitle;
    public String mLastViewport;
    public byte[] mLastScreen;
    public int mOwnActivityDepth = 0;
    private boolean mRestoreSession = false;

    private Vector<View> mPluginViews = new Vector<View>();

    public interface OnTabsChangedListener {
        public void onTabsChanged(Tab tab);
    }
    
    private static ArrayList<OnTabsChangedListener> mTabsChangedListeners;

    static class ExtraMenuItem implements MenuItem.OnMenuItemClickListener {
        String label;
        String icon;
        int id;
        public boolean onMenuItemClick(MenuItem item) {
            Log.i(LOGTAG, "menu item clicked");
            GeckoAppShell.sendEventToGecko(new GeckoEvent("Menu:Clicked", Integer.toString(id)));
            return true;
        }
    }

    static Vector<ExtraMenuItem> sExtraMenuItems = new Vector<ExtraMenuItem>();

    public enum LaunchState {Launching, WaitForDebugger,
                             Launched, GeckoRunning, GeckoExiting};
    private static LaunchState sLaunchState = LaunchState.Launching;
    private static boolean sTryCatchAttached = false;

    private static final int FILE_PICKER_REQUEST = 1;
    private static final int AWESOMEBAR_REQUEST = 2;
    private static final int CAMERA_CAPTURE_REQUEST = 3;

    public static boolean checkLaunchState(LaunchState checkState) {
        synchronized(sLaunchState) {
            return sLaunchState == checkState;
        }
    }

    static void setLaunchState(LaunchState setState) {
        synchronized(sLaunchState) {
            sLaunchState = setState;
        }
    }

    
    
    static boolean checkAndSetLaunchState(LaunchState checkState, LaunchState setState) {
        synchronized(sLaunchState) {
            if (sLaunchState != checkState)
                return false;
            sLaunchState = setState;
            return true;
        }
    }

    public static final String PLUGIN_ACTION = "android.webkit.PLUGIN";

    



    public static final String PLUGIN_PERMISSION = "android.webkit.permission.PLUGIN";

    private static final String PLUGIN_SYSTEM_LIB = "/system/lib/plugins/";

    private static final String PLUGIN_TYPE = "type";
    private static final String TYPE_NATIVE = "native";
    public ArrayList<PackageInfo> mPackageInfoCache = new ArrayList<PackageInfo>();

    String[] getPluginDirectories() {
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
            return new String[0];

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - start of getPluginDirectories");

        ArrayList<String> directories = new ArrayList<String>();
        PackageManager pm = mAppContext.getPackageManager();
        List<ResolveInfo> plugins = pm.queryIntentServices(new Intent(PLUGIN_ACTION),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);

        synchronized(mPackageInfoCache) {

            
            mPackageInfoCache.clear();


            for (ResolveInfo info : plugins) {

                
                ServiceInfo serviceInfo = info.serviceInfo;
                if (serviceInfo == null) {
                    Log.w(LOGTAG, "Ignore bad plugin");
                    continue;
                }

                
                
                
                if (serviceInfo.packageName.equals("com.htc.flashliteplugin")) {
                    Log.w(LOGTAG, "Skipping HTC's flash lite plugin");
                    continue;
                }

                Log.w(LOGTAG, "Loading plugin: " + serviceInfo.packageName);


                
                PackageInfo pkgInfo;
                try {
                    pkgInfo = pm.getPackageInfo(serviceInfo.packageName,
                                    PackageManager.GET_PERMISSIONS
                                    | PackageManager.GET_SIGNATURES);
                } catch (Exception e) {
                    Log.w(LOGTAG, "Can't find plugin: " + serviceInfo.packageName);
                    continue;
                }
                if (pkgInfo == null) {
                    Log.w(LOGTAG, "Loading plugin: " + serviceInfo.packageName + ". Could not load package information.");
                    continue;
                }

                





                String directory = pkgInfo.applicationInfo.dataDir + "/lib";
                final int appFlags = pkgInfo.applicationInfo.flags;
                final int updatedSystemFlags = ApplicationInfo.FLAG_SYSTEM |
                                               ApplicationInfo.FLAG_UPDATED_SYSTEM_APP;
                
                if ((appFlags & updatedSystemFlags) == ApplicationInfo.FLAG_SYSTEM) {
                    directory = PLUGIN_SYSTEM_LIB + pkgInfo.packageName;
                }

                
                String permissions[] = pkgInfo.requestedPermissions;
                if (permissions == null) {
                    Log.w(LOGTAG, "Loading plugin: " + serviceInfo.packageName + ". Does not have required permission.");
                    continue;
                }
                boolean permissionOk = false;
                for (String permit : permissions) {
                    if (PLUGIN_PERMISSION.equals(permit)) {
                        permissionOk = true;
                        break;
                    }
                }
                if (!permissionOk) {
                    Log.w(LOGTAG, "Loading plugin: " + serviceInfo.packageName + ". Does not have required permission (2).");
                    continue;
                }

                
                Signature signatures[] = pkgInfo.signatures;
                if (signatures == null) {
                    Log.w(LOGTAG, "Loading plugin: " + serviceInfo.packageName + ". Not signed.");
                    continue;
                }

                
                if (serviceInfo.metaData == null) {
                    Log.e(LOGTAG, "The plugin '" + serviceInfo.name + "' has no type defined");
                    continue;
                }

                String pluginType = serviceInfo.metaData.getString(PLUGIN_TYPE);
                if (!TYPE_NATIVE.equals(pluginType)) {
                    Log.e(LOGTAG, "Unrecognized plugin type: " + pluginType);
                    continue;
                }

                try {
                    Class<?> cls = getPluginClass(serviceInfo.packageName, serviceInfo.name);

                    
                    boolean classFound = true;

                    if (!classFound) {
                        Log.e(LOGTAG, "The plugin's class' " + serviceInfo.name + "' does not extend the appropriate class.");
                        continue;
                    }

                } catch (NameNotFoundException e) {
                    Log.e(LOGTAG, "Can't find plugin: " + serviceInfo.packageName);
                    continue;
                } catch (ClassNotFoundException e) {
                    Log.e(LOGTAG, "Can't find plugin's class: " + serviceInfo.name);
                    continue;
                }

                
                mPackageInfoCache.add(pkgInfo);
                directories.add(directory);
            }
        }

        String [] result = directories.toArray(new String[directories.size()]);
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - end of getPluginDirectories");
        return result;
    }

    String getPluginPackage(String pluginLib) {

        if (pluginLib == null || pluginLib.length() == 0) {
            return null;
        }

        synchronized(mPackageInfoCache) {
            for (PackageInfo pkgInfo : mPackageInfoCache) {
                if (pluginLib.contains(pkgInfo.packageName)) {
                    return pkgInfo.packageName;
                }
            }
        }

        return null;
    }

    Class<?> getPluginClass(String packageName, String className)
            throws NameNotFoundException, ClassNotFoundException {
        Context pluginContext = mAppContext.createPackageContext(packageName,
                Context.CONTEXT_INCLUDE_CODE |
                Context.CONTEXT_IGNORE_SECURITY);
        ClassLoader pluginCL = pluginContext.getClassLoader();
        return pluginCL.loadClass(className);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        sMenu = menu;
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.layout.gecko_menu, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu aMenu)
    {
        Iterator<ExtraMenuItem> i = sExtraMenuItems.iterator();
        while (i.hasNext()) {
            final ExtraMenuItem item = i.next();
            if (aMenu.findItem(item.id) == null) {
                final MenuItem mi = aMenu.add(Menu.NONE, item.id, Menu.NONE, item.label);
                if (item.icon != null) {
                    if (item.icon.startsWith("data")) {
                        byte[] raw = Base64.decode(item.icon.substring(22), Base64.DEFAULT);
                        Bitmap bitmap = BitmapFactory.decodeByteArray(raw, 0, raw.length);
                        BitmapDrawable drawable = new BitmapDrawable(bitmap);
                        mi.setIcon(drawable);
                    }
                    else if (item.icon.startsWith("jar:") || item.icon.startsWith("file://")) {
                        GeckoAppShell.getHandler().post(new Runnable() {
                            public void run() {
                                try {
                                    URL url = new URL(item.icon);
                                    InputStream is = (InputStream) url.getContent();
                                    Drawable drawable = Drawable.createFromStream(is, "src");
                                    mi.setIcon(drawable);
                                } catch (Exception e) {
                                    Log.w(LOGTAG, "onPrepareOptionsMenu: Unable to set icon", e);
                                }
                            }
                        });
                    }
                }
                mi.setOnMenuItemClickListener(item);
            }
        }

        if (!sIsGeckoReady)
            aMenu.findItem(R.id.settings).setEnabled(false);

        Tab tab = Tabs.getInstance().getSelectedTab();
        MenuItem bookmark = aMenu.findItem(R.id.bookmark);
        MenuItem forward = aMenu.findItem(R.id.forward);
        MenuItem share = aMenu.findItem(R.id.share);
        MenuItem saveAsPDF = aMenu.findItem(R.id.save_as_pdf);
        MenuItem downloads = aMenu.findItem(R.id.downloads);
        MenuItem charEncoding = aMenu.findItem(R.id.char_encoding);

        if (tab == null) {
            bookmark.setEnabled(false);
            forward.setEnabled(false);
            share.setEnabled(false);
            saveAsPDF.setEnabled(false);
            return true;
        }
        
        bookmark.setEnabled(true);
        bookmark.setCheckable(true);
        
        if (tab.isBookmark()) {
            bookmark.setChecked(true);
            bookmark.setIcon(R.drawable.ic_menu_bookmark_remove);
        } else {
            bookmark.setChecked(false);
            bookmark.setIcon(R.drawable.ic_menu_bookmark_add);
        }

        forward.setEnabled(tab.canDoForward());

        
        String scheme = Uri.parse(tab.getURL()).getScheme();
        boolean enabled = !(scheme.equals("about") || scheme.equals("chrome") ||
                            scheme.equals("file"));
        share.setEnabled(enabled);

        
        saveAsPDF.setEnabled(!(tab.getURL().equals("about:home") ||
                               tab.getContentType().equals("application/vnd.mozilla.xul+xml")));

        
        if (Build.VERSION.SDK_INT < 12)
            downloads.setVisible(false);

        charEncoding.setVisible(GeckoPreferences.getCharEncodingState());

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Tab tab = null;
        Intent intent = null;
        switch (item.getItemId()) {
            case R.id.quit:
                synchronized(sLaunchState) {
                    if (sLaunchState == LaunchState.GeckoRunning)
                        GeckoAppShell.notifyGeckoOfEvent(
                            new GeckoEvent("Browser:Quit", null));
                    else
                        System.exit(0);
                    sLaunchState = LaunchState.GeckoExiting;
                }
                return true;
            case R.id.bookmark:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    if (item.isChecked()) {
                        tab.removeBookmark();
                        Toast.makeText(this, R.string.bookmark_removed, Toast.LENGTH_SHORT).show();
                        item.setIcon(R.drawable.ic_menu_bookmark_add);
                    } else {
                        tab.addBookmark();
                        Toast.makeText(this, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        item.setIcon(R.drawable.ic_menu_bookmark_remove);
                    }
                }
                return true;
            case R.id.share:
                tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                  GeckoAppShell.openUriExternal(tab.getURL(), "text/plain", "", "",
                                                Intent.ACTION_SEND, tab.getTitle());
                }
                return true;
            case R.id.reload:
                doReload();
                return true;
            case R.id.forward:
                doForward();
                return true;
            case R.id.save_as_pdf:
                GeckoAppShell.sendEventToGecko(new GeckoEvent("SaveAs:PDF", null));
                return true;
            case R.id.settings:
                intent = new Intent(this, GeckoPreferences.class);
                startActivity(intent);
                return true;
            case R.id.site_settings:
                GeckoAppShell.sendEventToGecko(new GeckoEvent("Permissions:Get", null));
                return true;
            case R.id.addons:
                loadUrlInTab("about:addons");
                return true;
            case R.id.downloads:
                intent = new Intent(DownloadManager.ACTION_VIEW_DOWNLOADS);
                startActivity(intent);
                return true;
            case R.id.char_encoding:
                GeckoAppShell.sendEventToGecko(new GeckoEvent("CharEncoding:Get", null));
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    public String getLastViewport() {
        return mLastViewport;
    }

    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        if (mOwnActivityDepth > 0)
            return; 

        if (outState == null)
            outState = new Bundle();

        new SessionSnapshotRunnable(null).run();

        outState.putString(SAVED_STATE_TITLE, mLastTitle);
        outState.putString(SAVED_STATE_VIEWPORT, mLastViewport);
        outState.putByteArray(SAVED_STATE_SCREEN, mLastScreen);
        outState.putBoolean(SAVED_STATE_SESSION, true);
    }

    public class SessionSnapshotRunnable implements Runnable {
        Tab mThumbnailTab;
        SessionSnapshotRunnable(Tab thumbnailTab) {
            mThumbnailTab = thumbnailTab;
        }

        public void run() {
            synchronized (mSoftwareLayerClient) {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab == null)
                    return;

                HistoryEntry lastHistoryEntry = tab.getLastHistoryEntry();
                if (lastHistoryEntry == null)
                    return;

                if (getLayerController().getLayerClient() != mSoftwareLayerClient)
                    return;

                ViewportMetrics viewportMetrics = mSoftwareLayerClient.getGeckoViewportMetrics();
                if (viewportMetrics != null)
                    mLastViewport = viewportMetrics.toJSON();

                mLastTitle = lastHistoryEntry.mTitle;
                getAndProcessThumbnailForTab(tab);
            }
        }
    }

    void getAndProcessThumbnailForTab(final Tab tab) {
        boolean isSelectedTab = Tabs.getInstance().isSelectedTab(tab);
        final Bitmap bitmap = isSelectedTab ?
            mSoftwareLayerClient.getBitmap() : null;
        
        if (bitmap != null) {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            bitmap.compress(Bitmap.CompressFormat.PNG, 0, bos);
            processThumbnail(tab, bitmap, bos.toByteArray());
        } else {
            mLastScreen = null;
            int sw = isSelectedTab ? mSoftwareLayerClient.getWidth() : tab.getMinScreenshotWidth();
            int sh = isSelectedTab ? mSoftwareLayerClient.getHeight(): tab.getMinScreenshotHeight();
            int dw = isSelectedTab ? sw : tab.getThumbnailWidth();
            int dh = isSelectedTab ? sh : tab.getThumbnailHeight();
            try {
                JSONObject message = new JSONObject();
                message.put("tabID", tab.getId());

                JSONObject source = new JSONObject();
                source.put("width", sw);
                source.put("height", sh);
                message.put("source", source);

                JSONObject destination = new JSONObject();
                source.put("width", dw);
                source.put("height", dh);
                message.put("destination", destination);

                String json = message.toString();
                GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Screenshot", json));
            } catch(JSONException jsonEx) {
                Log.w(LOGTAG, "Constructing the JSON data for Tab:Screenshot event failed", jsonEx);
            }
        }
    }
    
    void processThumbnail(Tab thumbnailTab, Bitmap bitmap, byte[] compressed) {
        if (Tabs.getInstance().isSelectedTab(thumbnailTab))
            mLastScreen = compressed;
        if (thumbnailTab.getURL().equals("about:home")) {
            thumbnailTab.updateThumbnail(null);
            return;
        }
        try {
            if (bitmap == null)
                bitmap = BitmapFactory.decodeByteArray(compressed, 0, compressed.length);
            thumbnailTab.updateThumbnail(bitmap);
        } catch (OutOfMemoryError ome) {
            Log.w(LOGTAG, "decoding byte array ran out of memory", ome);
        }
    }

    private void maybeCancelFaviconLoad(Tab tab) {
        long faviconLoadId = tab.getFaviconLoadId();

        if (faviconLoadId == Favicons.NOT_LOADING)
            return;

        
        mFavicons.cancelFaviconLoad(faviconLoadId);

        
        tab.setFaviconLoadId(Favicons.NOT_LOADING);
    }

    private void loadFavicon(final Tab tab) {
        maybeCancelFaviconLoad(tab);

        long id = mFavicons.loadFavicon(tab.getURL(), tab.getFaviconURL(),
                        new Favicons.OnFaviconLoadedListener() {

            public void onFaviconLoaded(String pageUrl, Drawable favicon) {
                
                
                if (favicon == null)
                    return;

                Log.i(LOGTAG, "Favicon successfully loaded for URL = " + pageUrl);

                
                
                if (!tab.getURL().equals(pageUrl))
                    return;

                Log.i(LOGTAG, "Favicon is for current URL = " + pageUrl);

                tab.updateFavicon(favicon);
                tab.setFaviconLoadId(Favicons.NOT_LOADING);

                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setFavicon(tab.getFavicon());

                onTabsChanged(tab);
            }
        });

        tab.setFaviconLoadId(id);
    }

    void handleLocationChange(final int tabId, final String uri,
                              final String documentURI, final String contentType) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        if (Tabs.getInstance().isSelectedTab(tab)) {
            if (uri.equals("about:home"))
                showAboutHome();
            else 
                hideAboutHome();
        }
        
        String oldBaseURI = tab.getURL();
        tab.updateURL(uri);
        tab.setDocumentURI(documentURI);
        tab.setContentType(contentType);

        String baseURI = uri;
        if (baseURI.indexOf('#') != -1)
            baseURI = uri.substring(0, uri.indexOf('#'));

        if (oldBaseURI != null && oldBaseURI.indexOf('#') != -1)
            oldBaseURI = oldBaseURI.substring(0, oldBaseURI.indexOf('#'));
        
        if (baseURI.equals(oldBaseURI)) {
            mMainHandler.post(new Runnable() {
                public void run() {
                    if (Tabs.getInstance().isSelectedTab(tab)) {
                        mBrowserToolbar.setTitle(uri);
                    }
                }
            });
            return;
        }

        tab.updateFavicon(null);
        tab.updateFaviconURL(null);
        tab.updateSecurityMode("unknown");
        tab.removeTransientDoorHangers();
        tab.setHasTouchListeners(false);

        maybeCancelFaviconLoad(tab);

        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    mBrowserToolbar.setTitle(uri);
                    mBrowserToolbar.setFavicon(null);
                    mBrowserToolbar.setSecurityMode("unknown");
                    mDoorHangerPopup.updatePopup();
                    mBrowserToolbar.setShadowVisibility(!(tab.getURL().startsWith("about:")));
                    mLayerController.setWaitForTouchListeners(false);
                }
            }
        });
    }

    void handleSecurityChange(final int tabId, final String mode) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.updateSecurityMode(mode);
        
        mMainHandler.post(new Runnable() { 
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setSecurityMode(mode);
            }
        });
    }

    void handleLoadError(final int tabId, final String uri, final String title) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;
    
        
        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    mBrowserToolbar.setTitle(tab.getDisplayTitle());
                    mBrowserToolbar.setFavicon(tab.getFavicon());
                    mBrowserToolbar.setSecurityMode(tab.getSecurityMode());
                    mBrowserToolbar.setProgressVisibility(tab.isLoading());
                }
            }
        });
    }

    public StartupMode getStartupMode() {
        
        

        synchronized(this) {
            if (mStartupMode != null)
                return mStartupMode;

            String packageName = getPackageName();
            SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);

            
            
            String keyName = packageName + ".default.startup_version";
            String appVersion = null;

            try {
                PackageInfo pkgInfo = getPackageManager().getPackageInfo(packageName, 0);
                appVersion = pkgInfo.versionName;
            } catch(NameNotFoundException nnfe) {
                
                
                mStartupMode = StartupMode.NORMAL;
                return mStartupMode;
            }

            String startupVersion = settings.getString(keyName, null);
            if (startupVersion == null) {
                mStartupMode = StartupMode.NEW_PROFILE;
            } else {
                if (startupVersion.equals(appVersion))
                    mStartupMode = StartupMode.NORMAL;
                else
                    mStartupMode = StartupMode.NEW_VERSION;
            }

            if (mStartupMode != StartupMode.NORMAL)
                settings.edit().putString(keyName, appVersion).commit();

            Log.i(LOGTAG, "Startup mode: " + mStartupMode);

            return mStartupMode;
        }
    }

    public File getProfileDir() {
        return getProfileDir("default");
    }

    public File getProfileDir(final String profileName) {
        if (mProfileDir != null)
            return mProfileDir;
        try {
            mProfileDir = GeckoDirProvider.getProfileDir(mAppContext, profileName);
        } catch (IOException ex) {
            Log.e(LOGTAG, "Error getting profile dir.", ex);
        }
        return mProfileDir;
    }

    void addTab() {
        showAwesomebar(AwesomeBar.Type.ADD);
    }

    void showTabs() {
        Intent intent = new Intent(mAppContext, TabsTray.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
        startActivity(intent);
        overridePendingTransition(R.anim.grow_fade_in, 0);
    }

    public static void registerOnTabsChangedListener(OnTabsChangedListener listener) {
        if (mTabsChangedListeners == null)
            mTabsChangedListeners = new ArrayList<OnTabsChangedListener>();
        
        mTabsChangedListeners.add(listener);
    }

    public static void unregisterOnTabsChangedListener(OnTabsChangedListener listener) {
        if (mTabsChangedListeners == null)
            return;
        
        mTabsChangedListeners.remove(listener);
    }

    public void onTabsChanged(Tab tab) {
        if (mTabsChangedListeners == null)
            return;

        Iterator items = mTabsChangedListeners.iterator();
        while (items.hasNext()) {
            ((OnTabsChangedListener) items.next()).onTabsChanged(tab);
        }
    }

    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if (event.equals("Menu:Add")) {
                ExtraMenuItem item = new ExtraMenuItem();
                item.label = message.getString("name");
                item.id = message.getInt("id");
                try { 
                    item.icon = message.getString("icon");
                } catch (Exception ex) { }
                sExtraMenuItems.add(item);
            } else if (event.equals("Menu:Remove")) {
                
                Iterator<ExtraMenuItem> i = sExtraMenuItems.iterator();
                int id = message.getInt("id");
                while (i.hasNext()) {
                    ExtraMenuItem item = i.next();
                    if (item.id == id) {
                        sExtraMenuItems.remove(item);
                        MenuItem menu = sMenu.findItem(id);
                        if (menu != null)
                            sMenu.removeItem(id);
                        return;
                    }
                }
            } else if (event.equals("Toast:Show")) {
                final String msg = message.getString("message");
                final String duration = message.getString("duration");
                handleShowToast(msg, duration);
            } else if (event.equals("DOMContentLoaded")) {
                final int tabId = message.getInt("tabID");
                final String uri = message.getString("uri");
                final String title = message.getString("title");
                handleContentLoaded(tabId, uri, title);
                Log.i(LOGTAG, "URI - " + uri + ", title - " + title);
            } else if (event.equals("DOMTitleChanged")) {
                final int tabId = message.getInt("tabID");
                final String title = message.getString("title");
                handleTitleChanged(tabId, title);
                Log.i(LOGTAG, "title - " + title);
            } else if (event.equals("DOMLinkAdded")) {
                final int tabId = message.getInt("tabID");
                final String rel = message.getString("rel");
                final String href = message.getString("href");
                Log.i(LOGTAG, "link rel - " + rel + ", href - " + href);
                handleLinkAdded(tabId, rel, href);
            } else if (event.equals("DOMWindowClose")) {
                final int tabId = message.getInt("tabID");
                handleWindowClose(tabId);
            } else if (event.equals("log")) {
                
                final String msg = message.getString("msg");
                Log.i(LOGTAG, "Log: " + msg);
            } else if (event.equals("Content:LocationChange")) {
                final int tabId = message.getInt("tabID");
                final String uri = message.getString("uri");
                final String documentURI = message.getString("documentURI");
                final String contentType = message.getString("contentType");
                Log.i(LOGTAG, "URI - " + uri);
                handleLocationChange(tabId, uri, documentURI, contentType);
            } else if (event.equals("Content:SecurityChange")) {
                final int tabId = message.getInt("tabID");
                final String mode = message.getString("mode");
                Log.i(LOGTAG, "Security Mode - " + mode);
                handleSecurityChange(tabId, mode);
            } else if (event.equals("Content:StateChange")) {
                final int tabId = message.getInt("tabID");
                int state = message.getInt("state");
                Log.i(LOGTAG, "State - " + state);
                if ((state & GeckoAppShell.WPL_STATE_IS_NETWORK) != 0) {
                    if ((state & GeckoAppShell.WPL_STATE_START) != 0) {
                        Log.i(LOGTAG, "Got a document start");
                        final boolean showProgress = message.getBoolean("showProgress");
                        handleDocumentStart(tabId, showProgress);
                    } else if ((state & GeckoAppShell.WPL_STATE_STOP) != 0) {
                        Log.i(LOGTAG, "Got a document stop");
                        handleDocumentStop(tabId);
                    }
                }
            } else if (event.equals("Content:LoadError")) {
                final int tabId = message.getInt("tabID");
                final String uri = message.getString("uri");
                final String title = message.getString("title");
                handleLoadError(tabId, uri, title);
            } else if (event.equals("onCameraCapture")) {
                
                doCameraCapture();
            } else if (event.equals("Doorhanger:Add")) {
                handleDoorHanger(message);
            } else if (event.equals("Doorhanger:Remove")) {
                handleDoorHangerRemove(message);
            } else if (event.equals("Gecko:Ready")) {
                sIsGeckoReady = true;
                mMainHandler.post(new Runnable() {
                    public void run() {
                        if (sMenu != null)
                            sMenu.findItem(R.id.settings).setEnabled(true);
                    }
                });
                setLaunchState(GeckoApp.LaunchState.GeckoRunning);
                GeckoAppShell.sendPendingEventsToGecko();
                connectGeckoLayerClient();
            } else if (event.equals("ToggleChrome:Hide")) {
                mMainHandler.post(new Runnable() {
                    public void run() {
                        mBrowserToolbar.setVisibility(View.GONE);
                    }
                });
            } else if (event.equals("ToggleChrome:Show")) {
                mMainHandler.post(new Runnable() {
                    public void run() {
                        mBrowserToolbar.setVisibility(View.VISIBLE);
                    }
                });
            } else if (event.equals("DOMFullScreen:Start")) {
                mDOMFullScreen = true;
            } else if (event.equals("DOMFullScreen:Stop")) {
                mDOMFullScreen = false;
            } else if (event.equals("FormAssist:AutoComplete")) {
                final JSONArray suggestions = message.getJSONArray("suggestions");
                if (suggestions.length() == 0) {
                    mMainHandler.post(new Runnable() {
                        public void run() {
                            mAutoCompletePopup.hide();
                        }
                    });
                } else {
                    final JSONArray rect = message.getJSONArray("rect");
                    final double zoom = message.getDouble("zoom");
                    mMainHandler.post(new Runnable() {
                        public void run() {
                            
                            InputMethodManager imm =
                                (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                            if (!imm.isFullscreenMode())
                                mAutoCompletePopup.show(suggestions, rect, zoom);
                        }
                    });
                }
            } else if (event.equals("Permissions:Data")) {
                String host = message.getString("host");
                JSONArray permissions = message.getJSONArray("permissions");
                showSiteSettingsDialog(host, permissions);
            } else if (event.equals("Downloads:Done")) {
                String displayName = message.getString("displayName");
                String path = message.getString("path");
                String mimeType = message.getString("mimeType");
                int size = message.getInt("size");

                handleDownloadDone(displayName, path, mimeType, size);
            } else if (event.equals("CharEncoding:Data")) {
                final JSONArray charsets = message.getJSONArray("charsets");
                int selected = message.getInt("selected");

                final int len = charsets.length();
                final String[] titleArray = new String[len];
                for (int i = 0; i < len; i++) {
                    JSONObject charset = charsets.getJSONObject(i);
                    titleArray[i] = charset.getString("title");
                }

                final AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);
                dialogBuilder.setSingleChoiceItems(titleArray, selected, new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        try {
                            JSONObject charset = charsets.getJSONObject(which);
                            GeckoAppShell.sendEventToGecko(new GeckoEvent("CharEncoding:Set", charset.getString("code")));
                            dialog.dismiss();
                        } catch (JSONException e) {
                            Log.e(LOGTAG, "error parsing json", e);
                        }
                    }
                });
                dialogBuilder.setNegativeButton(R.string.button_cancel, new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
                mMainHandler.post(new Runnable() {
                    public void run() {
                        dialogBuilder.show();
                    }
                });
            } else if (event.equals("CharEncoding:State")) {
                final boolean visible = message.getString("visible").equals("true");
                GeckoPreferences.setCharEncodingState(visible);
                if (sMenu != null) {
                    mMainHandler.post(new Runnable() {
                        public void run() {
                            sMenu.findItem(R.id.char_encoding).setVisible(visible);
                        }
                    });
                }
            } else if (event.equals("Update:Restart")) {
                doRestart("org.mozilla.gecko.restart_update");
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public void showAboutHome() {
        Runnable r = new AboutHomeRunnable(true);
        mMainHandler.postAtFrontOfQueue(r);
    }

    public void hideAboutHome() {
        Runnable r = new AboutHomeRunnable(false);
        mMainHandler.postAtFrontOfQueue(r);
    }

    public class AboutHomeRunnable implements Runnable {
        boolean mShow;
        AboutHomeRunnable(boolean show) {
            mShow = show;
        }

        public void run() {
            mAutoCompletePopup.hide();
            if (mAboutHomeContent == null && mShow) {
                mAboutHomeContent = new AboutHomeContent(GeckoApp.mAppContext);
                mAboutHomeContent.update(GeckoApp.mAppContext, AboutHomeContent.UpdateFlags.ALL);
                mAboutHomeContent.setUriLoadCallback(new AboutHomeContent.UriLoadCallback() {
                    public void callback(String url) {
                        mBrowserToolbar.setProgressVisibility(true);
                        loadUrl(url, AwesomeBar.Type.EDIT);
                    }
                });
                RelativeLayout.LayoutParams lp = 
                    new RelativeLayout.LayoutParams(LayoutParams.FILL_PARENT, 
                                                    LayoutParams.FILL_PARENT);
                mGeckoLayout.addView(mAboutHomeContent, lp);
            } else if (mAboutHomeContent != null && mShow) {
                mAboutHomeContent.update(GeckoApp.mAppContext,
                                         EnumSet.of(AboutHomeContent.UpdateFlags.TOP_SITES));
            }

            if (mAboutHomeContent != null)
                mAboutHomeContent.setVisibility(mShow ? View.VISIBLE : View.GONE);
        }
    }

    




    private void showSiteSettingsDialog(String aHost, JSONArray aPermissions) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);

        View customTitleView = getLayoutInflater().inflate(R.layout.site_setting_title, null);
        ((TextView) customTitleView.findViewById(R.id.title)).setText(R.string.site_settings_title);
        ((TextView) customTitleView.findViewById(R.id.host)).setText(aHost);        
        builder.setCustomTitle(customTitleView);

        
        
        if (aPermissions.length() == 0) {
            builder.setMessage(R.string.site_settings_no_settings);
        } else {
            
            
            CharSequence[] items = new CharSequence[aPermissions.length()];
            boolean[] states = new boolean[aPermissions.length()];
            for (int i = 0; i < aPermissions.length(); i++) {
                try {
                    items[i] = aPermissions.getJSONObject(i).
                               getString("setting");
                    
                    states[i] = true;
                } catch (JSONException e) {
                    Log.i(LOGTAG, "JSONException: " + e);
                }
            }
            builder.setMultiChoiceItems(items, states, new DialogInterface.OnMultiChoiceClickListener(){
                public void onClick(DialogInterface dialog, int item, boolean state) {
                    
                }
            });
            builder.setPositiveButton(R.string.site_settings_clear, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    ListView listView = ((AlertDialog) dialog).getListView();
                    SparseBooleanArray checkedItemPositions = listView.getCheckedItemPositions();

                    
                    JSONArray permissionsToClear = new JSONArray();
                    for (int i = 0; i < checkedItemPositions.size(); i++) {
                        boolean checked = checkedItemPositions.get(i);
                        if (checked)
                            permissionsToClear.put(i);
                    }
                    GeckoAppShell.sendEventToGecko(new GeckoEvent("Permissions:Clear", permissionsToClear.toString()));
                }
            });
        }

        builder.setNegativeButton(R.string.site_settings_cancel, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }            
        });

        mMainHandler.post(new Runnable() {
            public void run() {
                builder.create().show();
            }
        });
    }

    void handleDoorHanger(JSONObject geckoObject) throws JSONException {
        final String message = geckoObject.getString("message");
        final String value = geckoObject.getString("value");
        final JSONArray buttons = geckoObject.getJSONArray("buttons");
        final int tabId = geckoObject.getInt("tabID");
        final JSONObject options = geckoObject.getJSONObject("options");

        Log.i(LOGTAG, "DoorHanger received for tab " + tabId + ", msg:" + message);

        mMainHandler.post(new Runnable() {
            public void run() {
                Tab tab = Tabs.getInstance().getTab(tabId);
                mDoorHangerPopup.addDoorHanger(message, value, buttons,
                                                           tab, options);
            }
        });
    }

    void handleDoorHangerRemove(JSONObject geckoObject) throws JSONException {
        final String value = geckoObject.getString("value");
        final int tabId = geckoObject.getInt("tabID");

        Log.i(LOGTAG, "Doorhanger:Remove received for tab " + tabId);

        mMainHandler.post(new Runnable() {
            public void run() {
                Tab tab = Tabs.getInstance().getTab(tabId);
                if (tab == null)
                    return;
                tab.removeDoorHanger(value);
                mDoorHangerPopup.updatePopup();
            }
        });
    }

    void handleDocumentStart(int tabId, final boolean showProgress) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.setLoading(true);
        tab.updateSecurityMode("unknown");

        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    mBrowserToolbar.setSecurityMode(tab.getSecurityMode());
                    if (showProgress)
                        mBrowserToolbar.setProgressVisibility(true);
                }
                onTabsChanged(tab);
            }
        });
    }

    void handleDocumentStop(int tabId) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.setLoading(false);

        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setProgressVisibility(false);
                onTabsChanged(tab);
            }
        });

        Runnable r = new SessionSnapshotRunnable(tab);
        GeckoAppShell.getHandler().postDelayed(r, 500);
    }

    void handleShowToast(final String message, final String duration) {
        mMainHandler.post(new Runnable() {
            public void run() {
                Toast toast;
                if (duration.equals("long"))
                    toast = Toast.makeText(mAppContext, message, Toast.LENGTH_LONG);
                else
                    toast = Toast.makeText(mAppContext, message, Toast.LENGTH_SHORT);
                toast.show();
            }
        });
    }

    void handleContentLoaded(int tabId, String uri, String title) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.updateTitle(title);

        
        mMainHandler.post(new Runnable() {
            public void run() {
                loadFavicon(tab);

                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setTitle(tab.getDisplayTitle());

                onTabsChanged(tab);
            }
        });
    }

    void handleTitleChanged(int tabId, String title) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.updateTitle(title);

        mMainHandler.post(new Runnable() {
            public void run() {
                if (Tabs.getInstance().isSelectedTab(tab))
                    mBrowserToolbar.setTitle(tab.getDisplayTitle());
                onTabsChanged(tab);
            }
        });
    }

    void handleLinkAdded(final int tabId, String rel, final String href) {
        if (rel.indexOf("[icon]") != -1) {
            final Tab tab = Tabs.getInstance().getTab(tabId);
            if (tab != null) {
                tab.updateFaviconURL(href);

                
                
                
                
                if (!tab.isLoading()) {
                    mMainHandler.post(new Runnable() {
                        public void run() {
                            loadFavicon(tab);
                        }
                    });
                }
            }
        }
    }

    void handleWindowClose(final int tabId) {
        Tabs tabs = Tabs.getInstance();
        Tab tab = tabs.getTab(tabId);
        tabs.closeTab(tab);
    }

    void handleDownloadDone(String displayName, String path, String mimeType, int size) {
        
        if (Build.VERSION.SDK_INT >= 12) {
            DownloadManager dm = (DownloadManager) mAppContext.getSystemService(Context.DOWNLOAD_SERVICE);
            dm.addCompletedDownload(displayName, displayName,
                false ,
                mimeType, path, size,
                false );
        }
    }

    void addPluginView(final View view,
                       final int x, final int y,
                       final int w, final int h,
                       final String metadata) {
        mMainHandler.post(new Runnable() { 
            public void run() {
                PluginLayoutParams lp;
                JSONObject viewportObject;
                ViewportMetrics pluginViewport;

                ViewportMetrics targetViewport = mLayerController.getViewportMetrics();
                
                try {
                    viewportObject = new JSONObject(metadata);
                    pluginViewport = new ViewportMetrics(viewportObject);
                } catch (JSONException e) {
                    Log.e(LOGTAG, "Bad viewport metadata: ", e);
                    return;
                }

                if (mPluginContainer.indexOfChild(view) == -1) {
                    lp = new PluginLayoutParams(x, y, w, h, pluginViewport);

                    view.setWillNotDraw(false);
                    if (view instanceof SurfaceView) {
                        SurfaceView sview = (SurfaceView)view;

                        sview.setZOrderOnTop(false);
                        sview.setZOrderMediaOverlay(true);
                    }

                    mPluginContainer.addView(view, lp);
                    mPluginViews.add(view);
                } else {
                    lp = (PluginLayoutParams)view.getLayoutParams();
                    lp.reset(x, y, w, h, pluginViewport);
                    lp.reposition(targetViewport);
                    try {
                        mPluginContainer.updateViewLayout(view, lp);
                    } catch (IllegalArgumentException e) {
                        Log.i(LOGTAG, "e:" + e);
                        
                        
                        
                    }
                }
            }
        });
    }

    void removePluginView(final View view) {
        mMainHandler.post(new Runnable() { 
            public void run() {
                try {
                    mPluginContainer.removeView(view);
                    mPluginViews.remove(view);
                } catch (Exception e) {}
            }
        });
    }

    public void hidePluginViews() {
        for (View view : mPluginViews) {
            view.setVisibility(View.GONE);
        }
    }

    public void showPluginViews() {
        repositionPluginViews(true);
    }

    public void repositionPluginViews(boolean setVisible) {
        ViewportMetrics targetViewport = mLayerController.getViewportMetrics();

        if (targetViewport == null)
            return;

        for (View view : mPluginViews) {
            PluginLayoutParams lp = (PluginLayoutParams)view.getLayoutParams();
            lp.reposition(targetViewport);

            if (setVisible) {
                view.setVisibility(View.VISIBLE);
            }

            mPluginContainer.updateViewLayout(view, lp);
        }
    }

    public void setFullScreen(final boolean fullscreen) {
        mMainHandler.post(new Runnable() { 
            public void run() {
                
                getWindow().setFlags(fullscreen ?
                                     WindowManager.LayoutParams.FLAG_FULLSCREEN : 0,
                                     WindowManager.LayoutParams.FLAG_FULLSCREEN);
            }
        });
    }

    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        mAppContext = this;

        
        if (getResources().getBoolean(R.bool.enableStrictMode)) {
            enableStrictMode();
        }

        System.loadLibrary("mozglue");
        mMainHandler = new Handler();
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - onCreate");
        if (savedInstanceState != null) {
            mLastTitle = savedInstanceState.getString(SAVED_STATE_TITLE);
            mLastViewport = savedInstanceState.getString(SAVED_STATE_VIEWPORT);
            mLastScreen = savedInstanceState.getByteArray(SAVED_STATE_SCREEN);
            mRestoreSession = savedInstanceState.getBoolean(SAVED_STATE_SESSION);
        }

        Intent intent = getIntent();
        String args = intent.getStringExtra("args");
        if (args != null && args.contains("-profile")) {
            Pattern p = Pattern.compile("(?:-profile\\s*)(\\w*)(\\s*)");
            Matcher m = p.matcher(args);
            if (m.find()) {
                mProfileDir = new File(m.group(1));
                mLastTitle = null;
                mLastViewport = null;
                mLastScreen = null;
            }
        }

        if (ACTION_UPDATE.equals(intent.getAction()) || args != null && args.contains("-alert update-app")) {
            Log.i(LOGTAG,"onCreate: Update request");
            checkAndLaunchUpdate();
        }

        String passedUri = null;
        String uri = getURIFromIntent(intent);
        if (uri != null && uri.length() > 0)
            passedUri = mLastTitle = uri;

        if (passedUri == null || passedUri.equals("about:home")) {
            
            Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - start check sessionstore.bak exists");
            File profileDir = getProfileDir();
            boolean sessionExists = false;
            if (profileDir != null)
                sessionExists = new File(profileDir, "sessionstore.bak").exists();
            Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - finish check sessionstore.bak exists");
            if (!sessionExists)
                showAboutHome();
        }

        if (sGREDir == null)
            sGREDir = new File(this.getApplicationInfo().dataDir);

        Uri data = intent.getData();
        if (data != null && "http".equals(data.getScheme()) &&
            isHostOnPrefetchWhitelist(data.getHost())) {
            Intent copy = new Intent(intent);
            copy.setAction(ACTION_LOAD);
            GeckoAppShell.getHandler().post(new RedirectorRunnable(copy));
            
            
            
            intent.setAction(Intent.ACTION_MAIN);
            intent.setData(null);
            passedUri = null;
        }

        sGeckoThread = new GeckoThread(intent, passedUri, mRestoreSession);
        if (!ACTION_DEBUG.equals(intent.getAction()) &&
            checkAndSetLaunchState(LaunchState.Launching, LaunchState.Launched))
            sGeckoThread.start();

        super.onCreate(savedInstanceState);

        setContentView(R.layout.gecko_app);

        mOrientation = getResources().getConfiguration().orientation;

        if (Build.VERSION.SDK_INT >= 11) {
            mBrowserToolbar = (BrowserToolbar) getLayoutInflater().inflate(R.layout.browser_toolbar, null);

            GeckoActionBar.setBackgroundDrawable(this, getResources().getDrawable(R.drawable.gecko_actionbar_bg));
            GeckoActionBar.setDisplayOptions(this, ActionBar.DISPLAY_SHOW_CUSTOM, ActionBar.DISPLAY_SHOW_CUSTOM |
                                                                                  ActionBar.DISPLAY_SHOW_HOME |
                                                                                  ActionBar.DISPLAY_SHOW_TITLE |
                                                                                  ActionBar.DISPLAY_USE_LOGO);
            GeckoActionBar.setCustomView(this, mBrowserToolbar);
        } else {
            mBrowserToolbar = (BrowserToolbar) findViewById(R.id.browser_toolbar);
        }

        mBrowserToolbar.setTitle(mLastTitle);

        mFavicons = new Favicons(this);

        
        mGeckoLayout = (RelativeLayout) findViewById(R.id.gecko_layout);
        mMainLayout = (LinearLayout) findViewById(R.id.main_layout);

        mDoorHangerPopup = new DoorHangerPopup(this);
        mAutoCompletePopup = (AutoCompletePopup) findViewById(R.id.autocomplete_popup);

        Tabs tabs = Tabs.getInstance();
        Tab tab = tabs.getSelectedTab();
        if (tab != null) {
            mBrowserToolbar.setTitle(tab.getDisplayTitle());
            mBrowserToolbar.setFavicon(tab.getFavicon());
            mBrowserToolbar.setProgressVisibility(tab.isLoading());
            mBrowserToolbar.updateTabs(Tabs.getInstance().getCount()); 
        }

        tabs.setContentResolver(getContentResolver()); 

        if (cameraView == null) {
            cameraView = new SurfaceView(this);
            cameraView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }

        if (mLayerController == null) {
            



            mSoftwareLayerClient = new GeckoSoftwareLayerClient(this);

            








            mLayerController = new LayerController(this);
            mPlaceholderLayerClient = PlaceholderLayerClient.createInstance(this);
            mLayerController.setLayerClient(mPlaceholderLayerClient);

            mGeckoLayout.addView(mLayerController.getView(), 0);
        }

        mPluginContainer = (AbsoluteLayout) findViewById(R.id.plugin_container);

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - UI almost up");

        if (!sTryCatchAttached) {
            sTryCatchAttached = true;
            mMainHandler.post(new Runnable() {
                public void run() {
                    try {
                        Looper.loop();
                    } catch (Exception e) {
                        GeckoAppShell.reportJavaCrash(e);
                    }
                    
                    sTryCatchAttached = false;
                }
            });
        }

        
        GeckoAppShell.registerGeckoEventListener("DOMContentLoaded", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("DOMTitleChanged", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("DOMLinkAdded", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("DOMWindowClose", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("log", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Content:LocationChange", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Content:SecurityChange", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Content:StateChange", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Content:LoadError", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("onCameraCapture", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Doorhanger:Add", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Doorhanger:Remove", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Menu:Add", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Menu:Remove", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Gecko:Ready", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Toast:Show", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("DOMFullScreen:Start", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("DOMFullScreen:Stop", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("ToggleChrome:Hide", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("ToggleChrome:Show", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("FormAssist:AutoComplete", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Permissions:Data", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Downloads:Done", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("CharEncoding:Data", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("CharEncoding:State", GeckoApp.mAppContext);
        GeckoAppShell.registerGeckoEventListener("Update:Restart", GeckoApp.mAppContext);

        mConnectivityFilter = new IntentFilter();
        mConnectivityFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        mConnectivityReceiver = new GeckoConnectivityReceiver();

        IntentFilter batteryFilter = new IntentFilter();
        batteryFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
        mBatteryReceiver = new GeckoBatteryManager();
        registerReceiver(mBatteryReceiver, batteryFilter);

        if (SmsManager.getInstance() != null) {
          SmsManager.getInstance().start();
        }

        GeckoNetworkManager.getInstance().init();

        final GeckoApp self = this;

        GeckoAppShell.getHandler().postDelayed(new Runnable() {
            public void run() {
                Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - pre checkLaunchState");

                








                if (!checkLaunchState(LaunchState.Launched)) {
                    return;
                }

                checkMigrateProfile();
            }
        }, 50);
    }

    



    private void enableStrictMode()
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.GINGERBREAD) {
            return;
        }

        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                                  .detectAll()
                                  .penaltyLog()
                                  .build());

        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                               .detectAll()
                               .penaltyLog()
                               .build());
    }

    public void enableCameraView() {
        
        mMainLayout.addView(cameraView, new AbsoluteLayout.LayoutParams(8, 16, 0, 0));
    }

    public void disableCameraView() {
        mMainLayout.removeView(cameraView);
    }

    abstract public String getDefaultUAString();
    abstract public String getUAStringForHost(String host);

    class RedirectorRunnable implements Runnable {
        Intent mIntent;
        RedirectorRunnable(Intent intent) {
            mIntent = intent;
        }
        public void run() {
            HttpURLConnection connection = null;
            try {
                
                URL url = new URL(mIntent.getData().toString());
                
                connection = (HttpURLConnection) url.openConnection();
                connection.setRequestProperty("User-Agent", getUAStringForHost(url.getHost()));
                connection.setInstanceFollowRedirects(false);
                connection.setRequestMethod("GET");
                connection.connect();
                int code = connection.getResponseCode();
                if (code >= 300 && code < 400) {
                    String location = connection.getHeaderField("Location");
                    Uri data;
                    if (location != null &&
                        (data = Uri.parse(location)) != null &&
                        !"about".equals(data.getScheme()) && 
                        !"chrome".equals(data.getScheme())) {
                        mIntent.setData(data);
                        mLastTitle = location;
                    } else {
                        mIntent.putExtra("prefetched", 1);
                    }
                } else {
                    mIntent.putExtra("prefetched", 1);
                }
            } catch (IOException ioe) {
                Log.i(LOGTAG, "exception trying to pre-fetch redirected url", ioe);
                mIntent.putExtra("prefetched", 1);
            } catch (Exception e) {
                Log.w(LOGTAG, "unexpected exception, passing url directly to Gecko but we should explicitly catch this", e);
                mIntent.putExtra("prefetched", 1);
            } finally {
                if (connection != null)
                    connection.disconnect();
            }
            mMainHandler.postAtFrontOfQueue(new Runnable() {
                public void run() {
                    onNewIntent(mIntent);
                }
            });
        }
    }

    private final String kPrefetchWhiteListArray[] = new String[] { 
        "t.co",
        "bit.ly",
        "moz.la",
        "aje.me",
        "facebook.com",
        "goo.gl",
        "tinyurl.com"
    };
    
    private final CopyOnWriteArrayList<String> kPrefetchWhiteList =
        new CopyOnWriteArrayList<String>(kPrefetchWhiteListArray);

    private boolean isHostOnPrefetchWhitelist(String host) {
        return kPrefetchWhiteList.contains(host);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - onNewIntent");

        if (checkLaunchState(LaunchState.GeckoExiting)) {
            
            
            System.exit(0);
            return;
        }

        if (checkLaunchState(LaunchState.Launched)) {
            Uri data = intent.getData();
            Bundle bundle = intent.getExtras();
            
            
            
            
            if (data != null && 
                "http".equals(data.getScheme()) &&
                (bundle == null || bundle.getInt("prefetched", 0) != 1) &&
                isHostOnPrefetchWhitelist(data.getHost())) {
                GeckoAppShell.getHandler().post(new RedirectorRunnable(intent));
                return;
            }
        }
        final String action = intent.getAction();
        if (ACTION_DEBUG.equals(action) &&
            checkAndSetLaunchState(LaunchState.Launching, LaunchState.WaitForDebugger)) {
            mMainHandler.postDelayed(new Runnable() {
                public void run() {
                    Log.i(LOGTAG, "Launching from debug intent after 5s wait");
                    setLaunchState(LaunchState.Launching);
                    sGeckoThread.start();
                }
            }, 1000 * 5 );
            Log.i(LOGTAG, "Intent : ACTION_DEBUG - waiting 5s before launching");
            return;
        }
        if (checkLaunchState(LaunchState.WaitForDebugger) || intent == getIntent())
            return;

        if (Intent.ACTION_MAIN.equals(action)) {
            Log.i(LOGTAG, "Intent : ACTION_MAIN");
            GeckoAppShell.sendEventToGecko(new GeckoEvent(""));
        }
        else if (ACTION_LOAD.equals(action)) {
            String uri = intent.getDataString();
            loadUrl(uri, AwesomeBar.Type.EDIT);
            Log.i(LOGTAG,"onNewIntent: " + uri);
        }
        else if (Intent.ACTION_VIEW.equals(action)) {
            String uri = intent.getDataString();
            GeckoAppShell.sendEventToGecko(new GeckoEvent(uri));
            Log.i(LOGTAG,"onNewIntent: " + uri);
        }
        else if (ACTION_WEBAPP.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(new GeckoEvent(uri));
            Log.i(LOGTAG,"Intent : WEBAPP - " + uri);
        }
        else if (ACTION_BOOKMARK.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(new GeckoEvent(uri));
            Log.i(LOGTAG,"Intent : BOOKMARK - " + uri);
        }
    }

    



    private String getURIFromIntent(Intent intent) {
        String uri = intent.getDataString();
        if (uri != null)
            return uri;

        final String action = intent.getAction();
        if (ACTION_WEBAPP.equals(action) || ACTION_BOOKMARK.equals(action)) {
            uri = intent.getStringExtra("args");
            if (uri != null && uri.startsWith("--url=")) {
                uri.replace("--url=", "");
            }
        }
        return uri;
    }

    @Override
    public void onPause()
    {
        Log.i(LOGTAG, "pause");

        Runnable r = new SessionSnapshotRunnable(null);
        GeckoAppShell.getHandler().post(r);

        GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_PAUSING));
        
        
        

        
        

        
        super.onPause();

        unregisterReceiver(mConnectivityReceiver);
        GeckoNetworkManager.getInstance().stop();
    }

    @Override
    public void onResume()
    {
        Log.i(LOGTAG, "resume");
        if (checkLaunchState(LaunchState.GeckoRunning))
            GeckoAppShell.onResume();
        
        
        super.onResume();

        
        if (checkLaunchState(LaunchState.Launching))
            onNewIntent(getIntent());

        registerReceiver(mConnectivityReceiver, mConnectivityFilter);
        GeckoNetworkManager.getInstance().start();

        if (mOwnActivityDepth > 0)
            mOwnActivityDepth--;
    }

    @Override
    public void onStop()
    {
        Log.i(LOGTAG, "stop");
        
        
        
        
        
        
        
        
        
        

        GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_STOPPING));
        super.onStop();
    }

    @Override
    public void onRestart()
    {
        Log.i(LOGTAG, "restart");
        super.onRestart();
    }

    @Override
    public void onStart()
    {
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - onStart");

        Log.i(LOGTAG, "start");
        GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_START));
        super.onStart();
    }

    @Override
    public void onDestroy()
    {
        Log.i(LOGTAG, "destroy");

        
        
        if (isFinishing())
            GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_SHUTDOWN));
        
        GeckoAppShell.unregisterGeckoEventListener("DOMContentLoaded", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("DOMTitleChanged", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("DOMLinkAdded", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("DOMWindowClose", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("log", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Content:LocationChange", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Content:SecurityChange", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Content:StateChange", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Content:LoadError", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("onCameraCapture", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Doorhanger:Add", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Menu:Add", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Menu:Remove", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Gecko:Ready", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Toast:Show", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("ToggleChrome:Hide", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("ToggleChrome:Show", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("FormAssist:AutoComplete", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Permissions:Data", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("Downloads:Done", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("CharEncoding:Data", GeckoApp.mAppContext);
        GeckoAppShell.unregisterGeckoEventListener("CharEncoding:State", GeckoApp.mAppContext);

        mFavicons.close();

        if (SmsManager.getInstance() != null) {
            SmsManager.getInstance().stop();
            if (isFinishing())
                SmsManager.getInstance().shutdown();
        }

        GeckoNetworkManager.getInstance().stop();

        super.onDestroy();

        unregisterReceiver(mBatteryReceiver);
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
        if (mAboutHomeContent != null)
            mAboutHomeContent.onActivityContentChanged(this);
    }


    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        Log.i(LOGTAG, "configuration changed");

        super.onConfigurationChanged(newConfig);

        if (mOrientation != newConfig.orientation) {
            mOrientation = newConfig.orientation;
            mAutoCompletePopup.hide();

            if (Build.VERSION.SDK_INT >= 11) {
                mBrowserToolbar = (BrowserToolbar) getLayoutInflater().inflate(R.layout.browser_toolbar, null);

                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    mBrowserToolbar.setTitle(tab.getDisplayTitle());
                    mBrowserToolbar.setFavicon(tab.getFavicon());
                    mBrowserToolbar.setSecurityMode(tab.getSecurityMode());
                    mBrowserToolbar.setProgressVisibility(tab.isLoading());
                    mBrowserToolbar.setShadowVisibility(!(tab.getURL().startsWith("about:")));
                    mBrowserToolbar.updateTabs(Tabs.getInstance().getCount());
                }

                GeckoActionBar.setBackgroundDrawable(this, getResources().getDrawable(R.drawable.gecko_actionbar_bg));
                GeckoActionBar.setCustomView(mAppContext, mBrowserToolbar);
            }
        }
    }

    @Override
    public void onLowMemory()
    {
        Log.e(LOGTAG, "low memory");
        if (checkLaunchState(LaunchState.GeckoRunning))
            GeckoAppShell.onLowMemory();
        super.onLowMemory();
    }

    abstract public String getPackageName();
    abstract public String getContentProcessName();

    public void addEnvToIntent(Intent intent) {
        Map<String,String> envMap = System.getenv();
        Set<Map.Entry<String,String>> envSet = envMap.entrySet();
        Iterator<Map.Entry<String,String>> envIter = envSet.iterator();
        int c = 0;
        while (envIter.hasNext()) {
            Map.Entry<String,String> entry = envIter.next();
            intent.putExtra("env" + c, entry.getKey() + "="
                            + entry.getValue());
            c++;
        }
    }

    public void doRestart() {
        doRestart("org.mozilla.gecko.restart");
    }

    public void doRestart(String action) {
        Log.i(LOGTAG, "doRestart(\"" + action + "\")");
        try {
            Intent intent = new Intent(action);
            intent.setClassName(getPackageName(),
                                getPackageName() + ".Restarter");
            
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
                            Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            Log.i(LOGTAG, intent.toString());
            GeckoAppShell.killAnyZombies();
            startActivity(intent);
        } catch (Exception e) {
            Log.i(LOGTAG, "error doing restart", e);
        }
        finish();
        
        GeckoAppShell.waitForAnotherGeckoProc();
    }

    public void handleNotification(String action, String alertName, String alertCookie) {
        GeckoAppShell.handleNotification(action, alertName, alertCookie);
    }

    private void checkAndLaunchUpdate() {
        Log.i(LOGTAG, "Checking for an update");

        int statusCode = 8; 
        File baseUpdateDir = null;
        if (Build.VERSION.SDK_INT >= 8)
            baseUpdateDir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        else
            baseUpdateDir = new File(Environment.getExternalStorageDirectory().getPath(), "download");

        File updateDir = new File(new File(baseUpdateDir, "updates"),"0");

        File updateFile = new File(updateDir, "update.apk");
        File statusFile = new File(updateDir, "update.status");

        if (!statusFile.exists() || !readUpdateStatus(statusFile).equals("pending"))
            return;

        if (!updateFile.exists())
            return;

        Log.i(LOGTAG, "Update is available!");

        
        File updateFileToRun = new File(updateDir, getPackageName() + "-update.apk");
        try {
            if (updateFile.renameTo(updateFileToRun)) {
                String amCmd = "/system/bin/am start -a android.intent.action.VIEW " +
                               "-n com.android.packageinstaller/.PackageInstallerActivity -d file://" +
                               updateFileToRun.getPath();
                Log.i(LOGTAG, amCmd);
                Runtime.getRuntime().exec(amCmd);
                statusCode = 0; 
            } else {
                Log.i(LOGTAG, "Cannot rename the update file!");
                statusCode = 7; 
            }
        } catch (Exception e) {
            Log.i(LOGTAG, "error launching installer to update", e);
        }

        
        String status = statusCode == 0 ? "succeeded\n" : "failed: "+ statusCode + "\n";

        OutputStream outStream;
        try {
            byte[] buf = status.getBytes("UTF-8");
            outStream = new FileOutputStream(statusFile);
            outStream.write(buf, 0, buf.length);
            outStream.close();
        } catch (Exception e) {
            Log.i(LOGTAG, "error writing status file", e);
        }

        if (statusCode == 0)
            System.exit(0);
    }

    private String readUpdateStatus(File statusFile) {
        String status = "";
        try {
            BufferedReader reader = new BufferedReader(new FileReader(statusFile));
            status = reader.readLine();
            reader.close();
        } catch (Exception e) {
            Log.i(LOGTAG, "error reading update status", e);
        }
        return status;
    }

    private void checkMigrateProfile() {
        File profileDir = getProfileDir();
        if (profileDir != null) {
            long currentTime = SystemClock.uptimeMillis();
            Log.i(LOGTAG, "checking profile migration in: " + profileDir.getAbsolutePath());
            final GeckoApp app = GeckoApp.mAppContext;
            final SetupScreen setupScreen = new SetupScreen(app);
            
            setupScreen.showDelayed(mMainHandler);
            GeckoAppShell.ensureSQLiteLibsLoaded(app.getApplication().getPackageResourcePath());
            ProfileMigrator profileMigrator =
                new ProfileMigrator(app.getContentResolver(), profileDir);
            profileMigrator.launch();
            setupScreen.dismiss();
            long timeDiff = SystemClock.uptimeMillis() - currentTime;
            Log.i(LOGTAG, "Profile migration took " + timeDiff + " ms");
        }
    }

    private SynchronousQueue<String> mFilePickerResult = new SynchronousQueue<String>();
    public String showFilePicker(String aMimeType) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(aMimeType);
        GeckoApp.this.
            startActivityForResult(
                Intent.createChooser(intent, getString(R.string.choose_file)),
                FILE_PICKER_REQUEST);
        String filePickerResult = "";

        try {
            while (null == (filePickerResult = mFilePickerResult.poll(1, TimeUnit.MILLISECONDS))) {
                Log.i(LOGTAG, "processing events from showFilePicker ");
                GeckoAppShell.processNextNativeEvent();
            }
        } catch (InterruptedException e) {
            Log.i(LOGTAG, "showing file picker ",  e);
        }

        return filePickerResult;
    }

    @Override
    public boolean onSearchRequested() {
        return showAwesomebar(AwesomeBar.Type.ADD);
    }
 
    public boolean onEditRequested() {
        return showAwesomebar(AwesomeBar.Type.EDIT);
    }

    public boolean showAwesomebar(AwesomeBar.Type aType) {
        Intent intent = new Intent(getBaseContext(), AwesomeBar.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_NO_HISTORY);
        intent.putExtra(AwesomeBar.TYPE_KEY, aType.name());

        if (aType != AwesomeBar.Type.ADD) {
            
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {
                Tab.HistoryEntry he = tab.getLastHistoryEntry();
                if (he != null) {
                    intent.putExtra(AwesomeBar.CURRENT_URL_KEY, he.mUri);
                }
            }
        }
        mOwnActivityDepth++;
        startActivityForResult(intent, AWESOMEBAR_REQUEST);
        return true;
    }

    public boolean doReload() {
        Log.i(LOGTAG, "Reload requested");
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null)
            return false;

        return tab.doReload();
    }

    public boolean doForward() {
        Log.i(LOGTAG, "Forward requested");
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null)
            return false;

        return tab.doForward();
    }

    public boolean doStop() {
        Log.i(LOGTAG, "Stop requested");
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null)
            return false;

        return tab.doStop();
    }

    @Override
    public void onBackPressed() {
        if (mDoorHangerPopup.isShowing()) {
            mDoorHangerPopup.dismiss();
            return;
        }

        if (mDOMFullScreen) {
            GeckoAppShell.sendEventToGecko(new GeckoEvent("FullScreen:Exit", null));
            return;
        }

        Tabs tabs = Tabs.getInstance();
        Tab tab = tabs.getSelectedTab();
        if (tab == null) {
            moveTaskToBack(true);
            return;
        }

        if (tab.doBack())
            return;

        if (tab.isExternal()) {
            moveTaskToBack(true);
            tabs.closeTab(tab);
            return;
        }

        int parentId = tab.getParentId();
        Tab parent = tabs.getTab(parentId);
        if (parent != null) {
            
            tabs.closeTab(tab, parent);
            return;
        }

        moveTaskToBack(true);
    }

    static int kCaptureIndex = 0;

    @Override
    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
        case FILE_PICKER_REQUEST:
            String filePickerResult = "";
            if (data != null && resultCode == RESULT_OK) {
                try {
                    ContentResolver cr = getContentResolver();
                    Uri uri = data.getData();
                    Cursor cursor = GeckoApp.mAppContext.getContentResolver().query(
                        uri, 
                        new String[] { OpenableColumns.DISPLAY_NAME },
                        null, 
                        null, 
                        null);
                    String name = null;
                    if (cursor != null) {
                        try {
                            if (cursor.moveToNext()) {
                                name = cursor.getString(0);
                            }
                        } finally {
                            cursor.close();
                        }
                    }
                    String fileName = "tmp_";
                    String fileExt = null;
                    int period;
                    if (name == null || (period = name.lastIndexOf('.')) == -1) {
                        String mimeType = cr.getType(uri);
                        fileExt = "." + GeckoAppShell.getExtensionFromMimeType(mimeType);
                    } else {
                        fileExt = name.substring(period);
                        fileName = name.substring(0, period);
                    }
                    File file = File.createTempFile(fileName, fileExt, sGREDir);

                    FileOutputStream fos = new FileOutputStream(file);
                    InputStream is = cr.openInputStream(uri);
                    byte[] buf = new byte[4096];
                    int len = is.read(buf);
                    while (len != -1) {
                        fos.write(buf, 0, len);
                        len = is.read(buf);
                    }
                    fos.close();
                    filePickerResult =  file.getAbsolutePath();
                }catch (Exception e) {
                    Log.e(LOGTAG, "showing file picker", e);
                }
            }
            try {
                mFilePickerResult.put(filePickerResult);
            } catch (InterruptedException e) {
                Log.i(LOGTAG, "error returning file picker result", e);
            }
            break;
        case AWESOMEBAR_REQUEST:
            if (data != null) {
                String url = data.getStringExtra(AwesomeBar.URL_KEY);
                AwesomeBar.Type type = AwesomeBar.Type.valueOf(data.getStringExtra(AwesomeBar.TYPE_KEY));
                String searchEngine = data.getStringExtra(AwesomeBar.SEARCH_KEY);
                if (url != null && url.length() > 0)
                    loadRequest(url, type, searchEngine);
            }
            break;
        case CAMERA_CAPTURE_REQUEST:
            Log.i(LOGTAG, "Returning from CAMERA_CAPTURE_REQUEST: " + resultCode);
            File file = new File(Environment.getExternalStorageDirectory(), "cameraCapture-" + Integer.toString(kCaptureIndex) + ".jpg");
            kCaptureIndex++;
            GeckoEvent e = new GeckoEvent("cameraCaptureDone", resultCode == Activity.RESULT_OK ?
                                          "{\"ok\": true,  \"path\": \"" + file.getPath() + "\" }" :
                                          "{\"ok\": false, \"path\": \"" + file.getPath() + "\" }");
            GeckoAppShell.sendEventToGecko(e);
            break;
       }
    }

    public void doCameraCapture() {
        File file = new File(Environment.getExternalStorageDirectory(), "cameraCapture-" + Integer.toString(kCaptureIndex) + ".jpg");

        Intent intent = new Intent(android.provider.MediaStore.ACTION_IMAGE_CAPTURE);
        intent.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(file));

        startActivityForResult(intent, CAMERA_CAPTURE_REQUEST);
    }

    
    
    private void loadRequest(String url, AwesomeBar.Type type, String searchEngine) {
        mBrowserToolbar.setTitle(url);
        Log.d(LOGTAG, type.name());
        JSONObject args = new JSONObject();
        try {
            args.put("url", url);
            args.put("engine", searchEngine);
        } catch (Exception e) {
            Log.e(LOGTAG, "error building JSON arguments");
        }
        if (type == AwesomeBar.Type.ADD) {
            Log.i(LOGTAG, "Sending message to Gecko: " + SystemClock.uptimeMillis() + " - Tab:Add");
            GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Add", args.toString()));
        } else {
            GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Load", args.toString()));
        }
    }

    public void loadUrl(String url, AwesomeBar.Type type) {
        loadRequest(url, type, null);
    }

    





    public void loadUrlInTab(String url) {
        ArrayList<Tab> tabs = Tabs.getInstance().getTabsInOrder();
        if (tabs != null) {
            Iterator<Tab> tabsIter = tabs.iterator();
            while (tabsIter.hasNext()) {
                Tab tab = tabsIter.next();
                if (url.equals(tab.getURL())) {
                    Tabs.getInstance().selectTab(tab.getId());
                    return;
                }
            }
        }

        JSONObject args = new JSONObject();
        try {
            args.put("url", url);
            args.put("parentId", Tabs.getInstance().getSelectedTab().getId());
        } catch (Exception e) {
            Log.e(LOGTAG, "error building JSON arguments");
        }
        Log.i(LOGTAG, "Sending message to Gecko: " + SystemClock.uptimeMillis() + " - Tab:Add");
        GeckoAppShell.sendEventToGecko(new GeckoEvent("Tab:Add", args.toString()));
    }

    public GeckoSoftwareLayerClient getSoftwareLayerClient() { return mSoftwareLayerClient; }
    public LayerController getLayerController() { return mLayerController; }

    
    public void onAccuracyChanged(Sensor sensor, int accuracy)
    {
    }

    public void onSensorChanged(SensorEvent event)
    {
        Log.w(LOGTAG, "onSensorChanged "+event);
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
    }

    private class GeocoderRunnable implements Runnable {
        Location mLocation;
        GeocoderRunnable (Location location) {
            mLocation = location;
        }
        public void run() {
            try {
                List<Address> addresses = mGeocoder.getFromLocation(mLocation.getLatitude(),
                                                                    mLocation.getLongitude(), 1);
                
                
                
                mLastGeoAddress = addresses.get(0);
                GeckoAppShell.sendEventToGecko(new GeckoEvent(mLocation, mLastGeoAddress));
            } catch (Exception e) {
                Log.w(LOGTAG, "GeocoderTask "+e);
            }
        }
    }

    
    public void onLocationChanged(Location location)
    {
        Log.w(LOGTAG, "onLocationChanged "+location);
        if (mGeocoder == null)
            mGeocoder = new Geocoder(mLayerController.getView().getContext(), Locale.getDefault());

        if (mLastGeoAddress == null) {
            GeckoAppShell.getHandler().post(new GeocoderRunnable(location));
        }
        else {
            float[] results = new float[1];
            Location.distanceBetween(location.getLatitude(),
                                     location.getLongitude(),
                                     mLastGeoAddress.getLatitude(),
                                     mLastGeoAddress.getLongitude(),
                                     results);
            
            
            
            if (results[0] > 100)
                GeckoAppShell.getHandler().post(new GeocoderRunnable(location));
        }

        GeckoAppShell.sendEventToGecko(new GeckoEvent(location, mLastGeoAddress));
    }

    public void onProviderDisabled(String provider)
    {
    }

    public void onProviderEnabled(String provider)
    {
    }

    public void onStatusChanged(String provider, int status, Bundle extras)
    {
    }


    private void connectGeckoLayerClient() {
        if (mPlaceholderLayerClient != null)
            mPlaceholderLayerClient.destroy();

        LayerController layerController = getLayerController();
        layerController.setLayerClient(mSoftwareLayerClient);
    }

}

class PluginLayoutParams extends AbsoluteLayout.LayoutParams
{
    private static final int MAX_DIMENSION = 2048;
    private static final String LOGTAG = "GeckoApp.PluginLayoutParams";

    private int mOriginalX;
    private int mOriginalY;
    private int mOriginalWidth;
    private int mOriginalHeight;
    private ViewportMetrics mOriginalViewport;
    private float mLastResolution;

    public PluginLayoutParams(int aX, int aY, int aWidth, int aHeight, ViewportMetrics aViewport) {
        super(aWidth, aHeight, aX, aY);

        Log.i(LOGTAG, "Creating plugin at " + aX + ", " + aY + ", " + aWidth + "x" + aHeight + ", (" + (aViewport.getZoomFactor() * 100) + "%)");

        mOriginalX = aX;
        mOriginalY = aY;
        mOriginalWidth = aWidth;
        mOriginalHeight = aHeight;
        mOriginalViewport = aViewport;
        mLastResolution = aViewport.getZoomFactor();

        clampToMaxSize();
    }

    private void clampToMaxSize() {
        if (width > MAX_DIMENSION || height > MAX_DIMENSION) {
            if (width > height) {
                height = (int)(((float)height/(float)width) * MAX_DIMENSION);
                width = MAX_DIMENSION;
            } else {
                width = (int)(((float)width/(float)height) * MAX_DIMENSION);
                height = MAX_DIMENSION;
            }
        }
    }

    public void reset(int aX, int aY, int aWidth, int aHeight, ViewportMetrics aViewport) {
        x = mOriginalX = aX;
        y = mOriginalY = aY;
        width = mOriginalWidth = aWidth;
        height = mOriginalHeight = aHeight;
        mOriginalViewport = aViewport;
        mLastResolution = aViewport.getZoomFactor();

        clampToMaxSize();
    }

    private void reposition(Point aOffset, float aResolution) {
        x = mOriginalX + aOffset.x;
        y = mOriginalY + aOffset.y;

        if (!FloatUtils.fuzzyEquals(mLastResolution, aResolution)) {
            width = Math.round(aResolution * mOriginalWidth);
            height = Math.round(aResolution * mOriginalHeight);
            mLastResolution = aResolution;

            clampToMaxSize();
        }
    }

    public void reposition(ViewportMetrics viewport) {
        PointF targetOrigin = viewport.getDisplayportOrigin();
        PointF originalOrigin = mOriginalViewport.getDisplayportOrigin();

        Point offset = new Point(Math.round(originalOrigin.x - targetOrigin.x),
                                 Math.round(originalOrigin.y - targetOrigin.y));

        reposition(offset, viewport.getZoomFactor());
    }
}
