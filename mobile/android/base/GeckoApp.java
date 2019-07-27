




package org.mozilla.gecko;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.GeckoProfileDirectories.NoMozillaDirectoryException;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.gfx.FullScreenState;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PluginLayer;
import org.mozilla.gecko.health.HealthRecorder;
import org.mozilla.gecko.health.SessionInformation;
import org.mozilla.gecko.health.StubbedHealthRecorder;
import org.mozilla.gecko.menu.GeckoMenu;
import org.mozilla.gecko.menu.GeckoMenuInflater;
import org.mozilla.gecko.menu.MenuPanel;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.mozglue.ContextUtils.SafeIntent;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.preferences.ClearOnShutdownPref;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.prompts.PromptService;
import org.mozilla.gecko.tabqueue.TabQueueHelper;
import org.mozilla.gecko.updater.UpdateServiceHelper;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.ActivityUtils;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.FileUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.PrefUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.webapp.EventListener;
import org.mozilla.gecko.webapp.UninstallListener;
import org.mozilla.gecko.widget.ButtonToast;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.location.Location;
import android.location.LocationListener;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.PowerManager;
import android.os.Process;
import android.os.StrictMode;
import android.provider.ContactsContract;
import android.provider.MediaStore.Images.Media;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Base64;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.Window;
import android.widget.AbsoluteLayout;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

public abstract class GeckoApp
    extends GeckoActivity
    implements
    ContextGetter,
    GeckoAppShell.GeckoInterface,
    GeckoEventListener,
    GeckoMenu.Callback,
    GeckoMenu.MenuPresenter,
    LocationListener,
    NativeEventListener,
    SensorEventListener,
    Tabs.OnTabsChangedListener {

    private static final String LOGTAG = "GeckoApp";
    private static final int ONE_DAY_MS = 1000*60*60*24;

    private static enum StartupAction {
        NORMAL,     
        URL,        
        PREFETCH    
    }

    public static final String ACTION_ALERT_CALLBACK       = "org.mozilla.gecko.ACTION_ALERT_CALLBACK";
    public static final String ACTION_HOMESCREEN_SHORTCUT  = "org.mozilla.gecko.BOOKMARK";
    public static final String ACTION_DEBUG                = "org.mozilla.gecko.DEBUG";
    public static final String ACTION_LAUNCH_SETTINGS      = "org.mozilla.gecko.SETTINGS";
    public static final String ACTION_LOAD                 = "org.mozilla.gecko.LOAD";
    public static final String ACTION_INIT_PW              = "org.mozilla.gecko.INIT_PW";

    public static final String EXTRA_STATE_BUNDLE          = "stateBundle";

    public static final String PREFS_ALLOW_STATE_BUNDLE    = "allowStateBundle";
    public static final String PREFS_OOM_EXCEPTION         = "OOMException";
    public static final String PREFS_VERSION_CODE          = "versionCode";
    public static final String PREFS_WAS_STOPPED           = "wasStopped";
    public static final String PREFS_CRASHED               = "crashed";
    public static final String PREFS_CLEANUP_TEMP_FILES    = "cleanupTempFiles";

    public static final String SAVED_STATE_IN_BACKGROUND   = "inBackground";
    public static final String SAVED_STATE_PRIVATE_SESSION = "privateSession";

    
    
    private static final int CLEANUP_DEFERRAL_SECONDS = 15;

    protected OuterLayout mRootLayout;
    protected RelativeLayout mMainLayout;

    protected RelativeLayout mGeckoLayout;
    private View mCameraView;
    private OrientationEventListener mCameraOrientationEventListener;
    public List<GeckoAppShell.AppStateListener> mAppStateListeners = new LinkedList<GeckoAppShell.AppStateListener>();
    protected MenuPanel mMenuPanel;
    protected Menu mMenu;
    protected GeckoProfile mProfile;
    protected boolean mIsRestoringActivity;

    private ContactService mContactService;
    private PromptService mPromptService;
    private TextSelection mTextSelection;

    protected DoorHangerPopup mDoorHangerPopup;
    protected FormAssistPopup mFormAssistPopup;
    protected ButtonToast mToast;

    protected LayerView mLayerView;
    private AbsoluteLayout mPluginContainer;

    private FullScreenHolder mFullScreenPluginContainer;
    private View mFullScreenPluginView;

    private final HashMap<String, PowerManager.WakeLock> mWakeLocks = new HashMap<String, PowerManager.WakeLock>();

    protected boolean mShouldRestore;
    protected boolean mInitialized;
    private Telemetry.Timer mJavaUiStartupTimer;
    private Telemetry.Timer mGeckoReadyStartupTimer;

    private String mPrivateBrowsingSession;

    private volatile HealthRecorder mHealthRecorder;
    private volatile Locale mLastLocale;

    private EventListener mWebappEventListener;

    private Intent mRestartIntent;

    abstract public int getLayout();

    abstract protected String getDefaultProfileName() throws NoMozillaDirectoryException;

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

    @Override
    public Context getContext() {
        return this;
    }

    @Override
    public SharedPreferences getSharedPreferences() {
        return GeckoSharedPrefs.forApp(this);
    }

    @Override
    public Activity getActivity() {
        return this;
    }

    @Override
    public LocationListener getLocationListener() {
        return this;
    }

    @Override
    public SensorEventListener getSensorEventListener() {
        return this;
    }

    @Override
    public View getCameraView() {
        return mCameraView;
    }

    @Override
    public void addAppStateListener(GeckoAppShell.AppStateListener listener) {
        mAppStateListeners.add(listener);
    }

    @Override
    public void removeAppStateListener(GeckoAppShell.AppStateListener listener) {
        mAppStateListeners.remove(listener);
    }

    @Override
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
        if (mMenu == null) {
            return;
        }

        onPrepareOptionsMenu(mMenu);

        if (Versions.feature11Plus) {
            super.invalidateOptionsMenu();
        }
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
        if (Versions.feature11Plus) {
            return new GeckoMenuInflater(this);
        } else {
            return super.getMenuInflater();
        }
    }

    public MenuPanel getMenuPanel() {
        if (mMenuPanel == null) {
            onCreatePanelMenu(Window.FEATURE_OPTIONS_PANEL, null);
            invalidateOptionsMenu();
        }
        return mMenuPanel;
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        return onOptionsItemSelected(item);
    }

    @Override
    public boolean onMenuItemLongClick(MenuItem item) {
        return false;
    }

    @Override
    public void openMenu() {
        openOptionsMenu();
    }

    @Override
    public void showMenu(final View menu) {
        
        
        closeMenu();

        
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                mMenuPanel.removeAllViews();
                mMenuPanel.addView(menu);
                openOptionsMenu();
            }
        });
    }

    @Override
    public void closeMenu() {
        closeOptionsMenu();
    }

    @Override
    public View onCreatePanelView(int featureId) {
        if (Versions.feature11Plus && featureId == Window.FEATURE_OPTIONS_PANEL) {
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
        if (Versions.feature11Plus && featureId == Window.FEATURE_OPTIONS_PANEL) {
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
        if (Versions.feature11Plus && featureId == Window.FEATURE_OPTIONS_PANEL) {
            return onPrepareOptionsMenu(menu);
        }

        return super.onPreparePanel(featureId, view, menu);
    }

    @Override
    public boolean onMenuOpened(int featureId, Menu menu) {
        
        if (mLayerView != null && mLayerView.isFullScreen()) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("FullScreen:Exit", null));
        }

        if (Versions.feature11Plus && featureId == Window.FEATURE_OPTIONS_PANEL) {
            if (mMenu == null) {
                
                MenuPanel panel = getMenuPanel();
                onPreparePanel(featureId, panel, mMenu);
            }

            
            if (mMenuPanel != null)
                mMenuPanel.scrollTo(0, 0);

            return true;
        }

        return super.onMenuOpened(featureId, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.quit) {
            
            GuestSession.hideNotification(this);

            final SharedPreferences prefs = GeckoSharedPrefs.forProfile(this);
            final Set<String> clearSet =
                    PrefUtils.getStringSet(prefs, ClearOnShutdownPref.PREF, new HashSet<String>());

            final JSONObject clearObj = new JSONObject();
            for (String clear : clearSet) {
                try {
                    clearObj.put(clear, true);
                } catch(JSONException ex) {
                    Log.e(LOGTAG, "Error adding clear object " + clear, ex);
                }
            }

            final JSONObject res = new JSONObject();
            try {
                res.put("sanitize", clearObj);
            } catch(JSONException ex) {
                Log.e(LOGTAG, "Error adding sanitize object", ex);
            }

            
            
            if (clearObj.has("private.data.history")) {
                final String sessionRestore = getSessionRestorePreference();
                try {
                    res.put("dontSaveSession", "quit".equals(sessionRestore));
                } catch(JSONException ex) {
                    Log.e(LOGTAG, "Error adding session restore data", ex);
                }
            }

            GeckoAppShell.sendEventToGeckoSync(
                    GeckoEvent.createBroadcastEvent("Browser:Quit", res.toString()));
            doShutdown();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onOptionsMenuClosed(Menu menu) {
        if (Versions.feature11Plus) {
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

        outState.putBoolean(SAVED_STATE_IN_BACKGROUND, isApplicationInBackground());
        outState.putString(SAVED_STATE_PRIVATE_SESSION, mPrivateBrowsingSession);
    }

    public void addTab() { }

    public void addPrivateTab() { }

    public void showNormalTabs() { }

    public void showPrivateTabs() { }

    public void hideTabs() { }

    






    public boolean autoHideTabs() { return false; }

    @Override
    public boolean areTabsShown() { return false; }

    @Override
    public void handleMessage(final String event, final NativeJSObject message,
                              final EventCallback callback) {
        if ("Accessibility:Ready".equals(event)) {
            GeckoAccessibility.updateAccessibilitySettings(this);

        } else if ("Bookmark:Insert".equals(event)) {
            final String url = message.getString("url");
            final String title = message.getString("title");
            final Context context = this;
            final BrowserDB db = getProfile().getDB();
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    db.addBookmark(getContentResolver(), title, url);
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(context, R.string.bookmark_added, Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            });

        } else if ("Contact:Add".equals(event)) {
            final String email = message.optString("email", null);
            final String phone = message.optString("phone", null);
            if (email != null) {
                Uri contactUri = Uri.parse(email);
                Intent i = new Intent(ContactsContract.Intents.SHOW_OR_CREATE_CONTACT, contactUri);
                startActivity(i);
            } else if (phone != null) {
                Uri contactUri = Uri.parse(phone);
                Intent i = new Intent(ContactsContract.Intents.SHOW_OR_CREATE_CONTACT, contactUri);
                startActivity(i);
            } else {
                
                Log.e(LOGTAG, "Received Contact:Add message with no email nor phone number");
            }

        } else if ("DOMFullScreen:Start".equals(event)) {
            
            LayerView layerView = mLayerView;
            if (layerView != null) {
                layerView.setFullScreenState(message.getBoolean("rootElement")
                        ? FullScreenState.ROOT_ELEMENT : FullScreenState.NON_ROOT_ELEMENT);
            }

        } else if ("DOMFullScreen:Stop".equals(event)) {
            
            LayerView layerView = mLayerView;
            if (layerView != null) {
                layerView.setFullScreenState(FullScreenState.NONE);
            }

        } else if ("Image:SetAs".equals(event)) {
            String src = message.getString("url");
            setImageAs(src);

        } else if ("Locale:Set".equals(event)) {
            setLocale(message.getString("locale"));

        } else if ("Permissions:Data".equals(event)) {
            String host = message.getString("host");
            final NativeJSObject[] permissions = message.getObjectArray("permissions");
            showSiteSettingsDialog(host, permissions);

        } else if ("PrivateBrowsing:Data".equals(event)) {
            mPrivateBrowsingSession = message.optString("session", null);

        } else if ("Session:StatePurged".equals(event)) {
            onStatePurged();

        } else if ("Share:Text".equals(event)) {
            String text = message.getString("text");
            GeckoAppShell.openUriExternal(text, "text/plain", "", "", Intent.ACTION_SEND, "");

            
            Telemetry.sendUIEvent(TelemetryContract.Event.SHARE, TelemetryContract.Method.LIST);

        } else if ("SystemUI:Visibility".equals(event)) {
            setSystemUiVisible(message.getBoolean("visible"));

        } else if ("Toast:Show".equals(event)) {
            final String msg = message.getString("message");
            final String duration = message.getString("duration");
            final NativeJSObject button = message.optObject("button", null);
            if (button != null) {
                final String label = button.optString("label", "");
                final String icon = button.optString("icon", "");
                final String id = button.optString("id", "");
                showButtonToast(msg, duration, label, icon, id);
            } else {
                showNormalToast(msg, duration);
            }

        } else if ("ToggleChrome:Focus".equals(event)) {
            focusChrome();

        } else if ("ToggleChrome:Hide".equals(event)) {
            toggleChrome(false);

        } else if ("ToggleChrome:Show".equals(event)) {
            toggleChrome(true);

        } else if ("Update:Check".equals(event)) {
            UpdateServiceHelper.checkForUpdate(this);
        } else if ("Update:Download".equals(event)) {
            UpdateServiceHelper.downloadUpdate(this);
        } else if ("Update:Install".equals(event)) {
            UpdateServiceHelper.applyUpdate(this);
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Gecko:DelayedStartup")) {
                ThreadUtils.postToBackgroundThread(new UninstallListener.DelayedStartupTask(this));
            } else if (event.equals("Gecko:Ready")) {
                mGeckoReadyStartupTimer.stop();
                geckoConnected();

                
                
                
                
                final HealthRecorder rec = mHealthRecorder;
                if (rec != null) {
                  rec.recordGeckoStartupTime(mGeckoReadyStartupTimer.getElapsed());
                }

            } else if (event.equals("Gecko:Exited")) {
                
                doShutdown();
                return;

            } else if ("NativeApp:IsDebuggable".equals(event)) {
                JSONObject ret = new JSONObject();
                ret.put("isDebuggable", getIsDebuggable());
                EventDispatcher.sendResponse(message, ret);
            } else if (event.equals("Accessibility:Event")) {
                GeckoAccessibility.sendAccessibilityEvent(message);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    void onStatePurged() { }

    




    private void showSiteSettingsDialog(final String host, final NativeJSObject[] permissions) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);

        View customTitleView = getLayoutInflater().inflate(R.layout.site_setting_title, null);
        ((TextView) customTitleView.findViewById(R.id.title)).setText(R.string.site_settings_title);
        ((TextView) customTitleView.findViewById(R.id.host)).setText(host);
        builder.setCustomTitle(customTitleView);

        
        
        if (permissions.length == 0) {
            builder.setMessage(R.string.site_settings_no_settings);
        } else {

            final ArrayList<HashMap<String, String>> itemList =
                    new ArrayList<HashMap<String, String>>();
            for (final NativeJSObject permObj : permissions) {
                final HashMap<String, String> map = new HashMap<String, String>();
                map.put("setting", permObj.getString("setting"));
                map.put("value", permObj.getString("value"));
                itemList.add(map);
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
                Toast.makeText(GeckoApp.this, resId, duration).show();
            }
        });
    }

    public void showNormalToast(final String message, final String duration) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Toast toast;
                if (duration.equals("long")) {
                    toast = Toast.makeText(GeckoApp.this, message, Toast.LENGTH_LONG);
                } else {
                    toast = Toast.makeText(GeckoApp.this, message, Toast.LENGTH_SHORT);
                }
                toast.show();
            }
        });
    }

    public ButtonToast getButtonToast() {
        if (mToast != null) {
            return mToast;
        }

        ViewStub toastStub = (ViewStub) findViewById(R.id.toast_stub);
        mToast = new ButtonToast(toastStub.inflate());

        return mToast;
    }

    void showButtonToast(final String message, final String duration,
                         final String buttonText, final String buttonIcon,
                         final String buttonId) {
        BitmapUtils.getDrawable(GeckoApp.this, buttonIcon, new BitmapUtils.BitmapLoader() {
            @Override
            public void onBitmapFound(final Drawable d) {
                final int toastDuration = duration.equals("long") ? ButtonToast.LENGTH_LONG : ButtonToast.LENGTH_SHORT;
                getButtonToast().show(false, message, toastDuration ,buttonText, d, new ButtonToast.ToastListener() {
                    @Override
                    public void onButtonClicked() {
                        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Toast:Click", buttonId));
                    }

                    @Override
                    public void onToastHidden(ButtonToast.ReasonHidden reason) {
                        if (reason == ButtonToast.ReasonHidden.TIMEOUT ||
                            reason == ButtonToast.ReasonHidden.TOUCH_OUTSIDE ||
                            reason == ButtonToast.ReasonHidden.REPLACED) {
                            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Toast:Hidden", buttonId));
                        }
                    }
                });
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
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            Gravity.CENTER);
        mFullScreenPluginContainer.addView(view, layoutParams);


        FrameLayout decor = (FrameLayout)getWindow().getDecorView();
        decor.addView(mFullScreenPluginContainer, layoutParams);

        mFullScreenPluginView = view;
    }

    @Override
    public void addPluginView(final View view, final RectF rect, final boolean isFullScreen) {
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
                mLayerView.showSurface();
            }
        });

        FrameLayout decor = (FrameLayout)getWindow().getDecorView();
        decor.removeView(mFullScreenPluginContainer);

        mFullScreenPluginView = null;

        GeckoScreenOrientation.getInstance().unlock();
        setFullScreen(false);
    }

    @Override
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

    
    private void setImageAs(final String aSrc) {
        boolean isDataURI = aSrc.startsWith("data:");
        Bitmap image = null;
        InputStream is = null;
        ByteArrayOutputStream os = null;
        try {
            if (isDataURI) {
                int dataStart = aSrc.indexOf(",");
                byte[] buf = Base64.decode(aSrc.substring(dataStart+1), Base64.DEFAULT);
                image = BitmapUtils.decodeByteArray(buf);
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
                image = BitmapUtils.decodeByteArray(imgBuffer);
            }
            if (image != null) {
                
                File dcimDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
                if (!dcimDir.mkdirs() && !dcimDir.isDirectory()) {
                    Toast.makeText((Context) this, R.string.set_image_path_fail, Toast.LENGTH_SHORT).show();
                    return;
                }
                String path = Media.insertImage(getContentResolver(),image, null, null);
                if (path == null) {
                    Toast.makeText((Context) this, R.string.set_image_path_fail, Toast.LENGTH_SHORT).show();
                    return;
                }
                final Intent intent = new Intent(Intent.ACTION_ATTACH_DATA);
                intent.addCategory(Intent.CATEGORY_DEFAULT);
                intent.setData(Uri.parse(path));

                
                Intent chooser = Intent.createChooser(intent, getString(R.string.set_image_chooser_title));
                ActivityResultHandler handler = new ActivityResultHandler() {
                    @Override
                    public void onActivityResult (int resultCode, Intent data) {
                        getContentResolver().delete(intent.getData(), null, null);
                    }
                };
                ActivityHandlerHelper.startIntentForActivity(this, chooser, handler);
            } else {
                Toast.makeText((Context) this, R.string.set_image_fail, Toast.LENGTH_SHORT).show();
            }
        } catch(OutOfMemoryError ome) {
            Log.e(LOGTAG, "Out of Memory when converting to byte array", ome);
        } catch(IOException ioe) {
            Log.e(LOGTAG, "I/O Exception while setting wallpaper", ioe);
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch(IOException ioe) {
                    Log.w(LOGTAG, "I/O Exception while closing stream", ioe);
                }
            }
            if (os != null) {
                try {
                    os.close();
                } catch(IOException ioe) {
                    Log.w(LOGTAG, "I/O Exception while closing stream", ioe);
                }
            }
        }
    }

    private int getBitmapSampleSize(BitmapFactory.Options options, int idealWidth, int idealHeight) {
        int width = options.outWidth;
        int height = options.outHeight;
        int inSampleSize = 1;
        if (height > idealHeight || width > idealWidth) {
            if (width > height) {
                inSampleSize = Math.round((float)height / idealHeight);
            } else {
                inSampleSize = Math.round((float)width / idealWidth);
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

    @Override
    public void setFullScreen(final boolean fullscreen) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                ActivityUtils.setFullScreen(GeckoApp.this, fullscreen);
            }
        });
    }

    


    protected static void earlyStartJavaSampler(SafeIntent intent) {
        String env = intent.getStringExtra("env0");
        for (int i = 1; env != null; i++) {
            if (env.startsWith("MOZ_PROFILER_STARTUP=")) {
                if (!env.endsWith("=")) {
                    GeckoJavaSampler.start(10, 1000);
                    Log.d(LOGTAG, "Profiling Java on startup");
                }
                break;
            }
            env = intent.getStringExtra("env" + i);
        }
    }

    





    @Override
    public void onCreate(Bundle savedInstanceState) {
        GeckoAppShell.ensureCrashHandling();

        
        if ("default".equals(AppConstants.MOZ_UPDATE_CHANNEL)) {
            enableStrictMode();
        }

        
        mJavaUiStartupTimer = new Telemetry.UptimeTimer("FENNEC_STARTUP_TIME_JAVAUI");
        mGeckoReadyStartupTimer = new Telemetry.UptimeTimer("FENNEC_STARTUP_TIME_GECKOREADY");

        final SafeIntent intent = new SafeIntent(getIntent());
        final String action = intent.getAction();
        final String args = intent.getStringExtra("args");

        earlyStartJavaSampler(intent);

        
        
        
        GeckoLoader.setLastIntent(intent);

        
        
        
        
        
        if (mProfile == null && args != null) {
            final GeckoProfile p = GeckoProfile.getFromArgs(this, args);
            if (p != null) {
                mProfile = p;
            }
        }

        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                getProfile();
            }
        });

        
        try {
            Class.forName("android.os.AsyncTask");
        } catch (ClassNotFoundException e) {}

        MemoryMonitor.getInstance().init(getApplicationContext());

        
        
        
        
        
        
        GeckoAppShell.setContextGetter(this);
        GeckoAppShell.setGeckoInterface(this);

        Tabs.getInstance().attachToContext(this);
        try {
            Favicons.initializeWithContext(this);
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception starting favicon cache. Corrupt resources?", e);
        }

        
        
        
        
        
        
        
        
        if (BrowserLocaleManager.getInstance().systemLocaleDidChange()) {
            Log.i(LOGTAG, "System locale changed. Restarting.");
            doRestart();
            return;
        }

        if (GeckoThread.isLaunched()) {
            
            
            mIsRestoringActivity = true;
            Telemetry.addToHistogram("FENNEC_RESTORING_ACTIVITY", 1);

        } else {
            final String uri = getURIFromIntent(intent);

            GeckoThread.ensureInit(args, action,
                    TextUtils.isEmpty(uri) ? null : uri,
                     ACTION_DEBUG.equals(action));
        }

        
        

        EventDispatcher.getInstance().registerGeckoThreadListener((GeckoEventListener)this,
            "Gecko:Ready",
            "Gecko:DelayedStartup",
            "Gecko:Exited",
            "Accessibility:Event",
            "NativeApp:IsDebuggable");

        EventDispatcher.getInstance().registerGeckoThreadListener((NativeEventListener)this,
            "Accessibility:Ready",
            "Bookmark:Insert",
            "Contact:Add",
            "DOMFullScreen:Start",
            "DOMFullScreen:Stop",
            "Image:SetAs",
            "Locale:Set",
            "Permissions:Data",
            "PrivateBrowsing:Data",
            "Session:StatePurged",
            "Share:Text",
            "SystemUI:Visibility",
            "Toast:Show",
            "ToggleChrome:Focus",
            "ToggleChrome:Hide",
            "ToggleChrome:Show",
            "Update:Check",
            "Update:Download",
            "Update:Install");

        if (mWebappEventListener == null) {
            mWebappEventListener = new EventListener();
            mWebappEventListener.registerEvents();
        }

        GeckoThread.launch();

        Bundle stateBundle = ContextUtils.getBundleExtra(getIntent(), EXTRA_STATE_BUNDLE);
        if (stateBundle != null) {
            
            
            
            
            final SharedPreferences prefs = getSharedPreferences();
            if (prefs.getBoolean(PREFS_ALLOW_STATE_BUNDLE, false)) {
                prefs.edit().remove(PREFS_ALLOW_STATE_BUNDLE).apply();
                savedInstanceState = stateBundle;
            }
        } else if (savedInstanceState != null) {
            
            setIntent(new Intent(Intent.ACTION_MAIN));
        }

        super.onCreate(savedInstanceState);

        GeckoScreenOrientation.getInstance().update(getResources().getConfiguration().orientation);

        setContentView(getLayout());

        
        mRootLayout = (OuterLayout) findViewById(R.id.root_layout);
        mGeckoLayout = (RelativeLayout) findViewById(R.id.gecko_layout);
        mMainLayout = (RelativeLayout) findViewById(R.id.main_layout);
        mLayerView = (LayerView) findViewById(R.id.layer_view);

        
        mShouldRestore = getSessionRestoreState(savedInstanceState);
        if (mShouldRestore && savedInstanceState != null) {
            boolean wasInBackground =
                savedInstanceState.getBoolean(SAVED_STATE_IN_BACKGROUND, false);

            
            
            if (!wasInBackground && !mIsRestoringActivity) {
                Telemetry.addToHistogram("FENNEC_WAS_KILLED", 1);
            }

            mPrivateBrowsingSession = savedInstanceState.getString(SAVED_STATE_PRIVATE_SESSION);
        }

        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final SharedPreferences prefs = GeckoApp.this.getSharedPreferences();

                
                
                final LocaleManager localeManager = BrowserLocaleManager.getInstance();
                localeManager.initialize(getApplicationContext());

                SessionInformation previousSession = SessionInformation.fromSharedPrefs(prefs);
                if (previousSession.wasKilled()) {
                    Telemetry.addToHistogram("FENNEC_WAS_KILLED", 1);
                }

                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_OOM_EXCEPTION, false);

                
                
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);

                editor.apply();

                
                
                
                final String profilePath = getProfile().getDir().getAbsolutePath();
                final EventDispatcher dispatcher = EventDispatcher.getInstance();

                
                final Locale osLocale = Locale.getDefault();

                
                final String osLocaleString = osLocale.toString();
                String appLocaleString = localeManager.getAndApplyPersistedLocale(GeckoApp.this);
                Log.d(LOGTAG, "OS locale is " + osLocaleString + ", app locale is " + appLocaleString);

                if (appLocaleString == null) {
                    appLocaleString = osLocaleString;
                }

                mHealthRecorder = GeckoApp.this.createHealthRecorder(GeckoApp.this,
                                                                     profilePath,
                                                                     dispatcher,
                                                                     osLocaleString,
                                                                     appLocaleString,
                                                                     previousSession);

                final String uiLocale = appLocaleString;
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        GeckoApp.this.onLocaleReady(uiLocale);
                    }
                });

                
                
                BrowserLocaleManager.storeAndNotifyOSLocale(GeckoSharedPrefs.forProfile(GeckoApp.this), osLocale);
            }
        });

        GeckoAppShell.setNotificationClient(makeNotificationClient());
        IntentHelper.init(this);
    }

    












    @Override
    public void onLocaleReady(final String locale) {
        if (!ThreadUtils.isOnUiThread()) {
            throw new RuntimeException("onLocaleReady must always be called from the UI thread.");
        }

        final Locale loc = Locales.parseLocaleCode(locale);
        if (loc.equals(mLastLocale)) {
            Log.d(LOGTAG, "New locale same as old; onLocaleReady has nothing to do.");
        }

        
        TextView urlBar = (TextView) findViewById(R.id.url_bar_title);
        if (urlBar != null) {
            final String hint = getResources().getString(R.string.url_bar_default_text);
            urlBar.setHint(hint);
        } else {
            Log.d(LOGTAG, "No URL bar in GeckoApp. Not loading localized hint string.");
        }

        mLastLocale = loc;

        
        
        
        
        super.onConfigurationChanged(getResources().getConfiguration());
    }

    protected void initializeChrome() {
        mDoorHangerPopup = new DoorHangerPopup(this);
        mPluginContainer = (AbsoluteLayout) findViewById(R.id.plugin_container);
        mFormAssistPopup = (FormAssistPopup) findViewById(R.id.form_assist_popup);

        if (mCameraView == null) {
            
            if (Versions.preICS) {
                mCameraView = new SurfaceView(this);
                ((SurfaceView)mCameraView).getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
            }
        }

        
        
        
        mLayerView.setFocusable(false);
        mLayerView.setFocusable(true);
        mLayerView.setFocusableInTouchMode(true);
    }

    








    protected void loadStartupTab(String url, int flags) {
        if (url == null) {
            if (!mShouldRestore) {
                
                
                Tabs.getInstance().loadUrl(AboutPages.HOME, flags);
            }
        } else {
            
            Tabs.getInstance().loadUrl(url, flags);
        }
    }

    private void initialize() {
        mInitialized = true;

        final SafeIntent intent = new SafeIntent(getIntent());
        final String action = intent.getAction();

        final String uri = getURIFromIntent(intent);

        final String passedUri;
        if (!TextUtils.isEmpty(uri)) {
            passedUri = uri;
        } else {
            passedUri = null;
        }

        final boolean isExternalURL = passedUri != null &&
                                      !AboutPages.isAboutHome(passedUri);
        final StartupAction startupAction;
        if (isExternalURL) {
            startupAction = StartupAction.URL;
        } else {
            startupAction = StartupAction.NORMAL;
        }

        
        
        checkMigrateProfile();

        Tabs.registerOnTabsChangedListener(this);

        initializeChrome();

        
        if (!mIsRestoringActivity) {
            String restoreMessage = null;
            if (mShouldRestore) {
                try {
                    
                    
                    
                    
                    
                    
                    restoreMessage = restoreSessionTabs(isExternalURL);
                } catch (SessionRestoreException e) {
                    
                    Log.e(LOGTAG, "An error occurred during restore", e);
                    mShouldRestore = false;
                }
            }

            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Session:Restore", restoreMessage));
        }

        
        
        if (isExternalURL) {
            
            
            Tabs.getInstance().notifyListeners(null, Tabs.TabEvents.RESTORED);
            int flags = Tabs.LOADURL_NEW_TAB | Tabs.LOADURL_USER_ENTERED | Tabs.LOADURL_EXTERNAL;
            if (ACTION_HOMESCREEN_SHORTCUT.equals(action)) {
                flags |= Tabs.LOADURL_PINNED;
            }
            loadStartupTab(passedUri, flags);
        } else {
            if (!mIsRestoringActivity) {
                loadStartupTab(null, Tabs.LOADURL_NEW_TAB);
            }

            Tabs.getInstance().notifyListeners(null, Tabs.TabEvents.RESTORED);
        }

        
        
        if (!mShouldRestore) {
            getProfile().moveSessionFile();
        }

        Telemetry.addToHistogram("FENNEC_STARTUP_GECKOAPP_ACTION", startupAction.ordinal());

        
        if (ACTION_LAUNCH_SETTINGS.equals(action)) {
            Intent settingsIntent = new Intent(GeckoApp.this, GeckoPreferences.class);
            
            settingsIntent.putExtras(intent.getUnsafe());
            startActivity(settingsIntent);
        }

        
        mAppStateListeners = new LinkedList<GeckoAppShell.AppStateListener>();

        if (SmsManager.isEnabled()) {
            SmsManager.getInstance().start();
        }

        mContactService = new ContactService(EventDispatcher.getInstance(), this);

        mPromptService = new PromptService(this);

        mTextSelection = new TextSelection((TextSelectionHandle) findViewById(R.id.anchor_handle),
                                           (TextSelectionHandle) findViewById(R.id.caret_handle),
                                           (TextSelectionHandle) findViewById(R.id.focus_handle));

        
        
        mJavaUiStartupTimer.stop();
        final long javaDuration = mJavaUiStartupTimer.getElapsed();

        ThreadUtils.getBackgroundHandler().postDelayed(new Runnable() {
            @Override
            public void run() {
                final HealthRecorder rec = mHealthRecorder;
                if (rec != null) {
                    rec.recordJavaStartupTime(javaDuration);
                }

                UpdateServiceHelper.registerForUpdates(GeckoApp.this);

                
                
                
                GeckoPreferences.broadcastHealthReportUploadPref(GeckoApp.this);
                if (!GeckoThread.checkLaunchState(GeckoThread.LaunchState.Launched)) {
                    return;
                }
            }
        }, 50);

        if (mIsRestoringActivity) {
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null) {
                Tabs.getInstance().notifyListeners(selectedTab, Tabs.TabEvents.SELECTED);
            }

            if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
                geckoConnected();
                GeckoAppShell.sendEventToGecko(
                        GeckoEvent.createBroadcastEvent("Viewport:Flush", null));
            }
        }

        if (ACTION_ALERT_CALLBACK.equals(action)) {
            processAlertCallback(intent);
        } else if (NotificationHelper.HELPER_BROADCAST_ACTION.equals(action)) {
            NotificationHelper.getInstance(getApplicationContext()).handleNotificationIntent(intent);
        }
    }

    private String restoreSessionTabs(final boolean isExternalURL) throws SessionRestoreException {
        try {
            String sessionString = getProfile().readSessionFile(false);
            if (sessionString == null) {
                throw new SessionRestoreException("Could not read from session file");
            }

            
            
            
            if (mShouldRestore) {
                final JSONArray tabs = new JSONArray();
                final JSONObject windowObject = new JSONObject();
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

                    @Override
                    public void onClosedTabsRead(final JSONArray closedTabData) throws JSONException {
                        windowObject.put("closedTabs", closedTabData);
                    }
                };

                if (mPrivateBrowsingSession == null) {
                    parser.parse(sessionString);
                } else {
                    parser.parse(sessionString, mPrivateBrowsingSession);
                }

                if (tabs.length() > 0) {
                    windowObject.put("tabs", tabs);
                    sessionString = new JSONObject().put("windows", new JSONArray().put(windowObject)).toString();
                } else {
                    throw new SessionRestoreException("No tabs could be read from session file");
                }
            }

            JSONObject restoreData = new JSONObject();
            restoreData.put("sessionString", sessionString);
            return restoreData.toString();
        } catch (JSONException e) {
            throw new SessionRestoreException(e);
        }
    }

    @Override
    public synchronized GeckoProfile getProfile() {
        
        if (mProfile == null) {
            mProfile = GeckoProfile.get(this);
        }
        return mProfile;
    }

    





    protected boolean getSessionRestoreState(Bundle savedInstanceState) {
        final SharedPreferences prefs = getSharedPreferences();
        boolean shouldRestore = false;

        final int versionCode = getVersionCode();
        if (prefs.getInt(PREFS_VERSION_CODE, 0) != versionCode) {
            
            
            prefs.edit().putInt(PREFS_VERSION_CODE, versionCode).apply();
            shouldRestore = true;
        } else if (savedInstanceState != null ||
                   getSessionRestorePreference().equals("always") ||
                   getRestartFromIntent()) {
            
            
            shouldRestore = true;
        } else if (prefs.getBoolean(GeckoApp.PREFS_CRASHED, false)) {
            prefs.edit().putBoolean(PREFS_CRASHED, false).apply();
            shouldRestore = true;
        }

        return shouldRestore;
    }

    private String getSessionRestorePreference() {
        return getSharedPreferences().getString(GeckoPreferences.PREFS_RESTORE_SESSION, "quit");
    }

    private boolean getRestartFromIntent() {
        return ContextUtils.getBooleanExtra(getIntent(), "didRestart", false);
    }

    



    private void enableStrictMode() {
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

    @Override
    public void enableCameraView() {
        
        mCameraOrientationEventListener = new OrientationEventListener(this) {
            @Override
            public void onOrientationChanged(int orientation) {
                if (mAppStateListeners != null) {
                    for (GeckoAppShell.AppStateListener listener: mAppStateListeners) {
                        listener.onOrientationChanged();
                    }
                }
            }
        };
        mCameraOrientationEventListener.enable();

        
        if (mCameraView instanceof SurfaceView) {
            if (Versions.feature11Plus) {
                mCameraView.setAlpha(0.0f);
            }
            ViewGroup mCameraLayout = (ViewGroup) findViewById(R.id.camera_layout);
            
            mCameraLayout.addView(mCameraView,
                                  new AbsoluteLayout.LayoutParams(8, 16, 0, 0));
        }
    }

    @Override
    public void disableCameraView() {
        if (mCameraOrientationEventListener != null) {
            mCameraOrientationEventListener.disable();
            mCameraOrientationEventListener = null;
        }
        if (mCameraView != null) {
          ViewGroup mCameraLayout = (ViewGroup) findViewById(R.id.camera_layout);
          mCameraLayout.removeView(mCameraView);
        }
    }

    @Override
    public String getDefaultUAString() {
        return HardwareUtils.isTablet() ? AppConstants.USER_AGENT_FENNEC_TABLET :
                                          AppConstants.USER_AGENT_FENNEC_MOBILE;
    }

    private void processAlertCallback(SafeIntent intent) {
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
    protected void onNewIntent(Intent externalIntent) {
        final SafeIntent intent = new SafeIntent(externalIntent);

        
        
        if (!mInitialized) {
            setIntent(externalIntent);
            return;
        }

        final String action = intent.getAction();

        if (ACTION_LOAD.equals(action)) {
            String uri = intent.getDataString();
            Tabs.getInstance().loadUrl(uri);
        } else if (Intent.ACTION_VIEW.equals(action)) {
            
            
            
            
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    if (AppConstants.NIGHTLY_BUILD && AppConstants.MOZ_ANDROID_TAB_QUEUE
                                && TabQueueHelper.shouldOpenTabQueueUrls(GeckoApp.this)) {

                        EventDispatcher.getInstance().registerGeckoThreadListener(new NativeEventListener() {
                            @Override
                            public void handleMessage(String event, NativeJSObject message, EventCallback callback) {
                                if ("Tabs:TabsOpened".equals(event)) {
                                    EventDispatcher.getInstance().unregisterGeckoThreadListener(this, "Tabs:TabsOpened");
                                    String uri = intent.getDataString();
                                    Tabs.getInstance().loadUrl(uri, Tabs.LOADURL_NEW_TAB |
                                                                            Tabs.LOADURL_USER_ENTERED |
                                                                            Tabs.LOADURL_EXTERNAL);
                                }
                            }
                        }, "Tabs:TabsOpened");

                        TabQueueHelper.openQueuedUrls(GeckoApp.this, mProfile, TabQueueHelper.FILE_NAME, true);
                    } else {
                        String uri = intent.getDataString();
                        Tabs.getInstance().loadUrl(uri, Tabs.LOADURL_NEW_TAB |
                                                                Tabs.LOADURL_USER_ENTERED |
                                                                Tabs.LOADURL_EXTERNAL);
                    }
                }
            });
        } else if (ACTION_HOMESCREEN_SHORTCUT.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBookmarkLoadEvent(uri));
        } else if (Intent.ACTION_SEARCH.equals(action)) {
            String uri = getURIFromIntent(intent);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(uri));
        } else if (ACTION_ALERT_CALLBACK.equals(action)) {
            processAlertCallback(intent);
        } else if (NotificationHelper.HELPER_BROADCAST_ACTION.equals(action)) {
            NotificationHelper.getInstance(getApplicationContext()).handleNotificationIntent(intent);
        } else if (ACTION_LAUNCH_SETTINGS.equals(action)) {
            
            Intent settingsIntent = new Intent(GeckoApp.this, GeckoPreferences.class);
            
            settingsIntent.putExtras(intent.getUnsafe());
            startActivity(settingsIntent);
        }
    }

    



    protected String getURIFromIntent(SafeIntent intent) {
        final String action = intent.getAction();
        if (ACTION_ALERT_CALLBACK.equals(action) || NotificationHelper.HELPER_BROADCAST_ACTION.equals(action)) {
            return null;
        }

        return intent.getDataString();
    }

    protected int getOrientation() {
        return GeckoScreenOrientation.getInstance().getAndroidOrientation();
    }

    @Override
    public void onResume()
    {
        
        
        super.onResume();

        int newOrientation = getResources().getConfiguration().orientation;
        if (GeckoScreenOrientation.getInstance().update(newOrientation)) {
            refreshChrome();
        }

        if (!Versions.feature14Plus) {
            
            
            
            GeckoAccessibility.updateAccessibilitySettings(this);
        }

        if (mAppStateListeners != null) {
            for (GeckoAppShell.AppStateListener listener : mAppStateListeners) {
                listener.onResume();
            }
        }

        
        
        
        final long now = System.currentTimeMillis();
        final long realTime = android.os.SystemClock.elapsedRealtime();

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                
                
                SessionInformation currentSession = new SessionInformation(now, realTime);

                SharedPreferences prefs = GeckoApp.this.getSharedPreferences();
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
                currentSession.recordBegin(editor);
                editor.apply();

                final HealthRecorder rec = mHealthRecorder;
                if (rec != null) {
                    rec.setCurrentSession(currentSession);
                    rec.processDelayed();
                } else {
                    Log.w(LOGTAG, "Can't record session: rec is null.");
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
        final HealthRecorder rec = mHealthRecorder;
        final Context context = this;

        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                SharedPreferences prefs = GeckoApp.this.getSharedPreferences();
                SharedPreferences.Editor editor = prefs.edit();
                editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, true);
                if (rec != null) {
                    rec.recordSessionEnd("P", editor);
                }

                
                if (prefs.getBoolean(GeckoApp.PREFS_CLEANUP_TEMP_FILES, true)) {
                    File tempDir = GeckoLoader.getGREDir(GeckoApp.this);
                    FileUtils.delTree(tempDir, new FileUtils.NameAndAgeFilter(null, ONE_DAY_MS), false);

                    editor.putBoolean(GeckoApp.PREFS_CLEANUP_TEMP_FILES, false);
                }

                editor.apply();

                
                
                
                GeckoPreferences.broadcastHealthReportPrune(context);
            }
        });

        if (mAppStateListeners != null) {
            for (GeckoAppShell.AppStateListener listener : mAppStateListeners) {
                listener.onPause();
            }
        }

        super.onPause();
    }

    @Override
    public void onRestart() {
        
        final StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();
        try {
            SharedPreferences.Editor editor = GeckoApp.this.getSharedPreferences().edit();
            editor.putBoolean(GeckoApp.PREFS_WAS_STOPPED, false);
            editor.apply();
        } finally {
            StrictMode.setThreadPolicy(savedPolicy);
        }

        super.onRestart();
    }

    @Override
    public void onDestroy() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener((GeckoEventListener)this,
            "Gecko:Ready",
            "Gecko:DelayedStartup",
            "Gecko:Exited",
            "Accessibility:Event",
            "NativeApp:IsDebuggable");

        EventDispatcher.getInstance().unregisterGeckoThreadListener((NativeEventListener)this,
            "Accessibility:Ready",
            "Bookmark:Insert",
            "Contact:Add",
            "DOMFullScreen:Start",
            "DOMFullScreen:Stop",
            "Image:SetAs",
            "Locale:Set",
            "Permissions:Data",
            "PrivateBrowsing:Data",
            "Session:StatePurged",
            "Share:Text",
            "SystemUI:Visibility",
            "Toast:Show",
            "ToggleChrome:Focus",
            "ToggleChrome:Hide",
            "ToggleChrome:Show",
            "Update:Check",
            "Update:Download",
            "Update:Install");

        if (mWebappEventListener != null) {
            mWebappEventListener.unregisterEvents();
            mWebappEventListener = null;
        }

        deleteTempFiles();

        if (mLayerView != null)
            mLayerView.destroy();
        if (mDoorHangerPopup != null)
            mDoorHangerPopup.destroy();
        if (mFormAssistPopup != null)
            mFormAssistPopup.destroy();
        if (mContactService != null)
            mContactService.destroy();
        if (mPromptService != null)
            mPromptService.destroy();
        if (mTextSelection != null)
            mTextSelection.destroy();
        NotificationHelper.destroy();
        IntentHelper.destroy();
        GeckoNetworkManager.destroy();

        if (SmsManager.isEnabled()) {
            SmsManager.getInstance().stop();
            if (isFinishing()) {
                SmsManager.getInstance().shutdown();
            }
        }

        final HealthRecorder rec = mHealthRecorder;
        mHealthRecorder = null;
        if (rec != null && rec.isEnabled()) {
            
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    rec.close();
                }
            });
        }

        Favicons.close();

        super.onDestroy();

        Tabs.unregisterOnTabsChangedListener(this);

        if (!isFinishing()) {
            
            return;
        }

        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createAppBackgroundingEvent());
        }

        if (mRestartIntent != null) {
            
            final Intent intent = new Intent();
            intent.setClass(getApplicationContext(), Restarter.class)
                  .putExtra("pid", Process.myPid())
                  .putExtra(Intent.EXTRA_INTENT, mRestartIntent);
            startService(intent);
        } else {
            
            Process.killProcess(Process.myPid());
        }
    }

    
    public static File getTempDirectory() {
        File dir = GeckoApplication.get().getExternalFilesDir("temp");
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
        Log.d(LOGTAG, "onConfigurationChanged: " + newConfig.locale);

        final LocaleManager localeManager = BrowserLocaleManager.getInstance();
        final Locale changed = localeManager.onSystemConfigurationChanged(this, getResources(), newConfig, mLastLocale);
        if (changed != null) {
            onLocaleChanged(Locales.getLanguageTag(changed));
        }

        
        
        
        if (GeckoScreenOrientation.getInstance().update(newConfig.orientation)) {
            if (mFormAssistPopup != null)
                mFormAssistPopup.hide();
            refreshChrome();
        }
        super.onConfigurationChanged(newConfig);
    }

    public String getContentProcessName() {
        return AppConstants.MOZ_CHILD_PROCESS_NAME;
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

    @Override
    public void doRestart() {
        doRestart(null, null);
    }

    public void doRestart(String args) {
        doRestart(args, null);
    }

    public void doRestart(Intent intent) {
        doRestart(null, intent);
    }

    public void doRestart(String args, Intent restartIntent) {
        if (restartIntent == null) {
            restartIntent = new Intent(Intent.ACTION_MAIN);
        }

        if (args != null) {
            restartIntent.putExtra("args", args);
        }

        mRestartIntent = restartIntent;
        Log.d(LOGTAG, "doRestart(\"" + restartIntent + "\")");

        doShutdown();
    }

    private void doShutdown() {
        
        runOnUiThread(new Runnable() {
            @Override public void run() {
                if (!isFinishing() && (Versions.preJBMR1 || !isDestroyed())) {
                    finish();
                }
            }
        });
    }

    public void handleNotification(String action, String alertName, String alertCookie) {
        
        
        
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            GeckoAppShell.handleNotification(action, alertName, alertCookie);
        }
    }

    private void checkMigrateProfile() {
        final File profileDir = getProfile().getDir();

        if (profileDir != null) {
            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    Handler handler = new Handler();
                    handler.postDelayed(new DeferredCleanupTask(), CLEANUP_DEFERRAL_SECONDS * 1000);
                }
            });
        }
    }

    private class DeferredCleanupTask implements Runnable {
        
        
        

        private static final String CLEANUP_VERSION = "cleanup-version";
        private static final int CURRENT_CLEANUP_VERSION = 1;

        @Override
        public void run() {
            long cleanupVersion = getSharedPreferences().getInt(CLEANUP_VERSION, 0);

            if (cleanupVersion < 1) {
                
                
                
                
                File dir = new File("res/fonts");
                if (dir.exists() && dir.isDirectory()) {
                    for (File file : dir.listFiles()) {
                        if (file.isFile() && file.getName().endsWith(".ttf")) {
                            file.delete();
                        }
                    }
                    if (!dir.delete()) {
                        Log.w(LOGTAG, "unable to delete res/fonts directory (not empty?)");
                    }
                }
            }

            

            if (cleanupVersion != CURRENT_CLEANUP_VERSION) {
                SharedPreferences.Editor editor = GeckoApp.this.getSharedPreferences().edit();
                editor.putInt(CLEANUP_VERSION, CURRENT_CLEANUP_VERSION);
                editor.apply();
            }
        }
    }

    @Override
    public PromptService getPromptService() {
        return mPromptService;
    }

    @Override
    public void onBackPressed() {
        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            super.onBackPressed();
            return;
        }

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
        if (!ActivityHandlerHelper.handleActivityResult(requestCode, resultCode, data)) {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    @Override
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

    private static final String CPU = "cpu";
    private static final String SCREEN = "screen";

    
    @Override
    public void notifyWakeLockChanged(String topic, String state) {
        PowerManager.WakeLock wl = mWakeLocks.get(topic);
        if (state.equals("locked-foreground") && wl == null) {
            PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);

            if (CPU.equals(topic)) {
              wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, topic);
            } else if (SCREEN.equals(topic)) {
              wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, topic);
            }

            if (wl != null) {
              wl.acquire();
              mWakeLocks.put(topic, wl);
            }
        } else if (!state.equals("locked-foreground") && wl != null) {
            wl.release();
            mWakeLocks.remove(topic);
        }
    }

    @Override
    public void notifyCheckUpdateResult(String result) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Update:CheckResult", result));
    }

    protected void geckoConnected() {
        mLayerView.geckoConnected();
        mLayerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
    }

    public void setAccessibilityEnabled(boolean enabled) {
    }

    public static class MainLayout extends RelativeLayout {
        private TouchEventInterceptor mTouchEventInterceptor;
        private MotionEventInterceptor mMotionEventInterceptor;
        private LayoutInterceptor mLayoutInterceptor;

        public MainLayout(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
            super.onLayout(changed, left, top, right, bottom);
            if (mLayoutInterceptor != null) {
                mLayoutInterceptor.onLayout();
            }
        }

        public void setLayoutInterceptor(LayoutInterceptor interceptor) {
            mLayoutInterceptor = interceptor;
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
                    mLayerView.hideSurface();
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

    private int getVersionCode() {
        int versionCode = 0;
        try {
            versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
        } catch (NameNotFoundException e) {
            Log.wtf(LOGTAG, getPackageName() + " not found", e);
        }
        return versionCode;
    }

    protected boolean getIsDebuggable() {
        
        
        
        
        return false;

        
        
        
        
        
        
        
        
    }

    
    
    private static final String SESSION_END_LOCALE_CHANGED = "L";

    





    protected void onLocaleChanged(final String locale) {
        final boolean startNewSession = true;
        final boolean shouldRestart = false;

        
        
        
        final HealthRecorder rec = mHealthRecorder;
        if (rec != null) {
            rec.onAppLocaleChanged(locale);
            rec.onEnvironmentChanged(startNewSession, SESSION_END_LOCALE_CHANGED);
        }

        if (!shouldRestart) {
            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    GeckoApp.this.onLocaleReady(locale);
                }
            });
            return;
        }

        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                GeckoApp.this.doRestart();
            }
        });
    }

    



    protected void setLocale(final String locale) {
        if (locale == null) {
            return;
        }

        final String resultant = BrowserLocaleManager.getInstance().setSelectedLocale(this, locale);
        if (resultant == null) {
            return;
        }

        onLocaleChanged(resultant);
    }

    private void setSystemUiVisible(final boolean visible) {
        if (Versions.preICS) {
            return;
        }

        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                if (visible) {
                    mMainLayout.setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
                } else {
                    mMainLayout.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE);
                }
            }
        });
    }

    protected HealthRecorder createHealthRecorder(final Context context,
                                                  final String profilePath,
                                                  final EventDispatcher dispatcher,
                                                  final String osLocale,
                                                  final String appLocale,
                                                  final SessionInformation previousSession) {
        
        return new StubbedHealthRecorder();
    }
}
