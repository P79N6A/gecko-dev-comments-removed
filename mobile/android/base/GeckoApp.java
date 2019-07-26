




package org.mozilla.gecko;

import org.mozilla.gecko.DataReportingNotification;
import org.mozilla.gecko.background.announcements.AnnouncementsBroadcastService;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PluginLayer;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.GeckoMenuInflater;
import org.mozilla.gecko.menu.MenuPanel;
import org.mozilla.gecko.health.BrowserHealthRecorder;
import org.mozilla.gecko.health.BrowserHealthRecorder.SessionInformation;
import org.mozilla.gecko.updater.UpdateService;
import org.mozilla.gecko.updater.UpdateServiceHelper;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.GeckoEventResponder;
import org.mozilla.gecko.util.HardwareUtils;
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
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.graphics.Rect;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.location.Location;
import android.location.LocationListener;
import android.net.wifi.ScanResult;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.StrictMode;

import android.telephony.CellLocation;
import android.telephony.NeighboringCellInfo;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;
import android.telephony.gsm.GsmCellLocation;

import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Base64;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.Display;
import android.view.Gravity;
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
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
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
                           ContextGetter, GeckoAppShell.GeckoInterface
{
    private static final String LOGTAG = "GeckoApp";

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

    static private final String LOCATION_URL = "https://location.services.mozilla.com/v1/submit";

    protected RelativeLayout mMainLayout;
    protected RelativeLayout mGeckoLayout;
    public View getView() { return mGeckoLayout; }
    public SurfaceView mCameraView;
    public List<GeckoAppShell.AppStateListener> mAppStateListeners;
    private static GeckoApp sAppContext;
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

    private volatile BrowserHealthRecorder mHealthRecorder = null;

    private int mSignalStrenth;
    private PhoneStateListener mPhoneStateListener = null;
    private boolean mShouldReportGeoData = false;

    abstract public int getLayout();
    abstract public boolean hasTabsSideBar();
    abstract protected String getDefaultProfileName();

    private static final String RESTARTER_ACTION = "org.mozilla.gecko.restart";
    private static final String RESTARTER_CLASS = "org.mozilla.gecko.Restarter";

    @SuppressWarnings("serial")
    class SessionRestoreException extends Exception {
        public SessionRestoreException(Exception e) {
            super(e);
        }

        public SessionRestoreException(String message) {
            super(message);
        }
    }

    void toggleChrome(final boolean aShow) { }

    void focusChrome() { }

    public Context getContext() {
        return sAppContext;
    }

    public Activity getActivity() {
        return this;
    }

    public LocationListener getLocationListener() {
        if (mShouldReportGeoData && mPhoneStateListener == null) {
            mPhoneStateListener = new PhoneStateListener() {
                public void onSignalStrengthsChanged(SignalStrength signalStrength) {
                    setCurrentSignalStrenth(signalStrength);
                }
            };
            TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
            tm.listen(mPhoneStateListener, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        }
        return this;
    }

    public SensorEventListener getSensorEventListener() {
        return this;
    }

    public static SharedPreferences getAppSharedPreferences() {
        return GeckoApp.sAppContext.getSharedPreferences(PREFS_NAME, 0);
    }

    public SurfaceView getCameraView() {
        return mCameraView;
    }

    public void addAppStateListener(GeckoAppShell.AppStateListener listener) {
        mAppStateListeners.add(listener);
    }

    public void removeAppStateListener(GeckoAppShell.AppStateListener listener) {
        mAppStateListeners.remove(listener);
    }

    public FormAssistPopup getFormAssistPopup() {
        return mFormAssistPopup;
    }

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
            return new GeckoMenuInflater(sAppContext);
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
        
        if (!HardwareUtils.hasMenuButton())
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
                mMenuPanel = new MenuPanel(this, null);
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

            GeckoMenu gMenu = new GeckoMenu(this, null);
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
        
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            openOptionsMenu();
            return true;
        }

        return super.onKeyDown(keyCode, event);
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

    public void addTab() { }

    public void addPrivateTab() { }

    public void showNormalTabs() { }

    public void showPrivateTabs() { }

    public void showRemoteTabs() { }

    private void showTabs(TabsPanel.Panel panel) { }

    public void hideTabs() { }

    






    public boolean autoHideTabs() { return false; }

    public boolean areTabsShown() { return false; }

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
                geckoConnected();

                
                
                
                mHealthRecorder.recordGeckoStartupTime(mGeckoReadyStartupTimer.getElapsed());
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
                tab.setIsRTL(message.getBoolean("isRTL"));
                
                LayerView layerView = mLayerView;
                if (layerView != null && Tabs.getInstance().isSelectedTab(tab)) {
                    layerView.setZoomConstraints(tab.getZoomConstraints());
                    layerView.setIsRTL(tab.getIsRTL());
                }
            } else if (event.equals("Session:StatePurged")) {
                onStatePurged();
            } else if (event.equals("Bookmark:Insert")) {
                final String url = message.getString("url");
                final String title = message.getString("title");
                final Context context = this;
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(context, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        ThreadUtils.postToBackgroundThread(new Runnable() {
                            @Override
                            public void run() {
                                BrowserDB.addBookmark(GeckoApp.sAppContext.getContentResolver(), title, url);
                            }
                        });
                    }
                });
            } else if (event.equals("Accessibility:Event")) {
                GeckoAccessibility.sendAccessibilityEvent(message);
            } else if (event.equals("Accessibility:Ready")) {
                GeckoAccessibility.updateAccessibilitySettings(this);
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

    public String getResponse(JSONObject origMessage) {
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
                Toast.makeText(sAppContext, resId, duration).show();
            }
        });
    }

    void handleShowToast(final String message, final String duration) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Toast toast;
                if (duration.equals("long"))
                    toast = Toast.makeText(sAppContext, message, Toast.LENGTH_LONG);
                else
                    toast = Toast.makeText(sAppContext, message, Toast.LENGTH_SHORT);
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

    public void addPluginView(final View view, final Rect rect, final boolean isFullScreen) {
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

    public void removePluginView(final View view, final boolean isFullScreen) {
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
        final String progText = sAppContext.getString(R.string.wallpaper_progress);
        final String successText = sAppContext.getString(R.string.wallpaper_success);
        final String failureText = sAppContext.getString(R.string.wallpaper_fail);
        final String fileName = aSrc.substring(aSrc.lastIndexOf("/") + 1);
        final PendingIntent emptyIntent = PendingIntent.getActivity(sAppContext, 0, new Intent(), 0);
        final AlertNotification notification = new AlertNotification(sAppContext, fileName.hashCode(), 
                                R.drawable.alert_download, fileName, progText, System.currentTimeMillis(), null);
        notification.setLatestEventInfo(sAppContext, fileName, progText, emptyIntent );
        notification.flags |= Notification.FLAG_ONGOING_EVENT;
        notification.show();
        new UiAsyncTask<Void, Void, Boolean>(ThreadUtils.getBackgroundHandler()) {

            @Override
            protected Boolean doInBackground(Void... params) {
                WallpaperManager mgr = WallpaperManager.getInstance(sAppContext);
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
                        BitmapUtils.decodeByteArray(buf, options);
                        options.inSampleSize = getBitmapSampleSize(options, idealWidth, idealHeight);
                        options.inJustDecodeBounds = false;
                        image = BitmapUtils.decodeByteArray(buf, options);
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
                        BitmapUtils.decodeByteArray(imgBuffer, options);
                        options.inSampleSize = getBitmapSampleSize(options, idealWidth, idealHeight);
                        options.inJustDecodeBounds = false;
                        image = BitmapUtils.decodeByteArray(imgBuffer, options);
                    }
                    if(image != null) {
                        mgr.setBitmap(image);
                        return true;
                    } else {
                        return false;
                    }
                } catch(OutOfMemoryError ome) {
                    Log.e(LOGTAG, "Out of Memory when converting to byte array", ome);
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
                    notification.setLatestEventInfo(sAppContext, fileName, failureText, emptyIntent);
                } else {
                    notification.tickerText = successText;
                    notification.setLatestEventInfo(sAppContext, fileName, successText, emptyIntent);
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

    





    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        GeckoAppShell.registerGlobalExceptionHandler();

        
        if ("default".equals(AppConstants.MOZ_UPDATE_CHANNEL)) {
            enableStrictMode();
        }

        
        mJavaUiStartupTimer = new Telemetry.Timer("FENNEC_STARTUP_TIME_JAVAUI");
        mGeckoReadyStartupTimer = new Telemetry.Timer("FENNEC_STARTUP_TIME_GECKOREADY");

        String args = getIntent().getStringExtra("args");

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
        ((GeckoApplication)getApplication()).initialize();

        sAppContext = this;
        GeckoAppShell.setContextGetter(this);
        GeckoAppShell.setGeckoInterface(this);
        ThreadUtils.setUiThread(Thread.currentThread(), new Handler());

        Tabs.getInstance().attachToActivity(this);
        Favicons.getInstance().attachToContext(this);

        
        if (getLastCustomNonConfigurationInstance() != null) {
            
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

        LayoutInflater.from(this).setFactory(this);

        super.onCreate(savedInstanceState);

        mOrientation = getResources().getConfiguration().orientation;

        setContentView(getLayout());

        
        mGeckoLayout = (RelativeLayout) findViewById(R.id.gecko_layout);
        mMainLayout = (RelativeLayout) findViewById(R.id.main_layout);

        
        mTabsPanel = (TabsPanel) findViewById(R.id.tabs_panel);

        
        
        
        mRestoreMode = getSessionRestoreState(savedInstanceState);
        if (mRestoreMode == RESTORE_OOM) {
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
                final SharedPreferences prefs = GeckoApp.getAppSharedPreferences();

                SessionInformation previousSession = SessionInformation.fromSharedPrefs(prefs);
                if (previousSession.wasKilled()) {
                    Telemetry.HistogramAdd("FENNEC_WAS_KILLED", 1);
                }

                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_OOM_EXCEPTION, false);

                
                
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);

                editor.commit();

                
                
                
                final String profilePath = getProfile().getDir().getAbsolutePath();
                final EventDispatcher dispatcher = GeckoAppShell.getEventDispatcher();
                Log.i(LOGTAG, "Creating BrowserHealthRecorder.");
                mHealthRecorder = new BrowserHealthRecorder(sAppContext, profilePath, dispatcher,
                                                            previousSession);
            }
        });

        GeckoAppShell.setNotificationClient(makeNotificationClient());
    }

    protected void initializeChrome() {
        mDoorHangerPopup = new DoorHangerPopup(this, null);
        mPluginContainer = (AbsoluteLayout) findViewById(R.id.plugin_container);
        mFormAssistPopup = (FormAssistPopup) findViewById(R.id.form_assist_popup);

        if (mCameraView == null) {
            mCameraView = new SurfaceView(this);
            mCameraView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }

        if (mLayerView == null) {
            LayerView layerView = (LayerView) findViewById(R.id.layer_view);
            layerView.initializeView(GeckoAppShell.getEventDispatcher());
            mLayerView = layerView;
            
            GeckoAppShell.notifyIMEContext(GeckoEditableListener.IME_STATE_DISABLED, "", "", "");
        }
    }

    








    protected void loadStartupTab(String url) {
        if (url == null) {
            if (mRestoreMode == RESTORE_NONE) {
                
                
                Tab tab = Tabs.getInstance().loadUrl("about:home", Tabs.LOADURL_NEW_TAB);
            }
        } else {
            
            int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_USER_ENTERED;
            Tabs.getInstance().loadUrl(url, flags);
        }
    }

    private void initialize() {
        mInitialized = true;

        invalidateOptionsMenu();

        Intent intent = getIntent();
        String action = intent.getAction();

        String passedUri = null;
        String uri = getURIFromIntent(intent);
        if (uri != null && uri.length() > 0) {
            passedUri = uri;
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

        initializeChrome();

        
        String restoreMessage = null;
        if (mRestoreMode != RESTORE_NONE && !mIsRestoringActivity) {
            try {
                
                
                
                
                
                
                restoreMessage = restoreSessionTabs(isExternalURL);
            } catch (SessionRestoreException e) {
                
                Log.e(LOGTAG, "An error occurred during restore", e);
                mRestoreMode = RESTORE_NONE;
            }
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Session:Restore", restoreMessage));

        if (!mIsRestoringActivity) {
            loadStartupTab(isExternalURL ? passedUri : null);
        }

        if (mRestoreMode == RESTORE_OOM) {
            
            
            Tabs.getInstance().notifyListeners(null, Tabs.TabEvents.RESTORED);
        } else {
            
            getProfile().moveSessionFile();
        }

        if (mRestoreMode == RESTORE_NONE) {
            Tabs.getInstance().notifyListeners(null, Tabs.TabEvents.RESTORED);
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

        
        mAppStateListeners = new LinkedList<GeckoAppShell.AppStateListener>();

        
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

        mPromptService = new PromptService(this);

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

        PrefsHelper.getPref("app.geo.reportdata", new PrefsHelper.PrefHandlerBase() {
            @Override public void prefValue(String pref, int value) {
                if (value == 1)
                    mShouldReportGeoData = true;
                else
                    mShouldReportGeoData = false;
            }
        });

        
        
        mJavaUiStartupTimer.stop();
        final long javaDuration = mJavaUiStartupTimer.getElapsed();

        ThreadUtils.getBackgroundHandler().postDelayed(new Runnable() {
            @Override
            public void run() {
                mHealthRecorder.recordJavaStartupTime(javaDuration);

                
                
                checkMigrateSync();

                
                
                final Context context = GeckoApp.sAppContext;
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
            geckoConnected();
            GeckoAppShell.setLayerClient(mLayerView.getLayerClient());
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Viewport:Flush", null));
        }

        if (ACTION_ALERT_CALLBACK.equals(action)) {
            processAlertCallback(intent);
        }
    }

    private String restoreSessionTabs(final boolean isExternalURL) throws SessionRestoreException {
        try {
            String sessionString = getProfile().readSessionFile(false);
            if (sessionString == null) {
                throw new SessionRestoreException("Could not read from session file");
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

                        Tab tab = Tabs.getInstance().loadUrl(sessionTab.getUrl(), flags);
                        tab.updateTitle(sessionTab.getTitle());

                        try {
                            tabObject.put("tabId", tab.getId());
                        } catch (JSONException e) {
                            Log.e(LOGTAG, "JSON error", e);
                        }
                        tabs.put(tabObject);
                    }
                };

                if (mPrivateBrowsingSession == null) {
                    parser.parse(sessionString);
                } else {
                    parser.parse(sessionString, mPrivateBrowsingSession);
                }

                if (tabs.length() > 0) {
                    sessionString = new JSONObject().put("windows", new JSONArray().put(new JSONObject().put("tabs", tabs))).toString();
                } else {
                    throw new SessionRestoreException("No tabs could be read from session file");
                }
            }

            JSONObject restoreData = new JSONObject();
            restoreData.put("restoringOOM", mRestoreMode == RESTORE_OOM);
            restoreData.put("sessionString", sessionString);
            return restoreData.toString();

        } catch (JSONException e) {
            throw new SessionRestoreException(e);
        }
    }

    public GeckoProfile getProfile() {
        
        if (mProfile == null) {
            mProfile = GeckoProfile.get(this);
        }
        return mProfile;
    }

    protected int getSessionRestoreState(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            return RESTORE_OOM;
        }

        final SharedPreferences prefs = GeckoApp.getAppSharedPreferences();

        
        
        
        
        if (prefs.getBoolean(PREFS_CRASHED, false)) {
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    prefs.edit()
                         .putBoolean(GeckoApp.PREFS_CRASHED, false)
                         .commit();
                }
            });

            if (getProfile().shouldRestoreSession()) {
                return RESTORE_CRASH;
            }
        }
        return RESTORE_NONE;
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
        
        mMainLayout.addView(mCameraView, new AbsoluteLayout.LayoutParams(8, 16, 0, 0));
    }

    public void disableCameraView() {
        mMainLayout.removeView(mCameraView);
    }

    public String getDefaultUAString() {
        return HardwareUtils.isTablet() ? AppConstants.USER_AGENT_FENNEC_TABLET :
                                          AppConstants.USER_AGENT_FENNEC_MOBILE;
    }

    public String getUAStringForHost(String host) {
        
        
        
        if ("t.co".equals(host)) {
            return AppConstants.USER_AGENT_BOT_LIKE;
        }
        return getDefaultUAString();
    }

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

        
        GeckoAccessibility.updateAccessibilitySettings(this);

        if (mAppStateListeners != null) {
            for (GeckoAppShell.AppStateListener listener: mAppStateListeners) {
                listener.onResume();
            }
        }

        
        
        
        final long now = System.currentTimeMillis();
        final long realTime = android.os.SystemClock.elapsedRealtime();
        final BrowserHealthRecorder rec = mHealthRecorder;

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                
                
                SessionInformation currentSession = new SessionInformation(now, realTime);

                SharedPreferences prefs = GeckoApp.getAppSharedPreferences();
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
                currentSession.recordBegin(editor);
                editor.commit();

                if (rec != null) {
                    rec.setCurrentSession(currentSession);
                }
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
        final BrowserHealthRecorder rec = mHealthRecorder;

        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs = GeckoApp.getAppSharedPreferences();
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, true);
                if (rec != null) {
                    rec.recordSessionEnd("P", editor);
                }
                editor.commit();
            }
        });

        GeckoScreenOrientationListener.getInstance().stop();

        if (mAppStateListeners != null) {
            for(GeckoAppShell.AppStateListener listener: mAppStateListeners) {
                listener.onPause();
            }
        }

        super.onPause();
    }

    @Override
    public void onRestart()
    {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs = GeckoApp.getAppSharedPreferences();
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

        final BrowserHealthRecorder rec = mHealthRecorder;
        mHealthRecorder = null;
        if (rec != null) {
            
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    rec.close();
                }
            });
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
        File dir = sAppContext.getExternalFilesDir("temp");
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
    public Object onRetainCustomNonConfigurationInstance() {
        
        
        return Boolean.TRUE;
    } 

    public String getContentProcessName() {
        return AppConstants.MOZ_CHILD_PROCESS_NAME;
    }

    





    @Override
    public View onCreateView(String name, Context context, AttributeSet attrs) {
        View view = GeckoViewsFactory.getInstance().onCreateView(name, context, attrs);
        if (view == null) {
            view = super.onCreateView(name, context, attrs);
        }
        return view;
    }

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
        doRestart(RESTARTER_ACTION);
    }

    public void doRestart(String action) {
        Log.d(LOGTAG, "doRestart(\"" + action + "\")");
        try {
            Intent intent = new Intent(action);
            intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME, RESTARTER_CLASS);
            
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
            final GeckoApp app = GeckoApp.sAppContext;

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
                                ThreadUtils.postToUiThread(new Runnable() {
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
                                ThreadUtils.postToUiThread(new Runnable() {
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
            final GeckoApp app = GeckoApp.sAppContext;
            ProfileMigrator profileMigrator = new ProfileMigrator(app);
            if (!profileMigrator.hasSyncMigrated()) {
                profileMigrator.launchSyncPrefs();
            }
        }
    }

    public PromptService getPromptService() {
        return mPromptService;
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

        if (mLayerView != null && mLayerView.isFullScreen()) {
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
        if (mShouldReportGeoData)
            collectAndReportLocInfo(location);
    }

    public void setCurrentSignalStrenth(SignalStrength ss) {
        if (ss.isGsm())
            mSignalStrenth = ss.getGsmSignalStrength();
    }

    private int getCellInfo(JSONArray cellInfo) {
        TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
        if (tm == null)
            return TelephonyManager.PHONE_TYPE_NONE;
        List<NeighboringCellInfo> cells = tm.getNeighboringCellInfo();
        CellLocation cl = tm.getCellLocation();
        String mcc = "", mnc = "";
        if (cl instanceof GsmCellLocation) {
            JSONObject obj = new JSONObject();
            GsmCellLocation gcl = (GsmCellLocation)cl;
            try {
                obj.put("lac", gcl.getLac());
                obj.put("cid", gcl.getCid());
                obj.put("psc", gcl.getPsc());
                switch(tm.getNetworkType()) {
                case TelephonyManager.NETWORK_TYPE_GPRS:
                case TelephonyManager.NETWORK_TYPE_EDGE:
                    obj.put("radio", "gsm");
                    break;
                case TelephonyManager.NETWORK_TYPE_UMTS:
                case TelephonyManager.NETWORK_TYPE_HSDPA:
                case TelephonyManager.NETWORK_TYPE_HSUPA:
                case TelephonyManager.NETWORK_TYPE_HSPA:
                case TelephonyManager.NETWORK_TYPE_HSPAP:
                    obj.put("radio", "umts");
                    break;
                }
                String mcc_mnc = tm.getNetworkOperator();
                mcc = mcc_mnc.substring(0, 3);
                mnc = mcc_mnc.substring(3);
                obj.put("mcc", mcc);
                obj.put("mnc", mnc);
                obj.put("asu", mSignalStrenth);
            } catch(JSONException jsonex) {}
            cellInfo.put(obj);
        }
        if (cells != null) {
            for (NeighboringCellInfo nci : cells) {
                try {
                    JSONObject obj = new JSONObject();
                    obj.put("lac", nci.getLac());
                    obj.put("cid", nci.getCid());
                    obj.put("psc", nci.getPsc());
                    obj.put("mcc", mcc);
                    obj.put("mnc", mnc);

                    int dbm;
                    switch(nci.getNetworkType()) {
                    case TelephonyManager.NETWORK_TYPE_GPRS:
                    case TelephonyManager.NETWORK_TYPE_EDGE:
                        obj.put("radio", "gsm");
                        break;
                    case TelephonyManager.NETWORK_TYPE_UMTS:
                    case TelephonyManager.NETWORK_TYPE_HSDPA:
                    case TelephonyManager.NETWORK_TYPE_HSUPA:
                    case TelephonyManager.NETWORK_TYPE_HSPA:
                    case TelephonyManager.NETWORK_TYPE_HSPAP:
                        obj.put("radio", "umts");
                        break;
                    }

                    obj.put("asu", nci.getRssi());
                    cellInfo.put(obj);
                } catch(JSONException jsonex) {}
            }
        }
        return tm.getPhoneType();
    }


    
    
    private static final Set<Character> AD_HOC_HEX_VALUES =
      new HashSet<Character>(Arrays.asList('2','6', 'a', 'e', 'A', 'E'));
    private static final String OPTOUT_SSID_SUFFIX = "_nomap";

    private static boolean shouldLog(final ScanResult sr) {
        
        
        
        
        
        
        final char secondNybble = sr.BSSID.length() == 17 ? sr.BSSID.charAt(1) : ' ';

        if(AD_HOC_HEX_VALUES.contains(secondNybble)) {
            return false;

        } else if (sr.SSID != null && sr.SSID.endsWith(OPTOUT_SSID_SUFFIX)) {
            return false;
        } else {
            return true;
        }
    }


    private void collectAndReportLocInfo(Location location) {
        final JSONObject locInfo = new JSONObject();
        WifiManager wm = (WifiManager)getSystemService(Context.WIFI_SERVICE);
        wm.startScan();
        try {
            JSONArray cellInfo = new JSONArray();
            int radioType = getCellInfo(cellInfo);
            if (radioType == TelephonyManager.PHONE_TYPE_GSM)
                locInfo.put("radio", "gsm");
            else
                return; 
            locInfo.put("lon", location.getLongitude());
            locInfo.put("lat", location.getLatitude());
            locInfo.put("accuracy", (int)location.getAccuracy());
            locInfo.put("altitude", (int)location.getAltitude());
            DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm'Z'");
            locInfo.put("time", df.format(new Date(location.getTime())));
            locInfo.put("cell", cellInfo);

            MessageDigest digest = MessageDigest.getInstance("SHA-1");

            JSONArray wifiInfo = new JSONArray();
            List<ScanResult> aps = wm.getScanResults();
            for (ScanResult ap : aps) {
                if (!shouldLog(ap))
                    continue;
                StringBuilder sb = new StringBuilder();
                try {
                    byte[] result = digest.digest((ap.BSSID + ap.SSID).getBytes("UTF-8"));
                    for (byte b : result) sb.append(String.format("%02X", b));

                    JSONObject obj = new JSONObject();

                    obj.put("key", sb.toString());
                    obj.put("frequency", ap.frequency);
                    obj.put("signal", ap.level);
                    wifiInfo.put(obj);
                } catch (UnsupportedEncodingException uee) {
                    Log.w(LOGTAG, "can't encode the key", uee);
                }
            }
            locInfo.put("wifi", wifiInfo);
        } catch (JSONException jsonex) {
            Log.w(LOGTAG, "json exception", jsonex);
        } catch (NoSuchAlgorithmException nsae) {
            Log.w(LOGTAG, "can't creat a SHA1", nsae);
        }
        ThreadUtils.postToBackgroundThread(new Runnable() {
            public void run() {
                try {
                    URL url = new URL(LOCATION_URL);
                    HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
                    try {
                        urlConnection.setDoOutput(true);
                        JSONArray batch = new JSONArray();
                        batch.put(locInfo);
                        JSONObject wrapper = new JSONObject();
                        wrapper.put("items", batch);
                        byte[] bytes = wrapper.toString().getBytes();
                        urlConnection.setFixedLengthStreamingMode(bytes.length);
                        OutputStream out = new BufferedOutputStream(urlConnection.getOutputStream());
                        out.write(bytes);
                        out.flush();
                    } catch (JSONException jsonex) {
                        Log.e(LOGTAG, "error wrapping data as a batch", jsonex);
                    } catch (IOException ioex) {
                        Log.e(LOGTAG, "error submitting data", ioex);
                    } finally {
                        urlConnection.disconnect();
                    }
                } catch (IOException ioex) {
                    Log.e(LOGTAG, "error submitting data", ioex);
                }
            }
        });
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

    protected void geckoConnected() {
        mLayerView.geckoConnected();
    }

    public void setAccessibilityEnabled(boolean enabled) {
    }

    public static class MainLayout extends RelativeLayout {
        private TouchEventInterceptor mTouchEventInterceptor;
        private MotionEventInterceptor mMotionEventInterceptor;

        public MainLayout(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public void setTouchEventInterceptor(TouchEventInterceptor interceptor) {
            mTouchEventInterceptor = interceptor;
        }

        public void setMotionEventInterceptor(MotionEventInterceptor interceptor) {
            mMotionEventInterceptor = interceptor;
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
        public boolean onGenericMotionEvent(MotionEvent event) {
            if (mMotionEventInterceptor != null && mMotionEventInterceptor.onInterceptMotionEvent(this, event)) {
                return true;
            }
            return super.onGenericMotionEvent(event);
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

    protected NotificationClient makeNotificationClient() {
        
        
        return new AppNotificationClient(getApplicationContext());
    }
}
