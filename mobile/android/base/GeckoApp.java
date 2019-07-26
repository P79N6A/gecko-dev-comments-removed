




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PluginLayer;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.util.GeckoAsyncTask;
import org.mozilla.gecko.util.GeckoBackgroundThread;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.GeckoEventResponder;
import org.mozilla.gecko.GeckoAccessibility;
import org.mozilla.gecko.updater.UpdateServiceHelper;
import org.mozilla.gecko.updater.UpdateService;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
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
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.location.Location;
import android.location.LocationListener;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.PowerManager;
import android.os.StrictMode;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.widget.AbsoluteLayout;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

abstract public class GeckoApp
                extends GeckoActivity 
                implements GeckoEventListener, SensorEventListener, LocationListener,
                           GeckoApplication.ApplicationLifecycleCallbacks,
                           Tabs.OnTabsChangedListener, GeckoEventResponder
{
    private static final String LOGTAG = "GeckoApp";

    public static enum StartupMode {
        NORMAL,
        NEW_VERSION,
        NEW_PROFILE
    }

    public static final String ACTION_ALERT_CLICK   = "org.mozilla.gecko.ACTION_ALERT_CLICK";
    public static final String ACTION_ALERT_CLEAR   = "org.mozilla.gecko.ACTION_ALERT_CLEAR";
    public static final String ACTION_ALERT_CALLBACK = "org.mozilla.gecko.ACTION_ALERT_CALLBACK";
    public static final String ACTION_WEBAPP_PREFIX = "org.mozilla.gecko.WEBAPP";
    public static final String ACTION_DEBUG         = "org.mozilla.gecko.DEBUG";
    public static final String ACTION_BOOKMARK      = "org.mozilla.gecko.BOOKMARK";
    public static final String ACTION_LOAD          = "org.mozilla.gecko.LOAD";
    public static final String ACTION_INIT_PW       = "org.mozilla.gecko.INIT_PW";
    public static final String SAVED_STATE_TITLE         = "title";
    public static final String SAVED_STATE_IN_BACKGROUND = "inBackground";

    public static final String PREFS_NAME          = "GeckoApp";
    public static final String PREFS_OOM_EXCEPTION = "OOMException";
    public static final String PREFS_WAS_STOPPED   = "wasStopped";

    StartupMode mStartupMode = null;
    protected LinearLayout mMainLayout;
    protected RelativeLayout mGeckoLayout;
    public View getView() { return mGeckoLayout; }
    public SurfaceView cameraView;
    public static GeckoApp mAppContext;
    public boolean mDOMFullScreen = false;
    protected MenuPanel mMenuPanel;
    protected Menu mMenu;
    private static GeckoThread sGeckoThread;
    public Handler mMainHandler;
    private GeckoProfile mProfile;
    public static boolean sIsGeckoReady = false;
    public static int mOrientation;
    private boolean mIsRestoringActivity;
    private String mCurrentResponse = "";

    private GeckoBatteryManager mBatteryReceiver;
    private PromptService mPromptService;
    private Favicons mFavicons;
    private TextSelection mTextSelection;

    protected DoorHangerPopup mDoorHangerPopup;
    protected FormAssistPopup mFormAssistPopup;
    protected TabsPanel mTabsPanel;

    private LayerView mLayerView;
    private AbsoluteLayout mPluginContainer;

    private FullScreenHolder mFullScreenPluginContainer;
    private View mFullScreenPluginView;

    private HashMap<String, PowerManager.WakeLock> mWakeLocks = new HashMap<String, PowerManager.WakeLock>();

    protected int mRestoreMode = GeckoAppShell.RESTORE_NONE;
    protected boolean mInitialized = false;

    public enum LaunchState {Launching, WaitForDebugger,
                             Launched, GeckoRunning, GeckoExiting};
    private static LaunchState sLaunchState = LaunchState.Launching;

    abstract public int getLayout();
    abstract public boolean hasTabsSideBar();
    abstract protected String getDefaultProfileName();

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

    void toggleChrome(final Boolean aShow) { }

    void focusChrome() { }

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

    synchronized Favicons getFavicons() {
        if (mFavicons == null)
            mFavicons = new Favicons(this);

        return mFavicons;
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

    public View getMenuPanel() {
        return mMenuPanel;
    }

    
    public static class MenuPanel extends ScrollView {
        public MenuPanel(Context context, AttributeSet attrs) {
            super(context, attrs);
            setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                                                       ViewGroup.LayoutParams.WRAP_CONTENT));
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);

            
            DisplayMetrics metrics = GeckoApp.mAppContext.getResources().getDisplayMetrics();
            int restrictedHeightSpec = MeasureSpec.makeMeasureSpec((int) (0.75 * metrics.heightPixels), MeasureSpec.AT_MOST);

            super.onMeasure(widthMeasureSpec, restrictedHeightSpec);
        }

        @Override
        public boolean dispatchPopulateAccessibilityEvent (AccessibilityEvent event) {
            if (Build.VERSION.SDK_INT >= 14) 
                onPopulateAccessibilityEvent(event);
            return true;
        }
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
                synchronized(sLaunchState) {
                    if (sLaunchState == LaunchState.GeckoRunning)
                        GeckoAppShell.notifyGeckoOfEvent(
                            GeckoEvent.createBroadcastEvent("Browser:Quit", null));
                    else
                        System.exit(0);
                    sLaunchState = LaunchState.GeckoExiting;
                }
                return true;
            default:
                return super.onOptionsItemSelected(item);
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

    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        Log.i(LOGTAG, "onSaveInstanceState");

        if (outState == null)
            outState = new Bundle();

        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null)
            outState.putString(SAVED_STATE_TITLE, tab.getDisplayTitle());

        boolean inBackground =
            ((GeckoApplication)getApplication()).isApplicationInBackground();

        outState.putBoolean(SAVED_STATE_IN_BACKGROUND, inBackground);
    }

    void getAndProcessThumbnailForTab(final Tab tab) {
        if ("about:home".equals(tab.getURL())) {
            tab.updateThumbnail(null);
            return;
        }

        if (tab.getState() == Tab.STATE_DELAYED) {
            byte[] thumbnail = BrowserDB.getThumbnailForUrl(getContentResolver(), tab.getURL());
            if (thumbnail != null)
                processThumbnail(tab, null, thumbnail);
            return;
        }

        int dw = tab.getThumbnailWidth();
        int dh = tab.getThumbnailHeight();
        GeckoAppShell.sendEventToGecko(GeckoEvent.createScreenshotEvent(tab.getId(), 0, 0, 0, 0, 0, 0, dw, dh, dw, dh, ScreenshotHandler.SCREENSHOT_THUMBNAIL, tab.getThumbnailBuffer()));
    }

    void handleThumbnailData(Tab tab, ByteBuffer data) {
        if (shouldUpdateThumbnail(tab)) {
            Bitmap b = tab.getThumbnailBitmap();
            b.copyPixelsFromBuffer(data);
            processThumbnail(tab, b, null);
        }
    }

    void processThumbnail(Tab thumbnailTab, Bitmap bitmap, byte[] compressed) {
        try {
            if (bitmap == null) {
                if (compressed == null) {
                    Log.e(LOGTAG, "processThumbnail: one of bitmap or compressed must be non-null!");
                    return;
                }
                bitmap = BitmapFactory.decodeByteArray(compressed, 0, compressed.length);
            }
            thumbnailTab.updateThumbnail(bitmap);
        } catch (OutOfMemoryError ome) {
            Log.w(LOGTAG, "decoding byte array ran out of memory", ome);
        }
    }

    private boolean shouldUpdateThumbnail(Tab tab) {
        return (Tabs.getInstance().isSelectedTab(tab) || areTabsShown());
    }

    public void hideFormAssistPopup() {
        if (mFormAssistPopup != null)
            mFormAssistPopup.hide();
    }

    void handleSecurityChange(final int tabId, final JSONObject identityData) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.updateIdentityData(identityData);
    }

    void handleReaderEnabled(final int tabId) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.setReaderEnabled(true);
    }

    void handleFaviconRequest(final String url) {
        (new GeckoAsyncTask<Void, Void, String>(mAppContext, GeckoAppShell.getHandler()) {
            @Override
            public String doInBackground(Void... params) {
                return getFavicons().getFaviconUrlForPageUrl(url);
            }

            @Override
            public void onPostExecute(String faviconUrl) {
                JSONObject args = new JSONObject();

                if (faviconUrl != null) {
                    try {
                        args.put("url", url);
                        args.put("faviconUrl", faviconUrl);
                    } catch (JSONException e) {
                        Log.e(LOGTAG, "error building json arguments");
                    }
                }

                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:FaviconReturn", args.toString()));
            }
        }).execute();
    }

    void handleLoadError(final int tabId, final String uri, final String title) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        
        mMainHandler.post(new Runnable() {
            public void run() {
                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.LOAD_ERROR);
            }
        });
    }

    void handlePageShow(final int tabId) { }

    void handleClearHistory() {
        BrowserDB.clearHistory(getContentResolver());
        getFavicons().clearFavicons();
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

    void addTab() { }

    public void showLocalTabs() { }

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

    public void handleMessage(String event, JSONObject message) {
        Log.i(LOGTAG, "Got message: " + event);
        try {
            if (event.equals("Toast:Show")) {
                final String msg = message.getString("message");
                final String duration = message.getString("duration");
                handleShowToast(msg, duration);
            } else if (event.equals("DOMContentLoaded")) {
                final int tabId = message.getInt("tabID");
                final String backgroundColor = message.getString("bgColor");
                handleContentLoaded(tabId);
                Tab tab = Tabs.getInstance().getTab(tabId);
                if (backgroundColor != null) {
                    tab.setCheckerboardColor(backgroundColor);
                } else {
                    
                    tab.setCheckerboardColor(Color.WHITE);
                }

                
                
                LayerView layerView = mLayerView;
                if (layerView != null && Tabs.getInstance().isSelectedTab(tab)) {
                    layerView.setCheckerboardColor(tab.getCheckerboardColor());
                }
            } else if (event.equals("DOMTitleChanged")) {
                final int tabId = message.getInt("tabID");
                final String title = message.getString("title");
                handleTitleChanged(tabId, title);
            } else if (event.equals("DOMLinkAdded")) {
                final int tabId = message.getInt("tabID");
                final String rel = message.getString("rel");
                final String href = message.getString("href");
                final int size = message.getInt("size");
                handleLinkAdded(tabId, rel, href, size);
            } else if (event.equals("DOMWindowClose")) {
                final int tabId = message.getInt("tabID");
                handleWindowClose(tabId);
            } else if (event.equals("log")) {
                
                final String msg = message.getString("msg");
                Log.i(LOGTAG, "Log: " + msg);
            } else if (event.equals("Content:SecurityChange")) {
                final int tabId = message.getInt("tabID");
                final JSONObject identity = message.getJSONObject("identity");
                Log.i(LOGTAG, "Security Mode - " + identity.getString("mode"));
                handleSecurityChange(tabId, identity);
            } else if (event.equals("Content:ReaderEnabled")) {
                final int tabId = message.getInt("tabID");
                handleReaderEnabled(tabId);
            } else if (event.equals("Reader:FaviconRequest")) {
                final String url = message.getString("url");
                handleFaviconRequest(url);
            } else if (event.equals("Reader:GoToReadingList")) {
                showReadingList();
            } else if (event.equals("Content:StateChange")) {
                final int tabId = message.getInt("tabID");
                final String uri = message.getString("uri");
                final boolean success = message.getBoolean("success");
                int state = message.getInt("state");
                Log.i(LOGTAG, "State - " + state);
                if ((state & GeckoAppShell.WPL_STATE_IS_NETWORK) != 0) {
                    if ((state & GeckoAppShell.WPL_STATE_START) != 0) {
                        Log.i(LOGTAG, "Got a document start");
                        final boolean showProgress = message.getBoolean("showProgress");
                        handleDocumentStart(tabId, showProgress, uri);
                    } else if ((state & GeckoAppShell.WPL_STATE_STOP) != 0) {
                        Log.i(LOGTAG, "Got a document stop");
                        handleDocumentStop(tabId, success);
                    }
                }
            } else if (event.equals("Content:LoadError")) {
                final int tabId = message.getInt("tabID");
                final String uri = message.getString("uri");
                final String title = message.getString("title");
                handleLoadError(tabId, uri, title);
            } else if (event.equals("Content:PageShow")) {
                final int tabId = message.getInt("tabID");
                handlePageShow(tabId);
            } else if (event.equals("Gecko:Ready")) {
                sIsGeckoReady = true;
                setLaunchState(GeckoApp.LaunchState.GeckoRunning);
                GeckoAppShell.sendPendingEventsToGecko();
                connectGeckoLayerClient();
            } else if (event.equals("ToggleChrome:Hide")) {
                toggleChrome(false);
            } else if (event.equals("ToggleChrome:Show")) {
                toggleChrome(true);
            } else if (event.equals("ToggleChrome:Focus")) {
                focusChrome();
            } else if (event.equals("DOMFullScreen:Start")) {
                mDOMFullScreen = true;
            } else if (event.equals("DOMFullScreen:Stop")) {
                mDOMFullScreen = false;
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
            } else if (event.equals("Tab:HasTouchListener")) {
                int tabId = message.getInt("tabID");
                final Tab tab = Tabs.getInstance().getTab(tabId);
                tab.setHasTouchListeners(true);
                mMainHandler.post(new Runnable() {
                    public void run() {
                        if (Tabs.getInstance().isSelectedTab(tab))
                            mLayerView.getTouchEventHandler().setWaitForTouchListeners(true);
                    }
                });
            } else if (event.equals("Session:StatePurged")) {
                onStatePurged();
            } else if (event.equals("Bookmark:Insert")) {
                final String url = message.getString("url");
                final String title = message.getString("title");
                mMainHandler.post(new Runnable() {
                    public void run() {
                        Toast.makeText(GeckoApp.mAppContext, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        GeckoAppShell.getHandler().post(new Runnable() {
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
                String url = message.getString("uri");
                String origin = message.getString("origin");
                Intent intent = GeckoAppShell.getWebAppIntent(url, origin, false);
                if (intent == null)
                    return;
                startActivity(intent);
            } else if (event.equals("WebApps:Install")) {
                String name = message.getString("name");
                String launchPath = message.getString("launchPath");
                String iconURL = message.getString("iconURL");
                String uniqueURI = message.getString("uniqueURI");

                
                mCurrentResponse = GeckoAppShell.installWebApp(name, launchPath, uniqueURI, iconURL).toString();
            } else if (event.equals("WebApps:Uninstall")) {
                String uniqueURI = message.getString("uniqueURI");
                GeckoAppShell.uninstallWebApp(uniqueURI);
            } else if (event.equals("DesktopMode:Changed")) {
                int tabId = message.getInt("tabId");
                boolean desktopMode = message.getBoolean("desktopMode");
                final Tab tab = Tabs.getInstance().getTab(tabId);
                if (tab == null)
                    return;

                tab.setDesktopMode(desktopMode);
                mMainHandler.post(new Runnable() {
                    public void run() {
                        if (tab == Tabs.getInstance().getSelectedTab())
                            invalidateOptionsMenu();
                    }
                });
            } else if (event.equals("Share:Text")) {
                String text = message.getString("text");
                GeckoAppShell.openUriExternal(text, "text/plain", "", "", Intent.ACTION_SEND, "");
            } else if (event.equals("Share:Image")) {
                String src = message.getString("url");
                String type = message.getString("mime");
                GeckoAppShell.shareImage(src, type);
            } else if (event.equals("Sanitize:ClearHistory")) {
                handleClearHistory();
            } else if (event.equals("Update:Check")) {
                startService(new Intent(UpdateServiceHelper.ACTION_CHECK_FOR_UPDATE, null, this, UpdateService.class));
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public String getResponse() {
        Log.i(LOGTAG, "Return " + mCurrentResponse);
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
            
            
            CharSequence[] items = new CharSequence[aPermissions.length()];
            boolean[] states = new boolean[aPermissions.length()];
            for (int i = 0; i < aPermissions.length(); i++) {
                try {
                    items[i] = aPermissions.getJSONObject(i).
                               getString("setting");
                    
                    states[i] = true;
                } catch (JSONException e) {
                    Log.i(LOGTAG, "JSONException", e);
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
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Permissions:Clear", permissionsToClear.toString()));
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

    void handleDocumentStart(int tabId, final boolean showProgress, String uri) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.setState(shouldShowProgress(uri) ? Tab.STATE_SUCCESS : Tab.STATE_LOADING);
        tab.updateIdentityData(null);
        tab.setReaderEnabled(false);
        if (Tabs.getInstance().isSelectedTab(tab))
            mLayerView.getRenderer().resetCheckerboard();
        mMainHandler.post(new Runnable() {
            public void run() {
                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.START, showProgress);
            }
        });
    }

    void handleDocumentStop(int tabId, boolean success) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.setState(success ? Tab.STATE_SUCCESS : Tab.STATE_ERROR);

        mMainHandler.post(new Runnable() {
            public void run() {
                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.STOP);
            }
        });

        final String oldURL = tab.getURL();
        GeckoAppShell.getHandler().postDelayed(new Runnable() {
            public void run() {
                
                if (!TextUtils.equals(oldURL, tab.getURL()))
                    return;

                getAndProcessThumbnailForTab(tab);
                if (Tabs.getInstance().isSelectedTab(tab)) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createStartPaintListentingEvent(tab.getId()));
                    GeckoAppShell.screenshotWholePage(tab);
                }

            }
        }, 500);
    }

    public void showToast(final int resId, final int duration) {
        mMainHandler.post(new Runnable() {
            public void run() {
                Toast.makeText(mAppContext, resId, duration).show();
            }
        });
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

    void handleContentLoaded(int tabId) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        mMainHandler.post(new Runnable() {
            public void run() {
                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.LOADED);
            }
        });
    }

    void handleTitleChanged(int tabId, String title) {
        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.updateTitle(title);
    }

    void handleLinkAdded(final int tabId, String rel, final String href, int size) {
        if (rel.indexOf("[icon]") == -1)
            return; 

        final Tab tab = Tabs.getInstance().getTab(tabId);
        if (tab == null)
            return;

        tab.updateFaviconURL(href, size);
    }

    void handleWindowClose(final int tabId) {
        Tabs tabs = Tabs.getInstance();
        Tab tab = tabs.getTab(tabId);
        tabs.closeTab(tab);
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
        mMainHandler.post(new Runnable() { 
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

        
        
        mMainHandler.post(new Runnable() { 
            public void run() {
                mLayerView.setVisibility(View.VISIBLE);
            }
        });

        FrameLayout decor = (FrameLayout)getWindow().getDecorView();
        decor.removeView(mFullScreenPluginContainer);
        
        mFullScreenPluginView = null;

        GeckoScreenOrientationListener.getInstance().unlockScreenOrientation();
        setFullScreen(false);
    }

    void removePluginView(final View view, final boolean isFullScreen) {
        mMainHandler.post(new Runnable() { 
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
        mMainHandler.post(new Runnable() { 
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
        int screenLayout = getResources().getConfiguration().screenLayout;
        return (Build.VERSION.SDK_INT >= 11 &&
                (((screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) == Configuration.SCREENLAYOUT_SIZE_LARGE) || 
                 ((screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) == Configuration.SCREENLAYOUT_SIZE_XLARGE)));
    }

    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        GeckoAppShell.registerGlobalExceptionHandler();

        mAppContext = this;
        Tabs.getInstance().attachToActivity(this);

        
        if (getLastNonConfigurationInstance() != null) {
            
            doRestart();
            System.exit(0);
            return;
        }

        
        if (getResources().getBoolean(R.bool.enableStrictMode)) {
            enableStrictMode();
        }

        GeckoAppShell.loadMozGlue();
        if (sGeckoThread != null) {
            
            
            mIsRestoringActivity = true;
        }

        mMainHandler = new Handler();
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - onCreate");

        LayoutInflater.from(this).setFactory(GeckoViewsFactory.getInstance());

        super.onCreate(savedInstanceState);

        mOrientation = getResources().getConfiguration().orientation;

        setContentView(getLayout());

        
        mGeckoLayout = (RelativeLayout) findViewById(R.id.gecko_layout);
        mMainLayout = (LinearLayout) findViewById(R.id.main_layout);

        
        mTabsPanel = (TabsPanel) findViewById(R.id.tabs_panel);

        
        
        
        if (savedInstanceState != null) {
            Log.i(LOGTAG, "Restoring from OOM");
            mRestoreMode = GeckoAppShell.RESTORE_OOM;

            boolean wasInBackground =
                savedInstanceState.getBoolean(SAVED_STATE_IN_BACKGROUND, false);
            Log.i(LOGTAG, "Was in background: " + wasInBackground);

            if (!wasInBackground) {
                Telemetry.HistogramAdd("OUT_OF_MEMORY_KILLED", 1);
            }
        }

        GeckoBackgroundThread.getHandler().post(new Runnable() {
            public void run() {
                SharedPreferences prefs =
                    GeckoApp.mAppContext.getSharedPreferences(PREFS_NAME, 0);

                boolean wasOOM = prefs.getBoolean(PREFS_OOM_EXCEPTION, false);
                boolean wasStopped = prefs.getBoolean(PREFS_WAS_STOPPED, true);
                if (wasOOM || !wasStopped) {
                    Log.i(LOGTAG, "Crashed due to OOM last run");
                    Telemetry.HistogramAdd("OUT_OF_MEMORY_KILLED", 1);
                }
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_OOM_EXCEPTION, false);

                
                
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
                editor.commit();
            }
        });

        ((GeckoApplication)getApplication()).addApplicationLifecycleCallbacks(this);
    }

    void initializeChrome(String uri, Boolean isExternalURL) {
        mDoorHangerPopup = new DoorHangerPopup(this, null);
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

        if (mRestoreMode == GeckoAppShell.RESTORE_NONE && getProfile().shouldRestoreSession()) {
            Log.i(LOGTAG, "Restoring crash");
            mRestoreMode = GeckoAppShell.RESTORE_CRASH;
        }

        boolean isExternalURL = passedUri != null && !passedUri.equals("about:home");
        initializeChrome(uri, isExternalURL);

        
        
        checkMigrateProfile();

        Uri data = intent.getData();
        if (data != null && "http".equals(data.getScheme())) {
            Intent copy = new Intent(intent);
            copy.setAction(ACTION_LOAD);
            if (isHostOnRedirectWhitelist(data.getHost())) {
                GeckoAppShell.getHandler().post(new RedirectorRunnable(copy));
                
                
                
                intent.setAction(Intent.ACTION_MAIN);
                intent.setData(null);
                passedUri = "about:empty";
            } else {
                GeckoAppShell.getHandler().post(new PrefetchRunnable(copy));
            }
        }

        if (!mIsRestoringActivity) {
            sGeckoThread = new GeckoThread(intent, passedUri, mRestoreMode);
        }
        if (!ACTION_DEBUG.equals(action) &&
            checkAndSetLaunchState(LaunchState.Launching, LaunchState.Launched)) {
            sGeckoThread.start();
        } else if (ACTION_DEBUG.equals(action) &&
            checkAndSetLaunchState(LaunchState.Launching, LaunchState.WaitForDebugger)) {
            mMainHandler.postDelayed(new Runnable() {
                public void run() {
                    Log.i(LOGTAG, "Launching from debug intent after 5s wait");
                    setLaunchState(LaunchState.Launching);
                    sGeckoThread.start();
                }
            }, 1000 * 5 );
            Log.i(LOGTAG, "Intent : ACTION_DEBUG - waiting 5s before launching");
        }

        Tabs.registerOnTabsChangedListener(this);

        if (cameraView == null) {
            cameraView = new SurfaceView(this);
            cameraView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }

        if (mLayerView == null) {
            LayerView layerView = (LayerView) findViewById(R.id.layer_view);
            layerView.createLayerClient(GeckoAppShell.getEventDispatcher());
            mLayerView = layerView;
        }

        mPluginContainer = (AbsoluteLayout) findViewById(R.id.plugin_container);
        mFormAssistPopup = (FormAssistPopup) findViewById(R.id.form_assist_popup);

        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - UI almost up");

        
        registerEventListener("DOMContentLoaded");
        registerEventListener("DOMTitleChanged");
        registerEventListener("DOMLinkAdded");
        registerEventListener("DOMWindowClose");
        registerEventListener("log");
        registerEventListener("Content:SecurityChange");
        registerEventListener("Content:ReaderEnabled");
        registerEventListener("Content:StateChange");
        registerEventListener("Content:LoadError");
        registerEventListener("Content:PageShow");
        registerEventListener("Reader:FaviconRequest");
        registerEventListener("Reader:GoToReadingList");
        registerEventListener("onCameraCapture");
        registerEventListener("Menu:Add");
        registerEventListener("Menu:Remove");
        registerEventListener("Gecko:Ready");
        registerEventListener("Toast:Show");
        registerEventListener("DOMFullScreen:Start");
        registerEventListener("DOMFullScreen:Stop");
        registerEventListener("ToggleChrome:Hide");
        registerEventListener("ToggleChrome:Show");
        registerEventListener("ToggleChrome:Focus");
        registerEventListener("Permissions:Data");
        registerEventListener("Tab:HasTouchListener");
        registerEventListener("Tab:ViewportMetadata");
        registerEventListener("Session:StatePurged");
        registerEventListener("Bookmark:Insert");
        registerEventListener("Accessibility:Event");
        registerEventListener("Accessibility:Ready");
        registerEventListener("Shortcut:Remove");
        registerEventListener("WebApps:Open");
        registerEventListener("WebApps:Install");
        registerEventListener("WebApps:Uninstall");
        registerEventListener("DesktopMode:Changed");
        registerEventListener("Share:Text");
        registerEventListener("Share:Image");
        registerEventListener("Sanitize:ClearHistory");
        registerEventListener("Update:Check");

        if (SmsManager.getInstance() != null) {
          SmsManager.getInstance().start();
        }

        mBatteryReceiver = new GeckoBatteryManager();
        mBatteryReceiver.registerFor(mAppContext);

        GeckoConnectivityReceiver.getInstance().init(this);
        GeckoConnectivityReceiver.getInstance().start();

        mPromptService = new PromptService();

        mTextSelection = new TextSelection((TextSelectionHandle) findViewById(R.id.start_handle),
                                           (TextSelectionHandle) findViewById(R.id.end_handle),
                                           GeckoAppShell.getEventDispatcher());

        GeckoNetworkManager.getInstance().init(this);
        GeckoNetworkManager.getInstance().start();

        UpdateServiceHelper.registerForUpdates(this);

        GeckoScreenOrientationListener.getInstance().start();

        final GeckoApp self = this;

        GeckoAppShell.getHandler().postDelayed(new Runnable() {
            public void run() {
                Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - pre checkLaunchState");
                
                
                checkMigrateSync();

                








                if (!checkLaunchState(LaunchState.Launched)) {
                    return;
                }
            }
        }, 50);

        if (mIsRestoringActivity) {
            setLaunchState(GeckoApp.LaunchState.GeckoRunning);
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null)
                Tabs.getInstance().selectTab(selectedTab.getId());
            connectGeckoLayerClient();
            GeckoAppShell.setLayerClient(mLayerView.getLayerClient());
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Viewport:Flush", null));
        }
    }

    public GeckoProfile getProfile() {
        
        if (mProfile == null) {
            mProfile = GeckoProfile.get(this);
        }
        return mProfile;
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

    class PrefetchRunnable implements Runnable {
        Intent mIntent;
        protected HttpURLConnection mConnection = null;
        PrefetchRunnable(Intent intent) {
            mIntent = intent;
        }

        private void afterLoad() { }

        public void run() {
            try {
                
                URL url = new URL(mIntent.getData().toString());
                
                mConnection = (HttpURLConnection) url.openConnection();
                mConnection.setRequestProperty("User-Agent", getUAStringForHost(url.getHost()));
                mConnection.setInstanceFollowRedirects(false);
                mConnection.setRequestMethod("GET");
                mConnection.connect();
                afterLoad();
            } catch (Exception e) {
                Log.w(LOGTAG, "unexpected exception, passing url directly to Gecko but we should explicitly catch this", e);
                mIntent.putExtra("prefetched", 1);
            } finally {
                if (mConnection != null)
                    mConnection.disconnect();
            }
        }
    }

    class RedirectorRunnable extends PrefetchRunnable {
        RedirectorRunnable(Intent intent) {
            super(intent);
        }
        private void afterLoad() {
            try {
                int code = mConnection.getResponseCode();
                if (code >= 300 && code < 400) {
                    String location = mConnection.getHeaderField("Location");
                    Uri data;
                    if (location != null &&
                        (data = Uri.parse(location)) != null &&
                        !"about".equals(data.getScheme()) && 
                        !"chrome".equals(data.getScheme())) {
                        mIntent.setData(data);
                    } else {
                        mIntent.putExtra("prefetched", 1);
                    }
                } else {
                    mIntent.putExtra("prefetched", 1);
                }
            } catch (IOException ioe) {
                Log.i(LOGTAG, "exception trying to pre-fetch redirected url", ioe);
                mIntent.putExtra("prefetched", 1);
            }
        }
        public void run() {
            super.run();

            mMainHandler.postAtFrontOfQueue(new Runnable() {
                public void run() {
                    onNewIntent(mIntent);
                }
            });
        }
    }

    private final String kRedirectWhiteListArray[] = new String[] { 
        "t.co",
        "bit.ly",
        "moz.la",
        "aje.me",
        "facebook.com",
        "goo.gl",
        "tinyurl.com"
    };
    
    private final CopyOnWriteArrayList<String> kRedirectWhiteList =
        new CopyOnWriteArrayList<String>(kRedirectWhiteListArray);

    private boolean isHostOnRedirectWhitelist(String host) {
        return kRedirectWhiteList.contains(host);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - onNewIntent");

        if (checkLaunchState(LaunchState.GeckoExiting)) {
            
            
            System.exit(0);
            return;
        }

        
        
        if (!mInitialized) {
            setIntent(intent);
            return;
        }

        
        if ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) != 0)
            return;

        if (checkLaunchState(LaunchState.Launched)) {
            Uri data = intent.getData();
            Bundle bundle = intent.getExtras();
            
            
            
            
            if (data != null && 
                "http".equals(data.getScheme()) &&
                (bundle == null || bundle.getInt("prefetched", 0) != 1)) {
                if (isHostOnRedirectWhitelist(data.getHost())) {
                    GeckoAppShell.getHandler().post(new RedirectorRunnable(intent));
                    return;
                } else {
                    GeckoAppShell.getHandler().post(new PrefetchRunnable(intent));
                }
            }
        }
        final String action = intent.getAction();

        if (Intent.ACTION_MAIN.equals(action)) {
            Log.i(LOGTAG, "Intent : ACTION_MAIN");
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(""));
        }
        else if (ACTION_LOAD.equals(action)) {
            String uri = intent.getDataString();
            loadUrl(uri, AwesomeBar.Target.CURRENT_TAB);
        }
        else if (Intent.ACTION_VIEW.equals(action)) {
            String uri = intent.getDataString();
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(uri));
        }
        else if (action != null && action.startsWith(ACTION_WEBAPP_PREFIX)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createWebappLoadEvent(uri));
        }
        else if (ACTION_BOOKMARK.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBookmarkLoadEvent(uri));
        }
        else if (ACTION_ALERT_CALLBACK.equals(action)) {
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
        Log.i(LOGTAG, "resume");

        
        
        super.onResume();

        SiteIdentityPopup.getInstance().dismiss();

        int newOrientation = getResources().getConfiguration().orientation;

        if (mOrientation != newOrientation) {
            mOrientation = newOrientation;
            refreshChrome();
        }

        
        GeckoAccessibility.updateAccessibilitySettings();

        GeckoBackgroundThread.getHandler().post(new Runnable() {
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

        if (!mInitialized && hasFocus)
            initialize();
    }

    @Override
    public void onStop()
    {
        Log.i(LOGTAG, "stop");
        
        
        
        
        
        
        
        
        
        

        GeckoAppShell.sendEventToGecko(GeckoEvent.createStoppingEvent(isApplicationInBackground()));
        super.onStop();
    }

    @Override
    public void onPause()
    {
        Log.i(LOGTAG, "pause");

        
        
        
        GeckoBackgroundThread.getHandler().post(new Runnable() {
            public void run() {
                SharedPreferences prefs =
                    GeckoApp.mAppContext.getSharedPreferences(GeckoApp.PREFS_NAME, 0);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, true);
                editor.commit();
            }
        });

        super.onPause();
    }

    @Override
    public void onRestart()
    {
        Log.i(LOGTAG, "restart");

        GeckoBackgroundThread.getHandler().post(new Runnable() {
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
    public void onStart()
    {
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - onStart");

        Log.i(LOGTAG, "start");
        GeckoAppShell.sendEventToGecko(GeckoEvent.createStartEvent(isApplicationInBackground()));
        super.onStart();
    }

    @Override
    public void onDestroy()
    {
        Log.i(LOGTAG, "destroy");

        
        
        if (isFinishing())
            GeckoAppShell.sendEventToGecko(GeckoEvent.createShutdownEvent());

        unregisterEventListener("DOMContentLoaded");
        unregisterEventListener("DOMTitleChanged");
        unregisterEventListener("DOMLinkAdded");
        unregisterEventListener("DOMWindowClose");
        unregisterEventListener("log");
        unregisterEventListener("Content:SecurityChange");
        unregisterEventListener("Content:ReaderEnabled");
        unregisterEventListener("Content:StateChange");
        unregisterEventListener("Content:LoadError");
        unregisterEventListener("Content:PageShow");
        unregisterEventListener("Reader:FaviconRequest");
        unregisterEventListener("Reader:GoToReadingList");
        unregisterEventListener("onCameraCapture");
        unregisterEventListener("Menu:Add");
        unregisterEventListener("Menu:Remove");
        unregisterEventListener("Gecko:Ready");
        unregisterEventListener("Toast:Show");
        unregisterEventListener("DOMFullScreen:Start");
        unregisterEventListener("DOMFullScreen:Stop");
        unregisterEventListener("ToggleChrome:Hide");
        unregisterEventListener("ToggleChrome:Show");
        unregisterEventListener("ToggleChrome:Focus");
        unregisterEventListener("Permissions:Data");
        unregisterEventListener("Tab:HasTouchListener");
        unregisterEventListener("Tab:ViewportMetadata");
        unregisterEventListener("Session:StatePurged");
        unregisterEventListener("Bookmark:Insert");
        unregisterEventListener("Accessibility:Event");
        unregisterEventListener("Accessibility:Ready");
        unregisterEventListener("Shortcut:Remove");
        unregisterEventListener("WebApps:Open");
        unregisterEventListener("WebApps:Install");
        unregisterEventListener("WebApps:Uninstall");
        unregisterEventListener("DesktopMode:Changed");
        unregisterEventListener("Share:Text");
        unregisterEventListener("Share:Image");
        unregisterEventListener("Sanitize:ClearHistory");
        unregisterEventListener("Update:Check");

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

        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                if (mFavicons != null)
                    mFavicons.close();
            }
        });

        if (SmsManager.getInstance() != null) {
            SmsManager.getInstance().stop();
            if (isFinishing())
                SmsManager.getInstance().shutdown();
        }

        super.onDestroy();

        if (mBatteryReceiver != null)
            mBatteryReceiver.unregisterFor(mAppContext);

        Tabs.unregisterOnTabsChangedListener(this);

        ((GeckoApplication) getApplication()).removeApplicationLifecycleCallbacks(this);
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
    public void onConfigurationChanged(Configuration newConfig)
    {
        Log.i(LOGTAG, "configuration changed");

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
    public void onLowMemory()
    {
        Log.e(LOGTAG, "low memory");
        if (checkLaunchState(LaunchState.GeckoRunning))
            GeckoAppShell.onLowMemory();
        super.onLowMemory();
        GeckoAppShell.geckoEventSync();
    }

    @Override
    public void onApplicationPause() {
        Log.i(LOGTAG, "application paused");
        GeckoAppShell.sendEventToGecko(GeckoEvent.createPauseEvent(true));

        GeckoConnectivityReceiver.getInstance().stop();
        GeckoNetworkManager.getInstance().stop();
        GeckoScreenOrientationListener.getInstance().stop();
    }

    @Override
    public void onApplicationResume() {
        Log.i(LOGTAG, "application resumed");
        if (checkLaunchState(LaunchState.GeckoRunning))
            GeckoAppShell.sendEventToGecko(GeckoEvent.createResumeEvent(true));

        GeckoConnectivityReceiver.getInstance().start();
        GeckoNetworkManager.getInstance().start();
        GeckoScreenOrientationListener.getInstance().start();
    }

    @Override
    public Object onRetainNonConfigurationInstance() {
        
        
        return new Boolean(true);
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
            Log.e(LOGTAG, "error doing restart", e);
        }
        finish();
        
        GeckoAppShell.waitForAnotherGeckoProc();
    }

    public void handleNotification(String action, String alertName, String alertCookie) {
        GeckoAppShell.handleNotification(action, alertName, alertCookie);
    }

    private void checkMigrateProfile() {
        final File profileDir = getProfile().getDir();
        final long currentTime = SystemClock.uptimeMillis();

        if (profileDir != null) {
            final GeckoApp app = GeckoApp.mAppContext;

            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    Log.i(LOGTAG, "Checking profile migration in: " + profileDir.getAbsolutePath());

                    ProfileMigrator profileMigrator = new ProfileMigrator(app);

                    
                    if (!profileMigrator.hasMigrationRun()) {
                        
                        
                        final SetupScreen setupScreen = new SetupScreen(app);

                        final Runnable startCallback = new Runnable() {
                            public void run() {
                                GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                                    public void run() {
                                       setupScreen.show();
                                    }
                                });
                            }
                        };

                        final Runnable stopCallback = new Runnable() {
                            public void run() {
                                GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                                    public void run() {
                                        setupScreen.dismiss();
                                    }
                                });
                            }
                        };

                        profileMigrator.setLongOperationCallbacks(startCallback,
                                                                  stopCallback);
                        profileMigrator.launchPlaces(profileDir);

                        long timeDiff = SystemClock.uptimeMillis() - currentTime;
                        Log.i(LOGTAG, "Profile migration took " + timeDiff + " ms");

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
        if (profileDir != null) {
            final GeckoApp app = GeckoApp.mAppContext;
            ProfileMigrator profileMigrator = new ProfileMigrator(app);
            if (!profileMigrator.hasSyncMigrated()) {
                Log.i(LOGTAG, "Checking Sync settings in: " + profileDir.getAbsolutePath());
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
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_NO_HISTORY);
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

        if (mDOMFullScreen) {
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

    
    
    protected void loadRequest(String url, AwesomeBar.Target target, String searchEngine, boolean userEntered) {
        Log.d(LOGTAG, target.name());
        JSONObject args = new JSONObject();
        try {
            args.put("url", url);
            args.put("engine", searchEngine);
            args.put("userEntered", userEntered);
        } catch (Exception e) {
            Log.e(LOGTAG, "error building JSON arguments");
        }

        if (target == AwesomeBar.Target.NEW_TAB) {
            Log.d(LOGTAG, "Sending message to Gecko: " + SystemClock.uptimeMillis() + " - Tab:Add");
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Add", args.toString()));
        } else {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Load", args.toString()));
        }
    }

    public void loadUrl(String url) {
        loadRequest(url, AwesomeBar.Target.CURRENT_TAB, null, false);
    }

    public void loadUrl(String url, AwesomeBar.Target target) {
        loadRequest(url, target, null, false);
    }

    





    public void loadUrlInTab(String url) {
        Tabs tabsInstance = Tabs.getInstance();
        Iterable<Tab> tabs = tabsInstance.getTabsInOrder();
        for (Tab tab : tabs) {
            if (url.equals(tab.getURL())) {
                tabsInstance.selectTab(tab.getId());
                return;
            }
        }

        JSONObject args = new JSONObject();
        try {
            args.put("url", url);
            args.put("parentId", tabsInstance.getSelectedTab().getId());
        } catch (Exception e) {
            Log.e(LOGTAG, "error building JSON arguments");
        }
        Log.i(LOGTAG, "Sending message to Gecko: " + SystemClock.uptimeMillis() + " - Tab:Add");
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Add", args.toString()));
    }

    public LayerView getLayerView() {
        return mLayerView;
    }

    public AbsoluteLayout getPluginContainer() { return mPluginContainer; }

    
    public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    public void onSensorChanged(SensorEvent event)
    {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createSensorEvent(event));
    }

    
    public void onLocationChanged(Location location)
    {
        Log.w(LOGTAG, "onLocationChanged "+location);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createLocationEvent(location));
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

    public void notifyCheckUpdateResult(boolean result) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Update:CheckResult", result ? "true" : "false"));
    }

    protected void connectGeckoLayerClient() {
        mLayerView.getLayerClient().notifyGeckoReady();

        mLayerView.getTouchEventHandler().setOnTouchListener(new ContentTouchListener() {
            private PointF initialPoint = null;

            @Override
            public boolean onTouch(View view, MotionEvent event) {
                if (event == null)
                    return true;

                if (super.onTouch(view, event))
                    return true;

                int action = event.getAction();
                PointF point = new PointF(event.getX(), event.getY());
                if ((action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_DOWN) {
                    initialPoint = point;
                }

                if (initialPoint != null && (action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_MOVE) {
                    if (PointUtils.subtract(point, initialPoint).length() < PanZoomController.PAN_THRESHOLD) {
                        
                        
                        return true;
                    } else {
                        initialPoint = null;
                    }
                }

                GeckoAppShell.sendEventToGecko(GeckoEvent.createMotionEvent(event));
                return true;
            }
        });
    }

    protected class ContentTouchListener implements OnInterceptTouchListener {
        private boolean mIsHidingTabs = false;

        @Override
        public boolean onInterceptTouchEvent(View view, MotionEvent event) {
            
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN && autoHideTabs()) {
                mIsHidingTabs = true;
                return true;
            }
            return false;
        }

        @Override
        public boolean onTouch(View view, MotionEvent event) {
            if (mIsHidingTabs) {
                
                int action = event.getActionMasked();
                if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                    mIsHidingTabs = false;
                }
                return true;
            }
            return false;
        }
    }

    public boolean linkerExtract() {
        return false;
    }

    private class FullScreenHolder extends FrameLayout {

        public FullScreenHolder(Context ctx) {
            super(ctx);
        }

        @Override
        public void addView(View view, int index) {
            








            super.addView(view, index);

            mMainHandler.post(new Runnable() { 
                public void run() {
                    mLayerView.setVisibility(View.INVISIBLE);
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
                    loadUrl(text, AwesomeBar.Target.CURRENT_TAB);
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
                    BitmapDrawable favicon = (BitmapDrawable)(tab.getFavicon());
                    if (url != null && title != null) {
                        GeckoAppShell.createShortcut(title, url, url, favicon == null ? null : favicon.getBitmap(), "");
                    }
                }
                return true;
            }
        }
        return false;
    }

    public static boolean shouldShowProgress(String url) {
        return "about:home".equals(url) || ReaderModeUtils.isAboutReader(url);
    }

    public static void assertOnUiThread() {
        Thread uiThread = mAppContext.getMainLooper().getThread();
        assertOnThread(uiThread);
    }

    public static void assertOnGeckoThread() {
        assertOnThread(sGeckoThread);
    }

    private static void assertOnThread(Thread expectedThread) {
        Thread currentThread = Thread.currentThread();
        long currentThreadId = currentThread.getId();
        long expectedThreadId = expectedThread.getId();

        if (currentThreadId != expectedThreadId) {
            throw new IllegalThreadStateException("Expected thread " + expectedThreadId + " (\""
                                                  + expectedThread.getName()
                                                  + "\"), but running on thread " + currentThreadId
                                                  + " (\"" + currentThread.getName() + ")");
        }
    }
}
