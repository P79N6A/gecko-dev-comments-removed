




package org.mozilla.gecko;

import org.mozilla.gecko.background.announcements.AnnouncementsBroadcastService;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PanZoomController;
import org.mozilla.gecko.gfx.PluginLayer;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.updater.UpdateService;
import org.mozilla.gecko.updater.UpdateServiceHelper;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.GeckoEventResponder;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UiAsyncTask;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.WallpaperManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.Signature;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.location.Location;
import android.location.LocationListener;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.StrictMode;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Base64;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.Display;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsoluteLayout;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

abstract public class GeckoApp
                extends GeckoActivity 
                implements GeckoEventListener, SensorEventListener, LocationListener,
                           Tabs.OnTabsChangedListener, GeckoEventResponder,
                           GeckoMenu.Callback, GeckoMenu.MenuPresenter,
                           TouchEventInterceptor
{
    private static final String LOGTAG = "GeckoApp";

    public static enum StartupMode {
        NORMAL,
        NEW_VERSION,
        NEW_PROFILE
    }

    private static enum StartupAction {
        NORMAL,     
        URL,        
        PREFETCH    
    }

    public static final String ACTION_ALERT_CALLBACK = "org.mozilla.gecko.ACTION_ALERT_CALLBACK";
    public static final String ACTION_WEBAPP_PREFIX = "org.mozilla.gecko.WEBAPP";
    public static final String ACTION_DEBUG         = "org.mozilla.gecko.DEBUG";
    public static final String ACTION_BOOKMARK      = "org.mozilla.gecko.BOOKMARK";
    public static final String ACTION_LOAD          = "org.mozilla.gecko.LOAD";
    public static final String ACTION_INIT_PW       = "org.mozilla.gecko.INIT_PW";
    public static final String SAVED_STATE_IN_BACKGROUND = "inBackground";
    public static final String SAVED_STATE_PRIVATE_SESSION = "privateSession";

    public static final String PREFS_NAME          = "GeckoApp";
    public static final String PREFS_OOM_EXCEPTION = "OOMException";
    public static final String PREFS_WAS_STOPPED   = "wasStopped";
    public static final String PREFS_CRASHED       = "crashed";

    static public final int RESTORE_NONE = 0;
    static public final int RESTORE_OOM = 1;
    static public final int RESTORE_CRASH = 2;

    StartupMode mStartupMode = null;
    protected RelativeLayout mMainLayout;
    protected RelativeLayout mGeckoLayout;
    public View getView() { return mGeckoLayout; }
    public SurfaceView cameraView;
    public static GeckoApp mAppContext;
    protected MenuPanel mMenuPanel;
    protected Menu mMenu;
    private static GeckoThread sGeckoThread;
    private GeckoProfile mProfile;
    public static int mOrientation;
    protected boolean mIsRestoringActivity;
    private String mCurrentResponse = "";
    public static boolean sIsUsingCustomProfile = false;

    private PromptService mPromptService;
    private TextSelection mTextSelection;

    protected DoorHangerPopup mDoorHangerPopup;
    protected FormAssistPopup mFormAssistPopup;
    protected TabsPanel mTabsPanel;

    protected LayerView mLayerView;
    private AbsoluteLayout mPluginContainer;

    private FullScreenHolder mFullScreenPluginContainer;
    private View mFullScreenPluginView;

    private HashMap<String, PowerManager.WakeLock> mWakeLocks = new HashMap<String, PowerManager.WakeLock>();

    protected int mRestoreMode = RESTORE_NONE;
    protected boolean mInitialized = false;
    private Telemetry.Timer mJavaUiStartupTimer;
    private Telemetry.Timer mGeckoReadyStartupTimer;

    private String mPrivateBrowsingSession;

    private PointF mInitialTouchPoint = null;

    private static Boolean sIsLargeTablet = null;
    private static Boolean sIsSmallTablet = null;
    private static Boolean sIsTouchDevice = null;

    abstract public int getLayout();
    abstract public boolean hasTabsSideBar();
    abstract protected String getDefaultProfileName();

    void toggleChrome(final boolean aShow) { }

    void focusChrome() { }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        
        
        switch(msg) {
            case UNSELECTED:
                hidePlugins(tab);
                break;

            case LOCATION_CHANGE:
                
                if (!Tabs.getInstance().isSelectedTab(tab))
                    break;
                
            case SELECTED:
                invalidateOptionsMenu();
                if (mFormAssistPopup != null)
                    mFormAssistPopup.hide();
                break;

            case LOADED:
                
                
                LayerView layerView = mLayerView;
                if (layerView != null && Tabs.getInstance().isSelectedTab(tab))
                    layerView.setBackgroundColor(tab.getBackgroundColor());
                break;

            case DESKTOP_MODE_CHANGE:
                if (Tabs.getInstance().isSelectedTab(tab))
                    invalidateOptionsMenu();
                break;
        }
    }

    public void refreshChrome() { }

    public static final String PLUGIN_ACTION = "android.webkit.PLUGIN";

    



    public static final String PLUGIN_PERMISSION = "android.webkit.permission.PLUGIN";

    private static final String PLUGIN_SYSTEM_LIB = "/system/lib/plugins/";

    private static final String PLUGIN_TYPE = "type";
    private static final String TYPE_NATIVE = "native";
    public ArrayList<PackageInfo> mPackageInfoCache = new ArrayList<PackageInfo>();

    
    String[] getPluginDirectories() {

        
        boolean isTegra = (new File("/system/lib/hw/gralloc.tegra.so")).exists();
        if (isTegra) {
            
            File vfile = new File("/proc/version");
            FileReader vreader = null;
            try {
                if (vfile.canRead()) {
                    vreader = new FileReader(vfile);
                    String version = new BufferedReader(vreader).readLine();
                    if (version.indexOf("CM9") != -1 ||
                        version.indexOf("cyanogen") != -1 ||
                        version.indexOf("Nova") != -1)
                    {
                        Log.w(LOGTAG, "Blocking plugins because of Tegra 2 + unofficial ICS bug (bug 736421)");
                        return null;
                    }
                }
            } catch (IOException ex) {
                
            } finally {
                try {
                    if (vreader != null) {
                        vreader.close();
                    }
                } catch (IOException ex) {
                    
                }
            }
        }

        ArrayList<String> directories = new ArrayList<String>();
        PackageManager pm = mAppContext.getPackageManager();
        List<ResolveInfo> plugins = pm.queryIntentServices(new Intent(PLUGIN_ACTION),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);

        synchronized(mPackageInfoCache) {

            
            mPackageInfoCache.clear();


            for (ResolveInfo info : plugins) {

                
                ServiceInfo serviceInfo = info.serviceInfo;
                if (serviceInfo == null) {
                    Log.w(LOGTAG, "Ignoring bad plugin.");
                    continue;
                }

                
                
                
                if (serviceInfo.packageName.equals("com.htc.flashliteplugin")) {
                    Log.w(LOGTAG, "Skipping HTC's flash lite plugin");
                    continue;
                }


                
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
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Could not load package information.");
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
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Does not have required permission.");
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
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Does not have required permission (2).");
                    continue;
                }

                
                Signature signatures[] = pkgInfo.signatures;
                if (signatures == null) {
                    Log.w(LOGTAG, "Not loading plugin: " + serviceInfo.packageName + ". Not signed.");
                    continue;
                }

                
                if (serviceInfo.metaData == null) {
                    Log.e(LOGTAG, "The plugin '" + serviceInfo.name + "' has no defined type.");
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

        return directories.toArray(new String[directories.size()]);
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
    public void invalidateOptionsMenu() {
        if (mMenu == null)
            return;

        onPrepareOptionsMenu(mMenu);

        if (Build.VERSION.SDK_INT >= 11)
            super.invalidateOptionsMenu();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        mMenu = menu;

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.gecko_app_menu, mMenu);
        return true;
    }

    @Override
    public MenuInflater getMenuInflater() {
        if (Build.VERSION.SDK_INT >= 11)
            return new GeckoMenuInflater(mAppContext);
        else
            return super.getMenuInflater();
    }

    public MenuPanel getMenuPanel() {
        return mMenuPanel;
    }

    @Override
    public boolean onMenuItemSelected(MenuItem item) {
        return onOptionsItemSelected(item);
    }

    @Override
    public void openMenu() {
        openOptionsMenu();
    }

    @Override
    public void showMenu(View menu) {
        
        if (!hasPermanentMenuKey())
            closeMenu();

        mMenuPanel.removeAllViews();
        mMenuPanel.addView(menu);

        openOptionsMenu();
    }

    @Override
    public void closeMenu() {
        closeOptionsMenu();
    }

    @Override
    public View onCreatePanelView(int featureId) {
        if (Build.VERSION.SDK_INT >= 11 && featureId == Window.FEATURE_OPTIONS_PANEL) {
            if (mMenuPanel == null) {
                mMenuPanel = new MenuPanel(mAppContext, null);
            } else {
                
                onPreparePanel(featureId, mMenuPanel, mMenu);
            }

            return mMenuPanel; 
        }
  
        return super.onCreatePanelView(featureId);
    }

    @Override
    public boolean onCreatePanelMenu(int featureId, Menu menu) {
        if (Build.VERSION.SDK_INT >= 11 && featureId == Window.FEATURE_OPTIONS_PANEL) {
            if (mMenuPanel == null) {
                mMenuPanel = (MenuPanel) onCreatePanelView(featureId);
            }

            GeckoMenu gMenu = new GeckoMenu(mAppContext, null);
            gMenu.setCallback(this);
            gMenu.setMenuPresenter(this);
            menu = gMenu;
            mMenuPanel.addView(gMenu);

            return onCreateOptionsMenu(menu);
        }

        return super.onCreatePanelMenu(featureId, menu);
    }

    @Override
    public boolean onPreparePanel(int featureId, View view, Menu menu) {
        if (Build.VERSION.SDK_INT >= 11 && featureId == Window.FEATURE_OPTIONS_PANEL)
            return onPrepareOptionsMenu(menu);

        return super.onPreparePanel(featureId, view, menu);
    }

    @Override
    public boolean onMenuOpened(int featureId, Menu menu) {
        
        if (mLayerView != null && mLayerView.isFullScreen()) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FullScreen:Exit", null));
        }

        if (Build.VERSION.SDK_INT >= 11 && featureId == Window.FEATURE_OPTIONS_PANEL) {
            if (mMenu == null) {
                onCreatePanelMenu(featureId, menu);
                onPreparePanel(featureId, mMenuPanel, mMenu);
            }

            
            if (mMenuPanel != null)
                mMenuPanel.scrollTo(0, 0);

            return true;
        }

        return super.onMenuOpened(featureId, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.quit:
                if (GeckoThread.checkAndSetLaunchState(GeckoThread.LaunchState.GeckoRunning, GeckoThread.LaunchState.GeckoExiting)) {
                    GeckoAppShell.notifyGeckoOfEvent(GeckoEvent.createBroadcastEvent("Browser:Quit", null));
                } else {
                    System.exit(0);
                }
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    public void onOptionsMenuClosed(Menu menu) {
        if (Build.VERSION.SDK_INT >= 11) {
            mMenuPanel.removeAllViews();
            mMenuPanel.addView((GeckoMenu) mMenu);
        }
    }
 
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        
        if (Build.VERSION.SDK_INT >= 11 && keyCode == KeyEvent.KEYCODE_MENU) {
            openOptionsMenu();
            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    protected void shareCurrentUrl() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null)
          return;

        String url = tab.getURL();
        if (url == null)
            return;

        if (ReaderModeUtils.isAboutReader(url))
            url = ReaderModeUtils.getUrlFromAboutReader(url);

        GeckoAppShell.openUriExternal(url, "text/plain", "", "",
                                      Intent.ACTION_SEND, tab.getDisplayTitle());
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        if (outState == null)
            outState = new Bundle();

        outState.putBoolean(SAVED_STATE_IN_BACKGROUND, isApplicationInBackground());
        outState.putString(SAVED_STATE_PRIVATE_SESSION, mPrivateBrowsingSession);
    }

    void handleFaviconRequest(final String url) {
        (new UiAsyncTask<Void, Void, String>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public String doInBackground(Void... params) {
                return Favicons.getInstance().getFaviconUrlForPageUrl(url);
            }

            @Override
            public void onPostExecute(String faviconUrl) {
                JSONObject args = new JSONObject();

                if (faviconUrl != null) {
                    try {
                        args.put("url", url);
                        args.put("faviconUrl", faviconUrl);
                    } catch (JSONException e) {
                        Log.w(LOGTAG, "Error building JSON favicon arguments.", e);
                    }
                }

                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:FaviconReturn", args.toString()));
            }
        }).execute();
    }

    void handleClearHistory() {
        BrowserDB.clearHistory(getContentResolver());
    }

    



    public StartupMode getStartupMode() {

        synchronized(this) {
            if (mStartupMode != null)
                return mStartupMode;

            String packageName = getPackageName();
            SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);

            
            
            String profileName = getDefaultProfileName();
            if (profileName == null)
                profileName = "default";
            String keyName = packageName + "." + profileName + ".startup_version";
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

    public void addTab() { }

    public void addPrivateTab() { }

    public void showNormalTabs() { }

    public void showPrivateTabs() { }

    public void showRemoteTabs() { }

    private void showTabs(TabsPanel.Panel panel) { }

    public void hideTabs() { }

    






    public boolean autoHideTabs() { return false; }

    public boolean areTabsShown() { return false; }

    public boolean hasPermanentMenuKey() {
        boolean hasMenu = true;

        if (Build.VERSION.SDK_INT >= 11)
            hasMenu = false;

        if (Build.VERSION.SDK_INT >= 14)
            hasMenu = ViewConfiguration.get(GeckoApp.mAppContext).hasPermanentMenuKey();

        return hasMenu;
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Toast:Show")) {
                final String msg = message.getString("message");
                final String duration = message.getString("duration");
                handleShowToast(msg, duration);
            } else if (event.equals("log")) {
                
                final String msg = message.getString("msg");
                Log.d(LOGTAG, "Log: " + msg);
            } else if (event.equals("Reader:FaviconRequest")) {
                final String url = message.getString("url");
                handleFaviconRequest(url);
            } else if (event.equals("Reader:GoToReadingList")) {
                showReadingList();
            } else if (event.equals("Gecko:Ready")) {
                mGeckoReadyStartupTimer.stop();
                connectGeckoLayerClient();
            } else if (event.equals("ToggleChrome:Hide")) {
                toggleChrome(false);
            } else if (event.equals("ToggleChrome:Show")) {
                toggleChrome(true);
            } else if (event.equals("ToggleChrome:Focus")) {
                focusChrome();
            } else if (event.equals("DOMFullScreen:Start")) {
                
                LayerView layerView = mLayerView;
                if (layerView != null) {
                    layerView.setFullScreen(true);
                }
            } else if (event.equals("DOMFullScreen:Stop")) {
                
                LayerView layerView = mLayerView;
                if (layerView != null) {
                    layerView.setFullScreen(false);
                }
            } else if (event.equals("Permissions:Data")) {
                String host = message.getString("host");
                JSONArray permissions = message.getJSONArray("permissions");
                showSiteSettingsDialog(host, permissions);
            } else if (event.equals("Tab:ViewportMetadata")) {
                int tabId = message.getInt("tabID");
                Tab tab = Tabs.getInstance().getTab(tabId);
                if (tab == null)
                    return;
                tab.setZoomConstraints(new ZoomConstraints(message));
                
                LayerView layerView = mLayerView;
                if (layerView != null && Tabs.getInstance().isSelectedTab(tab)) {
                    layerView.setZoomConstraints(tab.getZoomConstraints());
                }
            } else if (event.equals("Session:StatePurged")) {
                onStatePurged();
            } else if (event.equals("Bookmark:Insert")) {
                final String url = message.getString("url");
                final String title = message.getString("title");
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(GeckoApp.mAppContext, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        ThreadUtils.postToBackgroundThread(new Runnable() {
                            @Override
                            public void run() {
                                BrowserDB.addBookmark(GeckoApp.mAppContext.getContentResolver(), title, url);
                            }
                        });
                    }
                });
            } else if (event.equals("Accessibility:Event")) {
                GeckoAccessibility.sendAccessibilityEvent(message);
            } else if (event.equals("Accessibility:Ready")) {
                GeckoAccessibility.updateAccessibilitySettings();
            } else if (event.equals("Shortcut:Remove")) {
                final String url = message.getString("url");
                final String origin = message.getString("origin");
                final String title = message.getString("title");
                final String type = message.getString("shortcutType");
                GeckoAppShell.removeShortcut(title, url, origin, type);
            } else if (event.equals("WebApps:Open")) {
                String manifestURL = message.getString("manifestURL");
                String origin = message.getString("origin");
                Intent intent = GeckoAppShell.getWebAppIntent(manifestURL, origin, "", null);
                if (intent == null)
                    return;
                startActivity(intent);
            } else if (event.equals("WebApps:Install")) {
                String name = message.getString("name");
                String manifestURL = message.getString("manifestURL");
                String iconURL = message.getString("iconURL");
                String origin = message.getString("origin");
                
                mCurrentResponse = GeckoAppShell.preInstallWebApp(name, manifestURL, origin).toString();
                GeckoAppShell.postInstallWebApp(name, manifestURL, origin, iconURL);
            } else if (event.equals("WebApps:PreInstall")) {
                String name = message.getString("name");
                String manifestURL = message.getString("manifestURL");
                String origin = message.getString("origin");
                
                mCurrentResponse = GeckoAppShell.preInstallWebApp(name, manifestURL, origin).toString();
            } else if (event.equals("WebApps:PostInstall")) {
                String name = message.getString("name");
                String manifestURL = message.getString("manifestURL");
                String iconURL = message.getString("iconURL");
                String origin = message.getString("origin");
                GeckoAppShell.postInstallWebApp(name, manifestURL, origin, iconURL);
            } else if (event.equals("WebApps:Uninstall")) {
                String origin = message.getString("origin");
                GeckoAppShell.uninstallWebApp(origin);
            } else if (event.equals("Share:Text")) {
                String text = message.getString("text");
                GeckoAppShell.openUriExternal(text, "text/plain", "", "", Intent.ACTION_SEND, "");
            } else if (event.equals("Share:Image")) {
                String src = message.getString("url");
                String type = message.getString("mime");
                GeckoAppShell.shareImage(src, type);
            } else if (event.equals("Wallpaper:Set")) {
                String src = message.getString("url");
                setImageAsWallpaper(src);
            } else if (event.equals("Sanitize:ClearHistory")) {
                handleClearHistory();
            } else if (event.equals("Update:Check")) {
                startService(new Intent(UpdateServiceHelper.ACTION_CHECK_FOR_UPDATE, null, this, UpdateService.class));
            } else if (event.equals("Update:Download")) {
                startService(new Intent(UpdateServiceHelper.ACTION_DOWNLOAD_UPDATE, null, this, UpdateService.class));
            } else if (event.equals("Update:Install")) {
                startService(new Intent(UpdateServiceHelper.ACTION_APPLY_UPDATE, null, this, UpdateService.class));
            } else if (event.equals("PrivateBrowsing:Data")) {
                
                if (message.isNull("session")) {
                    mPrivateBrowsingSession = null;
                } else {
                    mPrivateBrowsingSession = message.getString("session");
                }
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    @Override
    public String getResponse() {
        String res = mCurrentResponse;
        mCurrentResponse = "";
        return res;
    }

    void onStatePurged() { }

    




    private void showSiteSettingsDialog(String aHost, JSONArray aPermissions) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);

        View customTitleView = getLayoutInflater().inflate(R.layout.site_setting_title, null);
        ((TextView) customTitleView.findViewById(R.id.title)).setText(R.string.site_settings_title);
        ((TextView) customTitleView.findViewById(R.id.host)).setText(aHost);
        builder.setCustomTitle(customTitleView);

        
        
        if (aPermissions.length() == 0) {
            builder.setMessage(R.string.site_settings_no_settings);
        } else {

            ArrayList <HashMap<String, String>> itemList = new ArrayList <HashMap<String, String>>();
            for (int i = 0; i < aPermissions.length(); i++) {
                try {
                    JSONObject permObj = aPermissions.getJSONObject(i);
                    HashMap<String, String> map = new HashMap<String, String>();
                    map.put("setting", permObj.getString("setting"));
                    map.put("value", permObj.getString("value"));
                    itemList.add(map);
                } catch (JSONException e) {
                    Log.w(LOGTAG, "Exception populating settings items.", e);
                }
            }

            
            
            builder.setSingleChoiceItems(new SimpleAdapter(
                GeckoApp.this,
                itemList,
                R.layout.site_setting_item,
                new String[] { "setting", "value" },
                new int[] { R.id.setting, R.id.value }
                ), -1, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) { }
                });

            builder.setPositiveButton(R.string.site_settings_clear, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int id) {
                    ListView listView = ((AlertDialog) dialog).getListView();
                    SparseBooleanArray checkedItemPositions = listView.getCheckedItemPositions();

                    
                    JSONArray permissionsToClear = new JSONArray();
                    for (int i = 0; i < checkedItemPositions.size(); i++)
                        if (checkedItemPositions.get(i))
                            permissionsToClear.put(i);

                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                        "Permissions:Clear", permissionsToClear.toString()));
                }
            });
        }

        builder.setNegativeButton(R.string.site_settings_cancel, new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });

        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Dialog dialog = builder.create();
                dialog.show();

                ListView listView = ((AlertDialog) dialog).getListView();
                if (listView != null) {
                    listView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
                    int listSize = listView.getAdapter().getCount();
                    for (int i = 0; i < listSize; i++)
                        listView.setItemChecked(i, true);
                }
            }
        });
    }

    public void showToast(final int resId, final int duration) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(mAppContext, resId, duration).show();
            }
        });
    }

    void handleShowToast(final String message, final String duration) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
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

    private void addFullScreenPluginView(View view) {
        if (mFullScreenPluginView != null) {
            Log.w(LOGTAG, "Already have a fullscreen plugin view");
            return;
        }

        setFullScreen(true);

        view.setWillNotDraw(false);
        if (view instanceof SurfaceView) {
            ((SurfaceView) view).setZOrderOnTop(true);
        }

        mFullScreenPluginContainer = new FullScreenHolder(this);

        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(
                            ViewGroup.LayoutParams.FILL_PARENT,
                            ViewGroup.LayoutParams.FILL_PARENT,
                            Gravity.CENTER);
        mFullScreenPluginContainer.addView(view, layoutParams);


        FrameLayout decor = (FrameLayout)getWindow().getDecorView();
        decor.addView(mFullScreenPluginContainer, layoutParams);

        mFullScreenPluginView = view;
    }

    void addPluginView(final View view, final Rect rect, final boolean isFullScreen) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Tabs tabs = Tabs.getInstance();
                Tab tab = tabs.getSelectedTab();

                if (isFullScreen) {
                    addFullScreenPluginView(view);
                    return;
                }

                PluginLayer layer = (PluginLayer) tab.getPluginLayer(view);
                if (layer == null) {
                    layer = new PluginLayer(view, rect, mLayerView.getRenderer().getMaxTextureSize());
                    tab.addPluginLayer(view, layer);
                } else {
                    layer.reset(rect);
                    layer.setVisible(true);
                }

                mLayerView.addLayer(layer);
            }
        });
    }

    private void removeFullScreenPluginView(View view) {
        if (mFullScreenPluginView == null) {
            Log.w(LOGTAG, "Don't have a fullscreen plugin view");
            return;
        }

        if (mFullScreenPluginView != view) {
            Log.w(LOGTAG, "Passed view is not the current full screen view");
            return;
        }

        mFullScreenPluginContainer.removeView(mFullScreenPluginView);

        
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                mLayerView.show();
            }
        });

        FrameLayout decor = (FrameLayout)getWindow().getDecorView();
        decor.removeView(mFullScreenPluginContainer);

        mFullScreenPluginView = null;

        GeckoScreenOrientationListener.getInstance().unlockScreenOrientation();
        setFullScreen(false);
    }

    void removePluginView(final View view, final boolean isFullScreen) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Tabs tabs = Tabs.getInstance();
                Tab tab = tabs.getSelectedTab();

                if (isFullScreen) {
                    removeFullScreenPluginView(view);
                    return;
                }

                PluginLayer layer = (PluginLayer) tab.removePluginLayer(view);
                if (layer != null) {
                    layer.destroy();
                }
            }
        });
    }

    private void setImageAsWallpaper(final String aSrc) {
        final String progText = mAppContext.getString(R.string.wallpaper_progress);
        final String successText = mAppContext.getString(R.string.wallpaper_success);
        final String failureText = mAppContext.getString(R.string.wallpaper_fail);
        final String fileName = aSrc.substring(aSrc.lastIndexOf("/") + 1);
        final PendingIntent emptyIntent = PendingIntent.getActivity(mAppContext, 0, new Intent(), 0);
        final AlertNotification notification = new AlertNotification(mAppContext, fileName.hashCode(), 
                                R.drawable.alert_download, fileName, progText, System.currentTimeMillis(), null);
        notification.setLatestEventInfo(mAppContext, fileName, progText, emptyIntent );
        notification.flags |= Notification.FLAG_ONGOING_EVENT;
        notification.show();
        new UiAsyncTask<Void, Void, Boolean>(ThreadUtils.getBackgroundHandler()) {

            @Override
            protected Boolean doInBackground(Void... params) {
                WallpaperManager mgr = WallpaperManager.getInstance(mAppContext);
                if (mgr == null) {
                    return false;
                }

                
                

                int idealWidth = mgr.getDesiredMinimumWidth();
                int idealHeight = mgr.getDesiredMinimumHeight();

                
                
                
                
                

                if (idealWidth <= 0 || idealHeight <= 0) {
                    int orientation;
                    Display defaultDisplay = getWindowManager().getDefaultDisplay();
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
                        orientation = defaultDisplay.getRotation();
                    } else {
                        orientation = defaultDisplay.getOrientation();
                    }
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
                        Point size = new Point();
                        defaultDisplay.getSize(size);
                        
                        
                        if (orientation == Surface.ROTATION_0 || orientation == Surface.ROTATION_270) {
                            idealWidth = size.x * 2;
                            idealHeight = size.y;
                        } else {
                            idealWidth = size.y;
                            idealHeight = size.x * 2;
                        }
                    } else {
                        if (orientation == Surface.ROTATION_0 || orientation == Surface.ROTATION_270) {
                            idealWidth = defaultDisplay.getWidth() * 2;
                            idealHeight = defaultDisplay.getHeight();
                        } else {
                            idealWidth = defaultDisplay.getHeight();
                            idealHeight = defaultDisplay.getWidth() * 2;
                        }
                    }
                }

                boolean isDataURI = aSrc.startsWith("data:");
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inJustDecodeBounds = true;
                Bitmap image = null;
                InputStream is = null;
                ByteArrayOutputStream os = null;
                try{
                    if (isDataURI) {
                        int dataStart = aSrc.indexOf(',');
                        byte[] buf = Base64.decode(aSrc.substring(dataStart+1), Base64.DEFAULT);
                        BitmapFactory.decodeByteArray(buf, 0, buf.length, options);
                        options.inSampleSize = getBitmapSampleSize(options, idealWidth, idealHeight);
                        options.inJustDecodeBounds = false;
                        image = BitmapFactory.decodeByteArray(buf, 0, buf.length, options);
                    } else {
                        int byteRead;
                        byte[] buf = new byte[4192];
                        os = new ByteArrayOutputStream();
                        URL url = new URL(aSrc);
                        is = url.openStream();

                        
                        

                        while((byteRead = is.read(buf)) != -1) {
                            os.write(buf, 0, byteRead);
                        }
                        byte[] imgBuffer = os.toByteArray();
                        BitmapFactory.decodeByteArray(imgBuffer, 0, imgBuffer.length, options);
                        options.inSampleSize = getBitmapSampleSize(options, idealWidth, idealHeight);
                        options.inJustDecodeBounds = false;
                        image = BitmapFactory.decodeByteArray(imgBuffer, 0, imgBuffer.length, options);
                    }
                    if(image != null) {
                        mgr.setBitmap(image);
                        return true;
                    } else {
                        return false;
                    }
                } catch(OutOfMemoryError ome) {
                    Log.e(LOGTAG, "Out of Memmory when coverting to byte array", ome);
                    return false;
                } catch(IOException ioe) {
                    Log.e(LOGTAG, "I/O Exception while setting wallpaper", ioe);
                    return false;
                } finally {
                    if(is != null) {
                        try {
                            is.close();
                        } catch(IOException ioe) {
                            Log.w(LOGTAG, "I/O Exception while closing stream", ioe);
                        }
                    }
                    if(os != null) {
                        try {
                            os.close();
                        } catch(IOException ioe) {
                            Log.w(LOGTAG, "I/O Exception while closing stream", ioe);
                        }
                    }
                }
            }

            @Override
            protected void onPostExecute(Boolean success) {
                notification.cancel();
                notification.flags = 0;
                notification.flags |= Notification.FLAG_AUTO_CANCEL;
                if(!success) {
                    notification.tickerText = failureText;
                    notification.setLatestEventInfo(mAppContext, fileName, failureText, emptyIntent);
                } else {
                    notification.tickerText = successText;
                    notification.setLatestEventInfo(mAppContext, fileName, successText, emptyIntent);
                }
                notification.show();
            }
        }.execute();
    }

    private int getBitmapSampleSize(BitmapFactory.Options options, int idealWidth, int idealHeight) {
        int width = options.outWidth;
        int height = options.outHeight;
        int inSampleSize = 1;
        if (height > idealHeight || width > idealWidth) {
            if (width > height) {
                inSampleSize = Math.round((float)height / (float)idealHeight);
            } else {
                inSampleSize = Math.round((float)width / (float)idealWidth);
            }
        }
        return inSampleSize;
    }

    private void hidePluginLayer(Layer layer) {
        LayerView layerView = mLayerView;
        layerView.removeLayer(layer);
        layerView.requestRender();
    }

    private void showPluginLayer(Layer layer) {
        LayerView layerView = mLayerView;
        layerView.addLayer(layer);
        layerView.requestRender();
    }

    public void requestRender() {
        mLayerView.requestRender();
    }
    
    public void hidePlugins(Tab tab) {
        for (Layer layer : tab.getPluginLayers()) {
            if (layer instanceof PluginLayer) {
                ((PluginLayer) layer).setVisible(false);
            }

            hidePluginLayer(layer);
        }

        requestRender();
    }

    public void showPlugins() {
        Tabs tabs = Tabs.getInstance();
        Tab tab = tabs.getSelectedTab();

        showPlugins(tab);
    }

    public void showPlugins(Tab tab) {
        for (Layer layer : tab.getPluginLayers()) {
            showPluginLayer(layer);

            if (layer instanceof PluginLayer) {
                ((PluginLayer) layer).setVisible(true);
            }
        }

        requestRender();
    }

    public void setFullScreen(final boolean fullscreen) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
                Window window = getWindow();
                window.setFlags(fullscreen ?
                                WindowManager.LayoutParams.FLAG_FULLSCREEN : 0,
                                WindowManager.LayoutParams.FLAG_FULLSCREEN);

                if (Build.VERSION.SDK_INT >= 11)
                    window.getDecorView().setSystemUiVisibility(fullscreen ? 1 : 0);
            }
        });
    }

    public boolean isTablet() {
        return isLargeTablet() || isSmallTablet();
    }

    public boolean isLargeTablet() {
        if (sIsLargeTablet == null) {
            int screenLayout = getResources().getConfiguration().screenLayout;
            sIsLargeTablet = (Build.VERSION.SDK_INT >= 11 &&
                              ((screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) == Configuration.SCREENLAYOUT_SIZE_XLARGE));
        }

        return sIsLargeTablet;
    }

    public boolean isSmallTablet() {
        if (sIsSmallTablet == null) {
            int screenLayout = getResources().getConfiguration().screenLayout;
            sIsSmallTablet = (Build.VERSION.SDK_INT >= 11 &&
                              ((screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) == Configuration.SCREENLAYOUT_SIZE_LARGE));
        }

        return sIsSmallTablet;
    }

    public boolean isTouchDevice() {
        if (sIsTouchDevice == null) {
            if (Build.VERSION.SDK_INT >= 9) {
                sIsTouchDevice = false;
                for (int inputDeviceId : InputDevice.getDeviceIds()) {
                    if ((InputDevice.getDevice(inputDeviceId).getSources() & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN) {
                        sIsTouchDevice = true;
                    }
                }
            } else {
                
                sIsTouchDevice = true;
            }
        }
        return sIsTouchDevice;
    }

    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        GeckoAppShell.registerGlobalExceptionHandler();

        
        if ("default".equals(GeckoAppInfo.getUpdateChannel())) {
            enableStrictMode();
        }

        
        mJavaUiStartupTimer = new Telemetry.Timer("FENNEC_STARTUP_TIME_JAVAUI");
        mGeckoReadyStartupTimer = new Telemetry.Timer("FENNEC_STARTUP_TIME_GECKOREADY");

        ((GeckoApplication)getApplication()).initialize();

        mAppContext = this;
        ThreadUtils.setUiThread(Thread.currentThread(), new Handler());

        Tabs.getInstance().attachToActivity(this);
        Favicons.getInstance().attachToContext(this);

        
        if (getLastNonConfigurationInstance() != null) {
            
            doRestart();
            System.exit(0);
            return;
        }

        if (sGeckoThread != null) {
            
            
            mIsRestoringActivity = true;
            Telemetry.HistogramAdd("FENNEC_RESTORING_ACTIVITY", 1);
        }

        
        
        
        if (Build.VERSION.SDK_INT < 9) {
            try {
                Class<?> inputBindResultClass =
                    Class.forName("com.android.internal.view.InputBindResult");
                java.lang.reflect.Field creatorField =
                    inputBindResultClass.getField("CREATOR");
                Log.i(LOGTAG, "froyo startup fix: " + String.valueOf(creatorField.get(null)));
            } catch (Exception e) {
                Log.w(LOGTAG, "froyo startup fix failed", e);
            }
        }

        LayoutInflater.from(this).setFactory(GeckoViewsFactory.getInstance());

        super.onCreate(savedInstanceState);

        mOrientation = getResources().getConfiguration().orientation;

        setContentView(getLayout());

        
        mGeckoLayout = (RelativeLayout) findViewById(R.id.gecko_layout);
        mMainLayout = (RelativeLayout) findViewById(R.id.main_layout);

        
        mTabsPanel = (TabsPanel) findViewById(R.id.tabs_panel);

        
        
        
        if (savedInstanceState != null) {
            mRestoreMode = RESTORE_OOM;

            boolean wasInBackground =
                savedInstanceState.getBoolean(SAVED_STATE_IN_BACKGROUND, false);

            
            
            if (!wasInBackground && !mIsRestoringActivity) {
                Telemetry.HistogramAdd("FENNEC_WAS_KILLED", 1);
            }

            mPrivateBrowsingSession = savedInstanceState.getString(SAVED_STATE_PRIVATE_SESSION);
        }

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs =
                    GeckoApp.mAppContext.getSharedPreferences(PREFS_NAME, 0);

                boolean wasOOM = prefs.getBoolean(PREFS_OOM_EXCEPTION, false);
                boolean wasStopped = prefs.getBoolean(PREFS_WAS_STOPPED, true);
                if (wasOOM || !wasStopped) {
                    Telemetry.HistogramAdd("FENNEC_WAS_KILLED", 1);
                }
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_OOM_EXCEPTION, false);

                
                
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
                editor.commit();
            }
        });

        GeckoAppShell.setNotificationClient(makeNotificationClient());
    }

    protected void initializeChrome(String uri, boolean isExternalURL) {
        mDoorHangerPopup = new DoorHangerPopup(this, null);
        mPluginContainer = (AbsoluteLayout) findViewById(R.id.plugin_container);
        mFormAssistPopup = (FormAssistPopup) findViewById(R.id.form_assist_popup);

        if (cameraView == null) {
            cameraView = new SurfaceView(this);
            cameraView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }

        if (mLayerView == null) {
            LayerView layerView = (LayerView) findViewById(R.id.layer_view);
            layerView.initializeView(GeckoAppShell.getEventDispatcher());
            mLayerView = layerView;
            
            GeckoAppShell.notifyIMEContext(GeckoEditableListener.IME_STATE_DISABLED, "", "", "");
        }
    }

    private void initialize() {
        mInitialized = true;

        invalidateOptionsMenu();

        Intent intent = getIntent();
        String action = intent.getAction();
        String args = intent.getStringExtra("args");

        String profileName = null;
        String profilePath = null;
        if (args != null) {
            if (args.contains("-P")) {
                Pattern p = Pattern.compile("(?:-P\\s*)(\\w*)(\\s*)");
                Matcher m = p.matcher(args);
                if (m.find()) {
                    profileName = m.group(1);
                }
            }
            if (args.contains("-profile")) {
                Pattern p = Pattern.compile("(?:-profile\\s*)(\\S*)(\\s*)");
                Matcher m = p.matcher(args);
                if (m.find()) {
                    profilePath =  m.group(1);
                }
                if (profileName == null) {
                    profileName = getDefaultProfileName();
                    if (profileName == null)
                        profileName = "default";
                }
                GeckoApp.sIsUsingCustomProfile = true;
            }
            if (profileName != null || profilePath != null) {
                mProfile = GeckoProfile.get(this, profileName, profilePath);
            }
        }

        BrowserDB.initialize(getProfile().getName());

        String passedUri = null;
        String uri = getURIFromIntent(intent);
        if (uri != null && uri.length() > 0) {
            passedUri = uri;
        }

        if (mRestoreMode == RESTORE_NONE && shouldRestoreSession()) {
            mRestoreMode = RESTORE_CRASH;
        }

        final boolean isExternalURL = passedUri != null && !passedUri.equals("about:home");
        StartupAction startupAction;
        if (isExternalURL) {
            startupAction = StartupAction.URL;
        } else {
            startupAction = StartupAction.NORMAL;
        }

        
        
        checkMigrateProfile();

        Uri data = intent.getData();
        if (data != null && "http".equals(data.getScheme())) {
            startupAction = StartupAction.PREFETCH;
            ThreadUtils.postToBackgroundThread(new PrefetchRunnable(data.toString()));
        }

        Tabs.registerOnTabsChangedListener(this);

        
        String restoreMessage = null;
        if (mRestoreMode != RESTORE_NONE && !mIsRestoringActivity) {
            try {
                String sessionString = getProfile().readSessionFile(false);
                if (sessionString == null) {
                    throw new IOException("could not read from session file");
                }

                
                
                
                if (mRestoreMode == RESTORE_OOM) {
                    final JSONArray tabs = new JSONArray();
                    SessionParser parser = new SessionParser() {
                        @Override
                        public void onTabRead(SessionTab sessionTab) {
                            JSONObject tabObject = sessionTab.getTabObject();

                            int flags = Tabs.LOADURL_NEW_TAB;
                            flags |= ((isExternalURL || !sessionTab.isSelected()) ? Tabs.LOADURL_DELAY_LOAD : 0);
                            flags |= (tabObject.optBoolean("desktopMode") ? Tabs.LOADURL_DESKTOP : 0);
                            flags |= (tabObject.optBoolean("isPrivate") ? Tabs.LOADURL_PRIVATE : 0);

                            Tab tab = Tabs.getInstance().loadUrl(sessionTab.getSelectedUrl(), flags);
                            tab.updateTitle(sessionTab.getSelectedTitle());

                            try {
                                tabObject.put("tabId", tab.getId());
                            } catch (JSONException e) {
                                Log.e(LOGTAG, "JSON error", e);
                            }
                            tabs.put(tabObject);
                        }
                    };

                    parser.parse(sessionString);
                    if (mPrivateBrowsingSession != null) {
                        parser.parse(mPrivateBrowsingSession);
                    }

                    if (tabs.length() > 0) {
                        sessionString = new JSONObject().put("windows", new JSONArray().put(new JSONObject().put("tabs", tabs))).toString();
                    } else {
                        throw new Exception("No tabs could be read from session file");
                    }
                }

                JSONObject restoreData = new JSONObject();
                restoreData.put("restoringOOM", mRestoreMode == RESTORE_OOM);
                restoreData.put("sessionString", sessionString);
                restoreMessage = restoreData.toString();
            } catch (Exception e) {
                
                Log.e(LOGTAG, "An error occurred during restore", e);
                mRestoreMode = RESTORE_NONE;
            }
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Session:Restore", restoreMessage));

        if (mRestoreMode == RESTORE_OOM) {
            
            
            Tabs.getInstance().notifyListeners(null, Tabs.TabEvents.RESTORED);
        } else {
            
            getProfile().moveSessionFile();
        }

        initializeChrome(passedUri, isExternalURL);

        
        if (mRestoreMode == RESTORE_NONE) {
            Tabs.getInstance().notifyListeners(null, Tabs.TabEvents.RESTORED);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Telemetry:Prompt", null));
        }

        Telemetry.HistogramAdd("FENNEC_STARTUP_GECKOAPP_ACTION", startupAction.ordinal());

        if (!mIsRestoringActivity) {
            sGeckoThread = new GeckoThread(intent, passedUri);
            ThreadUtils.setGeckoThread(sGeckoThread);
        }
        if (!ACTION_DEBUG.equals(action) &&
            GeckoThread.checkAndSetLaunchState(GeckoThread.LaunchState.Launching, GeckoThread.LaunchState.Launched)) {
            sGeckoThread.start();
        } else if (ACTION_DEBUG.equals(action) &&
            GeckoThread.checkAndSetLaunchState(GeckoThread.LaunchState.Launching, GeckoThread.LaunchState.WaitForDebugger)) {
            ThreadUtils.getUiHandler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    GeckoThread.setLaunchState(GeckoThread.LaunchState.Launching);
                    sGeckoThread.start();
                }
            }, 1000 * 5 );
        }

        
        registerEventListener("log");
        registerEventListener("Reader:Added");
        registerEventListener("Reader:Removed");
        registerEventListener("Reader:Share");
        registerEventListener("Reader:FaviconRequest");
        registerEventListener("Reader:GoToReadingList");
        registerEventListener("onCameraCapture");
        registerEventListener("Menu:Add");
        registerEventListener("Menu:Remove");
        registerEventListener("Menu:Update");
        registerEventListener("Gecko:Ready");
        registerEventListener("Toast:Show");
        registerEventListener("DOMFullScreen:Start");
        registerEventListener("DOMFullScreen:Stop");
        registerEventListener("ToggleChrome:Hide");
        registerEventListener("ToggleChrome:Show");
        registerEventListener("ToggleChrome:Focus");
        registerEventListener("Permissions:Data");
        registerEventListener("Tab:ViewportMetadata");
        registerEventListener("Session:StatePurged");
        registerEventListener("Bookmark:Insert");
        registerEventListener("Accessibility:Event");
        registerEventListener("Accessibility:Ready");
        registerEventListener("Shortcut:Remove");
        registerEventListener("WebApps:Open");
        registerEventListener("WebApps:PreInstall");
        registerEventListener("WebApps:PostInstall");
        registerEventListener("WebApps:Install");
        registerEventListener("WebApps:Uninstall");
        registerEventListener("Share:Text");
        registerEventListener("Share:Image");
        registerEventListener("Wallpaper:Set");
        registerEventListener("Sanitize:ClearHistory");
        registerEventListener("Update:Check");
        registerEventListener("Update:Download");
        registerEventListener("Update:Install");
        registerEventListener("PrivateBrowsing:Data");

        if (SmsManager.getInstance() != null) {
          SmsManager.getInstance().start();
        }

        mPromptService = new PromptService();

        mTextSelection = new TextSelection((TextSelectionHandle) findViewById(R.id.start_handle),
                                           (TextSelectionHandle) findViewById(R.id.middle_handle),
                                           (TextSelectionHandle) findViewById(R.id.end_handle),
                                           GeckoAppShell.getEventDispatcher(),
                                           this);

        PrefsHelper.getPref("app.update.autodownload", new PrefsHelper.PrefHandlerBase() {
            @Override public void prefValue(String pref, String value) {
                UpdateServiceHelper.registerForUpdates(GeckoApp.this, value);
            }
        });

        
        mJavaUiStartupTimer.stop();

        ThreadUtils.getBackgroundHandler().postDelayed(new Runnable() {
            @Override
            public void run() {
                
                
                checkMigrateSync();

                
                
                final Context context = GeckoApp.mAppContext;
                AnnouncementsBroadcastService.recordLastLaunch(context);

                
                
                
                GeckoPreferences.broadcastAnnouncementsPref(context);

                








                if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.Launched)) {
                    return;
                }
            }
        }, 50);

        if (mIsRestoringActivity) {
            GeckoThread.setLaunchState(GeckoThread.LaunchState.GeckoRunning);
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null)
                Tabs.getInstance().notifyListeners(selectedTab, Tabs.TabEvents.SELECTED);
            connectGeckoLayerClient();
            GeckoAppShell.setLayerClient(mLayerView.getLayerClient());
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Viewport:Flush", null));
        }

        if (ACTION_ALERT_CALLBACK.equals(action)) {
            processAlertCallback(intent);
        }
    }

    public GeckoProfile getProfile() {
        
        if (mProfile == null) {
            mProfile = GeckoProfile.get(this);
        }
        return mProfile;
    }

    protected boolean shouldRestoreSession() {
        SharedPreferences prefs = GeckoApp.mAppContext.getSharedPreferences(PREFS_NAME, 0);

        
        
        
        
        if (prefs.getBoolean(PREFS_CRASHED, false)) {
            prefs.edit().putBoolean(GeckoApp.PREFS_CRASHED, false).commit();
            if (getProfile().shouldRestoreSession()) {
                return true;
            }
        }
        return false;
    }

    



    private void enableStrictMode() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.GINGERBREAD) {
            return;
        }

        Log.d(LOGTAG, "Enabling Android StrictMode");

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

    class PrefetchRunnable implements Runnable {
        private String mPrefetchUrl;

        PrefetchRunnable(String prefetchUrl) {
            mPrefetchUrl = prefetchUrl;
        }

        @Override
        public void run() {
            HttpURLConnection connection = null;
            try {
                URL url = new URL(mPrefetchUrl);
                
                connection = (HttpURLConnection) url.openConnection();
                connection.setRequestProperty("User-Agent", getUAStringForHost(url.getHost()));
                connection.setInstanceFollowRedirects(false);
                connection.setRequestMethod("GET");
                connection.connect();
            } catch (Exception e) {
                Log.e(LOGTAG, "Exception prefetching URL", e);
            } finally {
                if (connection != null)
                    connection.disconnect();
            }
        }
    }

    private void processAlertCallback(Intent intent) {
        String alertName = "";
        String alertCookie = "";
        Uri data = intent.getData();
        if (data != null) {
            alertName = data.getQueryParameter("name");
            if (alertName == null)
                alertName = "";
            alertCookie = data.getQueryParameter("cookie");
            if (alertCookie == null)
                alertCookie = "";
        }
        handleNotification(ACTION_ALERT_CALLBACK, alertName, alertCookie);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoExiting)) {
            
            
            System.exit(0);
            return;
        }

        
        
        if (!mInitialized) {
            setIntent(intent);
            return;
        }

        
        if ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) != 0)
            return;

        final String action = intent.getAction();

        if (Intent.ACTION_MAIN.equals(action)) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(""));
        } else if (ACTION_LOAD.equals(action)) {
            String uri = intent.getDataString();
            Tabs.getInstance().loadUrl(uri);
        } else if (Intent.ACTION_VIEW.equals(action)) {
            String uri = intent.getDataString();
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(uri));
        } else if (action != null && action.startsWith(ACTION_WEBAPP_PREFIX)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createWebappLoadEvent(uri));
        } else if (ACTION_BOOKMARK.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBookmarkLoadEvent(uri));
        } else if (Intent.ACTION_SEARCH.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(uri));
        } else if (ACTION_ALERT_CALLBACK.equals(action)) {
            processAlertCallback(intent);
        }
    }

    



    protected String getURIFromIntent(Intent intent) {
        final String action = intent.getAction();
        if (ACTION_ALERT_CALLBACK.equals(action))
            return null;

        String uri = intent.getDataString();
        if (uri != null)
            return uri;

        if ((action != null && action.startsWith(ACTION_WEBAPP_PREFIX)) || ACTION_BOOKMARK.equals(action)) {
            uri = intent.getStringExtra("args");
            if (uri != null && uri.startsWith("--url=")) {
                uri.replace("--url=", "");
            }
        }
        return uri;
    }

    @Override
    public void onResume()
    {
        
        
        super.onResume();

        SiteIdentityPopup.getInstance().dismiss();

        int newOrientation = getResources().getConfiguration().orientation;

        if (mOrientation != newOrientation) {
            mOrientation = newOrientation;
            refreshChrome();
        }

        GeckoScreenOrientationListener.getInstance().start();

        
        GeckoAccessibility.updateAccessibilitySettings();

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs =
                    GeckoApp.mAppContext.getSharedPreferences(GeckoApp.PREFS_NAME, 0);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
                editor.commit();
            }
         });
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (!mInitialized && hasFocus) {
            initialize();
            getWindow().setBackgroundDrawable(null);
        }
    }

    @Override
    public void onPause()
    {
        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs =
                    GeckoApp.mAppContext.getSharedPreferences(GeckoApp.PREFS_NAME, 0);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, true);
                editor.commit();

                BrowserDB.expireHistory(getContentResolver(),
                                        BrowserContract.ExpirePriority.NORMAL);
            }
        });

        GeckoScreenOrientationListener.getInstance().stop();

        super.onPause();
    }

    @Override
    public void onRestart()
    {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs =
                    GeckoApp.mAppContext.getSharedPreferences(GeckoApp.PREFS_NAME, 0);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
                editor.commit();
            }
        });

        super.onRestart();
    }

    @Override
    public void onDestroy()
    {
        unregisterEventListener("log");
        unregisterEventListener("Reader:Added");
        unregisterEventListener("Reader:Removed");
        unregisterEventListener("Reader:Share");
        unregisterEventListener("Reader:FaviconRequest");
        unregisterEventListener("Reader:GoToReadingList");
        unregisterEventListener("onCameraCapture");
        unregisterEventListener("Menu:Add");
        unregisterEventListener("Menu:Remove");
        unregisterEventListener("Menu:Update");
        unregisterEventListener("Gecko:Ready");
        unregisterEventListener("Toast:Show");
        unregisterEventListener("DOMFullScreen:Start");
        unregisterEventListener("DOMFullScreen:Stop");
        unregisterEventListener("ToggleChrome:Hide");
        unregisterEventListener("ToggleChrome:Show");
        unregisterEventListener("ToggleChrome:Focus");
        unregisterEventListener("Permissions:Data");
        unregisterEventListener("Tab:ViewportMetadata");
        unregisterEventListener("Session:StatePurged");
        unregisterEventListener("Bookmark:Insert");
        unregisterEventListener("Accessibility:Event");
        unregisterEventListener("Accessibility:Ready");
        unregisterEventListener("Shortcut:Remove");
        unregisterEventListener("WebApps:Open");
        unregisterEventListener("WebApps:PreInstall");
        unregisterEventListener("WebApps:PostInstall");
        unregisterEventListener("WebApps:Install");
        unregisterEventListener("WebApps:Uninstall");
        unregisterEventListener("Share:Text");
        unregisterEventListener("Share:Image");
        unregisterEventListener("Wallpaper:Set");
        unregisterEventListener("Sanitize:ClearHistory");
        unregisterEventListener("Update:Check");
        unregisterEventListener("Update:Download");
        unregisterEventListener("Update:Install");
        unregisterEventListener("PrivateBrowsing:Data");

        deleteTempFiles();

        if (mLayerView != null)
            mLayerView.destroy();
        if (mDoorHangerPopup != null)
            mDoorHangerPopup.destroy();
        if (mFormAssistPopup != null)
            mFormAssistPopup.destroy();
        if (mPromptService != null)
            mPromptService.destroy();
        if (mTextSelection != null)
            mTextSelection.destroy();
        SiteIdentityPopup.clearInstance();

        Tabs.getInstance().detachFromActivity(this);

        if (SmsManager.getInstance() != null) {
            SmsManager.getInstance().stop();
            if (isFinishing())
                SmsManager.getInstance().shutdown();
        }

        super.onDestroy();

        Tabs.unregisterOnTabsChangedListener(this);
    }

    protected void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    protected void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, this);
    }

    
    public static File getTempDirectory() {
        File dir = mAppContext.getExternalFilesDir("temp");
        return dir;
    }

    
    public static void deleteTempFiles() {
        File dir = getTempDirectory();
        if (dir == null)
            return;
        File[] files = dir.listFiles();
        if (files == null)
            return;
        for (File file : files) {
            file.delete();
        }
    }

    @Override
    public void onContentChanged() {
        super.onContentChanged();
    }


    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (mOrientation != newConfig.orientation) {
            mOrientation = newConfig.orientation;
            if (mFormAssistPopup != null)
                mFormAssistPopup.hide();
            SiteIdentityPopup.getInstance().dismiss();
            refreshChrome();
        }
    }

    @Override
    public Object onRetainNonConfigurationInstance() {
        
        
        return Boolean.TRUE;
    } 

    @Override
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
        Log.d(LOGTAG, "doRestart(\"" + action + "\")");
        try {
            Intent intent = new Intent(action);
            intent.setClassName(getPackageName(),
                                getPackageName() + ".Restarter");
            
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
                            Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            Log.d(LOGTAG, "Restart intent: " + intent.toString());
            GeckoAppShell.killAnyZombies();
            startActivity(intent);
        } catch (Exception e) {
            Log.e(LOGTAG, "Error effecting restart.", e);
        }
        finish();
        
        GeckoAppShell.waitForAnotherGeckoProc();
    }

    public void handleNotification(String action, String alertName, String alertCookie) {
        
        
        
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            GeckoAppShell.handleNotification(action, alertName, alertCookie);
        }
    }

    private void checkMigrateProfile() {
        final File profileDir = getProfile().getDir();

        if (profileDir != null) {
            final GeckoApp app = GeckoApp.mAppContext;

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    ProfileMigrator profileMigrator = new ProfileMigrator(app);

                    
                    if (!GeckoApp.sIsUsingCustomProfile &&
                        !profileMigrator.hasMigrationRun()) {
                        
                        

                        
                        
                        
                        
                        
                        final SetupScreen[] setupScreenHolder = new SetupScreen[1];

                        final Runnable startCallback = new Runnable() {
                            @Override
                            public void run() {
                                GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        setupScreenHolder[0] = new SetupScreen(app);
                                        setupScreenHolder[0].show();
                                    }
                                });
                            }
                        };

                        final Runnable stopCallback = new Runnable() {
                            @Override
                            public void run() {
                                GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        SetupScreen screen = setupScreenHolder[0];
                                        
                                        
                                        if (screen != null) {
                                            screen.dismiss();
                                        }
                                    }
                                });
                            }
                        };

                        profileMigrator.setLongOperationCallbacks(startCallback,
                                                                  stopCallback);
                        profileMigrator.launchPlaces(profileDir);
                        finishProfileMigration();
                    }
                }}
            );
        }
    }

    protected void finishProfileMigration() {
    }

    private void checkMigrateSync() {
        final File profileDir = getProfile().getDir();
        if (!GeckoApp.sIsUsingCustomProfile && profileDir != null) {
            final GeckoApp app = GeckoApp.mAppContext;
            ProfileMigrator profileMigrator = new ProfileMigrator(app);
            if (!profileMigrator.hasSyncMigrated()) {
                profileMigrator.launchSyncPrefs();
            }
        }
    }

    PromptService getPromptService() {
        return mPromptService;
    }

    @Override
    public boolean onSearchRequested() {
        return showAwesomebar(AwesomeBar.Target.CURRENT_TAB);
    }

    public boolean showAwesomebar(AwesomeBar.Target aTarget) {
        return showAwesomebar(aTarget, null);
    }

    public boolean showAwesomebar(AwesomeBar.Target aTarget, String aUrl) {
        Intent intent = new Intent(getBaseContext(), AwesomeBar.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
        intent.putExtra(AwesomeBar.TARGET_KEY, aTarget.name());

        
        if (aUrl != null && !TextUtils.isEmpty(aUrl)) {
            intent.putExtra(AwesomeBar.CURRENT_URL_KEY, aUrl);
        } else if (aTarget == AwesomeBar.Target.CURRENT_TAB) {
            
            Tab tab = Tabs.getInstance().getSelectedTab();
            if (tab != null) {

                aUrl = tab.getURL();
                if (aUrl != null) {
                    intent.putExtra(AwesomeBar.CURRENT_URL_KEY, aUrl);
                }

            }
        }

        int requestCode = GeckoAppShell.sActivityHelper.makeRequestCodeForAwesomebar();
        startActivityForResult(intent, requestCode);
        overridePendingTransition (R.anim.awesomebar_fade_in, R.anim.awesomebar_hold_still);
        return true;
    }

    public void showReadingList() {
        Intent intent = new Intent(getBaseContext(), AwesomeBar.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_NO_HISTORY);
        intent.putExtra(AwesomeBar.TARGET_KEY, AwesomeBar.Target.CURRENT_TAB.toString());
        intent.putExtra(AwesomeBar.READING_LIST_KEY, true);

        int requestCode = GeckoAppShell.sActivityHelper.makeRequestCodeForAwesomebar();
        startActivityForResult(intent, requestCode);
    }

    @Override
    public void onBackPressed() {
        if (autoHideTabs()) {
            return;
        }

        if (mDoorHangerPopup != null && mDoorHangerPopup.isShowing()) {
            mDoorHangerPopup.dismiss();
            return;
        }

        if (mFullScreenPluginView != null) {
            GeckoAppShell.onFullScreenPluginHidden(mFullScreenPluginView);
            removeFullScreenPluginView(mFullScreenPluginView);
            return;
        }

        SiteIdentityPopup identityPopup = SiteIdentityPopup.getInstance();
        if (identityPopup.isShowing()) {
            identityPopup.dismiss();
            return;
        }

        if (mLayerView.isFullScreen()) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FullScreen:Exit", null));
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

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (!GeckoAppShell.sActivityHelper.handleActivityResult(requestCode, resultCode, data)) {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    public LayerView getLayerView() {
        return mLayerView;
    }

    public AbsoluteLayout getPluginContainer() { return mPluginContainer; }

    
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createSensorEvent(event));
    }

    
    @Override
    public void onLocationChanged(Location location) {
        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createLocationEvent(location));
    }

    @Override
    public void onProviderDisabled(String provider)
    {
    }

    @Override
    public void onProviderEnabled(String provider)
    {
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras)
    {
    }

    
    public void notifyWakeLockChanged(String topic, String state) {
        PowerManager.WakeLock wl = mWakeLocks.get(topic);
        if (state.equals("locked-foreground") && wl == null) {
            PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, topic);
            wl.acquire();
            mWakeLocks.put(topic, wl);
        } else if (!state.equals("locked-foreground") && wl != null) {
            wl.release();
            mWakeLocks.remove(topic);
        }
    }

    public void notifyCheckUpdateResult(String result) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Update:CheckResult", result));
    }

    protected void connectGeckoLayerClient() {
        mLayerView.getLayerClient().notifyGeckoReady();

        mLayerView.setTouchIntercepter(this);
    }

    @Override
    public boolean onInterceptTouchEvent(View view, MotionEvent event) {
        return false;
    }

    @Override
    public boolean onTouch(View view, MotionEvent event) {
        if (event == null)
            return true;

        int action = event.getActionMasked();
        PointF point = new PointF(event.getX(), event.getY());
        if (action == MotionEvent.ACTION_DOWN) {
            mInitialTouchPoint = point;
        }

        if (mInitialTouchPoint != null && action == MotionEvent.ACTION_MOVE) {
            if (PointUtils.subtract(point, mInitialTouchPoint).length() <
                PanZoomController.PAN_THRESHOLD) {
                
                
                return true;
            } else {
                mInitialTouchPoint = null;
            }
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createMotionEvent(event));
        return true;
    }

    public static class MainLayout extends RelativeLayout {
        private TouchEventInterceptor mTouchEventInterceptor;

        public MainLayout(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public void setTouchEventInterceptor(TouchEventInterceptor interceptor) {
            mTouchEventInterceptor = interceptor;
        }

        @Override
        public boolean onInterceptTouchEvent(MotionEvent event) {
            if (mTouchEventInterceptor != null && mTouchEventInterceptor.onInterceptTouchEvent(this, event)) {
                return true;
            }
            return super.onInterceptTouchEvent(event);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (mTouchEventInterceptor != null && mTouchEventInterceptor.onTouch(this, event)) {
                return true;
            }
            return super.onTouchEvent(event);
        }

        @Override
        public void setDrawingCacheEnabled(boolean enabled) {
            
            
            
            super.setChildrenDrawnWithCacheEnabled(enabled);
        }
    }

    private class FullScreenHolder extends FrameLayout {

        public FullScreenHolder(Context ctx) {
            super(ctx);
        }

        @Override
        public void addView(View view, int index) {
            








            super.addView(view, index);

            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    mLayerView.hide();
                }
            });
        }

        






        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            if (event.isSystem()) {
                return super.onKeyDown(keyCode, event);
            }
            mFullScreenPluginView.onKeyDown(keyCode, event);
            return true;
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            if (event.isSystem()) {
                return super.onKeyUp(keyCode, event);
            }
            mFullScreenPluginView.onKeyUp(keyCode, event);
            return true;
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            return true;
        }

        @Override
        public boolean onTrackballEvent(MotionEvent event) {
            mFullScreenPluginView.onTrackballEvent(event);
            return true;
        }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.pasteandgo: {
                String text = GeckoAppShell.getClipboardText();
                if (text != null && !TextUtils.isEmpty(text)) {
                    Tabs.getInstance().loadUrl(text);
                }
                return true;
            }
            case R.id.site_settings: {
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Permissions:Get", null));
                return true;
            }
            case R.id.paste: {
                String text = GeckoAppShell.getClipboardText();
                if (text != null && !TextUtils.isEmpty(text)) {
                    showAwesomebar(AwesomeBar.Target.CURRENT_TAB, text);
                }
                return true;
            }
            case R.id.share: {
                shareCurrentUrl();
                return true;
            }
            case R.id.copyurl: {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    String url = tab.getURL();
                    if (url != null) {
                        GeckoAppShell.setClipboardText(url);
                    }
                }
                return true;
            }
            case R.id.add_to_launcher: {
                Tab tab = Tabs.getInstance().getSelectedTab();
                if (tab != null) {
                    String url = tab.getURL();
                    String title = tab.getDisplayTitle();
                    Bitmap favicon = tab.getFavicon();
                    if (url != null && title != null) {
                        GeckoAppShell.createShortcut(title, url, url, favicon == null ? null : favicon, "");
                    }
                }
                return true;
            }
        }
        return false;
    }

    protected NotificationClient makeNotificationClient() {
        
        
        return new AppNotificationClient(getApplicationContext());
    }
}
