




package org.mozilla.gecko.preferences;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import android.os.Build;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.DataReportingNotification;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoActivityStatus;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoApplication;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.GuestSession;
import org.mozilla.gecko.LocaleManager;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.R;
import org.mozilla.gecko.RestrictedProfiles;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.TelemetryContract.Method;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;
import org.mozilla.gecko.db.BrowserContract.SuggestedSites;
import org.mozilla.gecko.updater.UpdateService;
import org.mozilla.gecko.updater.UpdateServiceHelper;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.FloatingHintEditText;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.NotificationManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.res.Configuration;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;

public class GeckoPreferences
extends PreferenceActivity
implements
GeckoActivityStatus,
GeckoEventListener,
OnPreferenceChangeListener,
OnSharedPreferenceChangeListener
{
    private static final String LOGTAG = "GeckoPreferences";

    
    
    
    private static final boolean NO_TRANSITIONS = HardwareUtils.IS_KINDLE_DEVICE;

    public static final String NON_PREF_PREFIX = "android.not_a_preference.";
    public static final String INTENT_EXTRA_RESOURCES = "resource";
    public static String PREFS_HEALTHREPORT_UPLOAD_ENABLED = NON_PREF_PREFIX + "healthreport.uploadEnabled";

    private static boolean sIsCharEncodingEnabled;
    private boolean mInitialized;
    private int mPrefsRequestId;
    private PanelsPreferenceCategory mPanelsPreferenceCategory;

    
    private static final String PREFS_SEARCH_RESTORE_DEFAULTS = NON_PREF_PREFIX + "search.restore_defaults";
    private static final String PREFS_DATA_REPORTING_PREFERENCES = NON_PREF_PREFIX + "datareporting.preferences";
    private static final String PREFS_TELEMETRY_ENABLED = "toolkit.telemetry.enabled";
    private static final String PREFS_CRASHREPORTER_ENABLED = "datareporting.crashreporter.submitEnabled";
    private static final String PREFS_MENU_CHAR_ENCODING = "browser.menu.showCharacterEncoding";
    private static final String PREFS_MP_ENABLED = "privacy.masterpassword.enabled";
    private static final String PREFS_LOGIN_MANAGE = NON_PREF_PREFIX + "signon.manage";
    private static final String PREFS_UPDATER_AUTODOWNLOAD = "app.update.autodownload";
    private static final String PREFS_UPDATER_URL = "app.update.url.android";
    private static final String PREFS_GEO_REPORTING = NON_PREF_PREFIX + "app.geo.reportdata";
    private static final String PREFS_GEO_LEARN_MORE = NON_PREF_PREFIX + "geo.learn_more";
    private static final String PREFS_HEALTHREPORT_LINK = NON_PREF_PREFIX + "healthreport.link";
    private static final String PREFS_DEVTOOLS_REMOTE_ENABLED = "devtools.debugger.remote-enabled";
    private static final String PREFS_DISPLAY_REFLOW_ON_ZOOM = "browser.zoom.reflowOnZoom";
    private static final String PREFS_DISPLAY_TITLEBAR_MODE = "browser.chrome.titlebarMode";
    private static final String PREFS_SYNC = NON_PREF_PREFIX + "sync";
    private static final String PREFS_TRACKING_PROTECTION = "privacy.trackingprotection.enabled";
    private static final String PREFS_TRACKING_PROTECTION_LEARN_MORE = NON_PREF_PREFIX + "trackingprotection.learn_more";

    private static final String ACTION_STUMBLER_UPLOAD_PREF = AppConstants.ANDROID_PACKAGE_NAME + ".STUMBLER_PREF";

    
    private static final String PREFS_BROWSER_LOCALE = "locale";

    public static final String PREFS_RESTORE_SESSION = NON_PREF_PREFIX + "restoreSession3";
    public static final String PREFS_SUGGESTED_SITES = NON_PREF_PREFIX + "home_suggested_sites";
    public static final String PREFS_TAB_QUEUE = NON_PREF_PREFIX + "tab_queue";
    public static final String PREFS_CUSTOMIZE_SCREEN = NON_PREF_PREFIX + "customize_screen";
    public static final String PREFS_TAB_QUEUE_LAST_SITE = NON_PREF_PREFIX + "last_site";
    public static final String PREFS_TAB_QUEUE_LAST_TIME = NON_PREF_PREFIX + "last_time";

    
    private static final int REQUEST_CODE_PREF_SCREEN = 5;
    private static final int RESULT_CODE_EXIT_SETTINGS = 6;

    
    
    
    public static final int RESULT_CODE_LOCALE_DID_CHANGE = 7;

    


    private Locale lastLocale = Locale.getDefault();
    private boolean localeSwitchingIsEnabled;

    private void startActivityForResultChoosingTransition(final Intent intent, final int requestCode) {
        startActivityForResult(intent, requestCode);
        if (NO_TRANSITIONS) {
            overridePendingTransition(0, 0);
        }
    }

    private void finishChoosingTransition() {
        finish();
        if (NO_TRANSITIONS) {
            overridePendingTransition(0, 0);
        }
    }
    private void updateActionBarTitle(int title) {
        if (Versions.feature14Plus) {
            final String newTitle = getString(title);
            if (newTitle != null) {
                Log.v(LOGTAG, "Setting action bar title to " + newTitle);

                final ActionBar actionBar = getActionBar();
                if (actionBar != null) {
                    actionBar.setTitle(newTitle);
                }
            }
        }
    }

    private void updateTitle(String newTitle) {
        if (newTitle != null) {
            Log.v(LOGTAG, "Setting activity title to " + newTitle);
            setTitle(newTitle);
        }
    }

    private void updateTitle(int title) {
        updateTitle(getString(title));
    }

    



    private void updateBreadcrumbTitle(int title) {
        final String newTitle = getString(title);
        showBreadCrumbs(newTitle, newTitle);
    }

    private void updateTitleForPrefsResource(int res) {
        
        
        
        if (Versions.feature11Plus && isMultiPane()) {
            int title = getIntent().getIntExtra(EXTRA_SHOW_FRAGMENT_TITLE, -1);
            if (res == R.xml.preferences) {
                
                
                
                updateActionBarTitle(R.string.settings_title);
            }

            updateTitle(title);
            updateBreadcrumbTitle(title);
            return;
        }

        
        
        int title = -1;
        if (res == R.xml.preferences) {
            title = R.string.settings_title;
        } else if (res == R.xml.preferences_locale) {
            title = R.string.pref_category_language;
        } else if (res == R.xml.preferences_vendor) {
            title = R.string.pref_category_vendor;
        } else if (res == R.xml.preferences_customize) {
            title = R.string.pref_category_customize;
        }
        if (title != -1) {
            updateTitle(title);
        }
    }

    private void onLocaleChanged(Locale newLocale) {
        Log.d(LOGTAG, "onLocaleChanged: " + newLocale);

        BrowserLocaleManager.getInstance().updateConfiguration(getApplicationContext(), newLocale);
        this.lastLocale = newLocale;

        if (Versions.feature11Plus && isMultiPane()) {
            
            invalidateHeaders();

            
            
            final FragmentManager fragmentManager = getFragmentManager();
            int id = getResources().getIdentifier("android:id/prefs", null, null);
            final Fragment current = fragmentManager.findFragmentById(id);
            if (current != null) {
                fragmentManager.beginTransaction()
                               .disallowAddToBackStack()
                               .detach(current)
                               .attach(current)
                               .commitAllowingStateLoss();
            } else {
                Log.e(LOGTAG, "No prefs fragment to reattach!");
            }

            
            
            
            if (onIsMultiPane()) {
                updateActionBarTitle(R.string.settings_title);
            }

            updateTitle(R.string.pref_header_language);
            updateBreadcrumbTitle(R.string.pref_header_language);

            
            
            
            setResult(RESULT_CODE_LOCALE_DID_CHANGE);
            return;
        }

        refreshSuggestedSites();

        
        
        
        
        final Intent intent = (Intent) getIntent().clone();
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        startActivityForResultChoosingTransition(intent, REQUEST_CODE_PREF_SCREEN);

        setResult(RESULT_CODE_LOCALE_DID_CHANGE);
        finishChoosingTransition();
    }

    private void checkLocale() {
        final Locale currentLocale = Locale.getDefault();
        Log.v(LOGTAG, "Checking locale: " + currentLocale + " vs " + lastLocale);
        if (currentLocale.equals(lastLocale)) {
            return;
        }

        onLocaleChanged(currentLocale);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        
        RestrictedProfiles.initWithProfile(GeckoProfile.get(this));

        
        checkLocale();

        
        
        localeSwitchingIsEnabled = BrowserLocaleManager.getInstance().isEnabled();

        
        
        
        

        if (Versions.feature11Plus) {
            if (!getIntent().hasExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT)) {
                
                setupTopLevelFragmentIntent();

                
                
                updateTitle(getString(R.string.pref_header_customize));
            }

            if (onIsMultiPane()) {
                
                
                updateActionBarTitle(R.string.settings_title);

                if (Build.VERSION.SDK_INT < 13) {
                    
                    
                    
                    localeSwitchingIsEnabled = false;
                }
            }
        }

        super.onCreate(savedInstanceState);
        initActionBar();

        
        Bundle intentExtras = getIntent().getExtras();

        
        
        
        if (Versions.preHC) {
            
            getPreferenceManager().setSharedPreferencesName(GeckoSharedPrefs.APP_PREFS_NAME);

            int res = 0;
            if (intentExtras != null && intentExtras.containsKey(INTENT_EXTRA_RESOURCES)) {
                
                final String resourceName = intentExtras.getString(INTENT_EXTRA_RESOURCES);
                Telemetry.sendUIEvent(TelemetryContract.Event.ACTION, Method.SETTINGS, resourceName);

                if (resourceName != null) {
                    res = getResources().getIdentifier(resourceName, "xml", getPackageName());
                    if (res == 0) {
                        Log.e(LOGTAG, "No resource found named " + resourceName);
                    }
                }
            }
            if (res == 0) {
                
                Log.e(LOGTAG, "Displaying default settings.");
                res = R.xml.preferences;
                Telemetry.startUISession(TelemetryContract.Session.SETTINGS);
            }

            
            updateTitleForPrefsResource(res);
            addPreferencesFromResource(res);
        }

        EventDispatcher.getInstance().registerGeckoThreadListener(this,
            "Sanitize:Finished");

        
        
        final ListView mListView = getListView();
        mListView.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                
                final ListAdapter listAdapter = ((ListView) parent).getAdapter();
                final Object listItem = listAdapter.getItem(position);

                
                if (listItem instanceof CustomListPreference && listItem instanceof View.OnLongClickListener) {
                    final View.OnLongClickListener longClickListener = (View.OnLongClickListener) listItem;
                    return longClickListener.onLongClick(view);
                }
                return false;
            }
        });

        
        
        

        
        if (intentExtras != null && intentExtras.containsKey(DataReportingNotification.ALERT_NAME_DATAREPORTING_NOTIFICATION)) {
            NotificationManager notificationManager = (NotificationManager) this.getSystemService(Context.NOTIFICATION_SERVICE);
            notificationManager.cancel(DataReportingNotification.ALERT_NAME_DATAREPORTING_NOTIFICATION.hashCode());
        }
    }

    








    private void initActionBar() {
        if (Versions.feature14Plus) {
            final ActionBar actionBar = getActionBar();
            if (actionBar != null) {
                actionBar.setHomeButtonEnabled(true);
                actionBar.setDisplayHomeAsUpEnabled(true);
                actionBar.setLogo(R.drawable.logo);
                actionBar.setDisplayUseLogoEnabled(true);
            }
        }
    }

    



    private void setupTopLevelFragmentIntent() {
        Intent intent = getIntent();
        
        Bundle intentExtras = intent.getExtras();
        Bundle fragmentArgs = new Bundle();
        
        if (intentExtras != null && intentExtras.containsKey(INTENT_EXTRA_RESOURCES)) {
            String resourceName = intentExtras.getString(INTENT_EXTRA_RESOURCES);
            fragmentArgs.putString(INTENT_EXTRA_RESOURCES, resourceName);
        } else {
            
            if (!onIsMultiPane()) {
                fragmentArgs.putString(INTENT_EXTRA_RESOURCES, "preferences");
            } else {
                fragmentArgs.putString(INTENT_EXTRA_RESOURCES, "preferences_customize_tablet");
            }
        }

        
        intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT, GeckoPreferenceFragment.class.getName());
        intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT_ARGUMENTS, fragmentArgs);
    }

    @Override
    public boolean isValidFragment(String fragmentName) {
        return GeckoPreferenceFragment.class.getName().equals(fragmentName);
    }

    @Override
    public void onBuildHeaders(List<Header> target) {
        if (onIsMultiPane()) {
            loadHeadersFromResource(R.xml.preference_headers, target);

            
            
            
            if (!localeSwitchingIsEnabled) {
                for (Header header : target) {
                    if (header.id == R.id.pref_header_language) {
                        target.remove(header);
                        break;
                    }
                }
            }
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (!hasFocus || mInitialized)
            return;

        mInitialized = true;
        if (Versions.preHC) {
            PreferenceScreen screen = getPreferenceScreen();
            mPrefsRequestId = setupPreferences(screen);
        }
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();

        if (NO_TRANSITIONS) {
            overridePendingTransition(0, 0);
        }

        Telemetry.sendUIEvent(TelemetryContract.Event.CANCEL, Method.BACK, "settings");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
            "Sanitize:Finished");
        if (mPrefsRequestId > 0) {
            PrefsHelper.removeObserver(mPrefsRequestId);
        }

        
        
        
        if (Versions.preHC && getIntent().getExtras() == null) {
            Telemetry.stopUISession(TelemetryContract.Session.SETTINGS);
        }
    }

    @Override
    public void onPause() {
        
        if (Versions.feature11Plus) {
            if (isMultiPane()) {
                SharedPreferences prefs = GeckoSharedPrefs.forApp(this);
                prefs.unregisterOnSharedPreferenceChangeListener(this);
            }
        }

        super.onPause();

        if (getApplication() instanceof GeckoApplication) {
            ((GeckoApplication) getApplication()).onActivityPause(this);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        if (getApplication() instanceof GeckoApplication) {
            ((GeckoApplication) getApplication()).onActivityResume(this);
        }

        if (Versions.feature11Plus) {
            
            
            
            if (isMultiPane()) {
                SharedPreferences prefs = GeckoSharedPrefs.forApp(this);
                prefs.registerOnSharedPreferenceChangeListener(this);
            }
        }
    }

    @Override
    public void startActivity(Intent intent) {
        
        
        
        
        
        
        startActivityForResultChoosingTransition(intent, REQUEST_CODE_PREF_SCREEN);
    }

    @Override
    public void startWithFragment(String fragmentName, Bundle args,
            Fragment resultTo, int resultRequestCode, int titleRes, int shortTitleRes) {
        Log.v(LOGTAG, "Starting with fragment: " + fragmentName + ", title " + titleRes);

        
        Intent intent = onBuildStartFragmentIntent(fragmentName, args, titleRes, shortTitleRes);
        if (resultTo == null) {
            startActivityForResultChoosingTransition(intent, REQUEST_CODE_PREF_SCREEN);
        } else {
            resultTo.startActivityForResult(intent, resultRequestCode);
            if (NO_TRANSITIONS) {
                overridePendingTransition(0, 0);
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        
        
        checkLocale();

        switch (requestCode) {
          case REQUEST_CODE_PREF_SCREEN:
              switch (resultCode) {
              case RESULT_CODE_EXIT_SETTINGS:
                  updateActionBarTitle(R.string.settings_title);

                  
                  setResult(RESULT_CODE_EXIT_SETTINGS);
                  finishChoosingTransition();
                  break;
              }
              break;
        }
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Sanitize:Finished")) {
                boolean success = message.getBoolean("success");
                final int stringRes = success ? R.string.private_data_success : R.string.private_data_fail;
                final Context context = this;
                ThreadUtils.postToUiThread(new Runnable () {
                    @Override
                    public void run() {
                        Toast.makeText(context, stringRes, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    






    public int setupPreferences(PreferenceGroup prefs) {
        ArrayList<String> list = new ArrayList<String>();
        setupPreferences(prefs, list);
        return getGeckoPreferences(prefs, list);
    }

    









    private void setupPreferences(PreferenceGroup preferences, ArrayList<String> prefs) {
        for (int i = 0; i < preferences.getPreferenceCount(); i++) {
            Preference pref = preferences.getPreference(i);

            
            
            
            if (!localeSwitchingIsEnabled &&
                "preferences_locale".equals(pref.getExtras().getString("resource"))) {
                preferences.removePreference(pref);
                i--;
                continue;
            }

            String key = pref.getKey();
            if (pref instanceof PreferenceGroup) {
                
                if (PREFS_DATA_REPORTING_PREFERENCES.equals(key)) {
                    if (!AppConstants.MOZ_DATA_REPORTING) {
                        preferences.removePreference(pref);
                        i--;
                        continue;
                    }
                } else if (pref instanceof PanelsPreferenceCategory) {
                    mPanelsPreferenceCategory = (PanelsPreferenceCategory) pref;
                }
                if((AppConstants.MOZ_ANDROID_TAB_QUEUE && AppConstants.NIGHTLY_BUILD) && (PREFS_CUSTOMIZE_SCREEN.equals(key))) {
                    
                    pref.setSummary(getString(R.string.pref_category_customize_alt_summary));
                }
                setupPreferences((PreferenceGroup) pref, prefs);
            } else {
                pref.setOnPreferenceChangeListener(this);
                if (!AppConstants.MOZ_UPDATER &&
                    PREFS_UPDATER_AUTODOWNLOAD.equals(key)) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (!AppConstants.NIGHTLY_BUILD &&
                           PREFS_LOGIN_MANAGE.equals(key)) {
                    preferences.removePreference(pref);
                    i--;
                } else if (AppConstants.RELEASE_BUILD &&
                           PREFS_DISPLAY_REFLOW_ON_ZOOM.equals(key)) {
                    
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (!AppConstants.NIGHTLY_BUILD &&
                           (PREFS_TRACKING_PROTECTION.equals(key) ||
                            PREFS_TRACKING_PROTECTION_LEARN_MORE.equals(key))) {
                    
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (!AppConstants.MOZ_TELEMETRY_REPORTING &&
                           PREFS_TELEMETRY_ENABLED.equals(key)) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (!AppConstants.MOZ_SERVICES_HEALTHREPORT &&
                           (PREFS_HEALTHREPORT_UPLOAD_ENABLED.equals(key) ||
                            PREFS_HEALTHREPORT_LINK.equals(key))) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (!AppConstants.MOZ_CRASHREPORTER &&
                           PREFS_CRASHREPORTER_ENABLED.equals(key)) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (!AppConstants.MOZ_STUMBLER_BUILD_TIME_ENABLED &&
                           (PREFS_GEO_REPORTING.equals(key) ||
                            PREFS_GEO_LEARN_MORE.equals(key))) {
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (PREFS_DEVTOOLS_REMOTE_ENABLED.equals(key)) {
                    if (!RestrictedProfiles.isAllowed(this, RestrictedProfiles.Restriction.DISALLOW_REMOTE_DEBUGGING)) {
                        preferences.removePreference(pref);
                        i--;
                        continue;
                    }
                } else if (PREFS_RESTORE_SESSION.equals(key) ||
                           PREFS_BROWSER_LOCALE.equals(key)) {
                    
                    
                    
                    
                    ListPreference listPref = (ListPreference) pref;
                    CharSequence selectedEntry = listPref.getEntry();
                    listPref.setSummary(selectedEntry);
                    continue;
                } else if (PREFS_SYNC.equals(key) &&
                           !RestrictedProfiles.isAllowed(this, RestrictedProfiles.Restriction.DISALLOW_MODIFY_ACCOUNTS)) {
                    
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (PREFS_SEARCH_RESTORE_DEFAULTS.equals(key)) {
                    pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
                        @Override
                        public boolean onPreferenceClick(Preference preference) {
                            GeckoPreferences.this.restoreDefaultSearchEngines();
                            Telemetry.sendUIEvent(TelemetryContract.Event.SEARCH_RESTORE_DEFAULTS, Method.LIST_ITEM);
                            return true;
                        }
                    });
                } else if (PREFS_DISPLAY_TITLEBAR_MODE.equals(key) &&
                           HardwareUtils.isTablet()) {
                    
                    preferences.removePreference(pref);
                    i--;
                    continue;
                } else if (handlers.containsKey(key)) {
                    PrefHandler handler = handlers.get(key);
                    if (!handler.setupPref(this, pref)) {
                        preferences.removePreference(pref);
                        i--;
                        continue;
                    }
                } else if (!(AppConstants.MOZ_ANDROID_TAB_QUEUE && AppConstants.NIGHTLY_BUILD) && PREFS_TAB_QUEUE.equals(key)) {
                    
                    preferences.removePreference(pref);
                    i--;
                    continue;
                }

                
                
                
                
                
                
                if (isGeckoPref(key)) {
                    prefs.add(key);
                }
            }
        }
    }

    private boolean isGeckoPref(String key) {
        if (TextUtils.isEmpty(key)) {
            return false;
        }

        if (key.startsWith(NON_PREF_PREFIX)) {
            return false;
        }

        if (key.equals(PREFS_BROWSER_LOCALE)) {
            return false;
        }

        return true;
    }

    


    protected void restoreDefaultSearchEngines() {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:RestoreDefaults", null));

        
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:GetVisible", null));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int itemId = item.getItemId();
        switch (itemId) {
            case android.R.id.home:
                finishChoosingTransition();
                return true;
        }

        
        if (itemId == R.id.restore_defaults) {
            restoreDefaultSearchEngines();
            Telemetry.sendUIEvent(TelemetryContract.Event.SEARCH_RESTORE_DEFAULTS, Method.MENU);
            return true;
       }

        return super.onOptionsItemSelected(item);
    }

    final private int DIALOG_CREATE_MASTER_PASSWORD = 0;
    final private int DIALOG_REMOVE_MASTER_PASSWORD = 1;

    public static void setCharEncodingState(boolean enabled) {
        sIsCharEncodingEnabled = enabled;
    }

    public static boolean getCharEncodingState() {
        return sIsCharEncodingEnabled;
    }

    public static void broadcastAction(final Context context, final Intent intent) {
        fillIntentWithProfileInfo(context, intent);
        context.sendBroadcast(intent, GlobalConstants.PER_ANDROID_PACKAGE_PERMISSION);
    }

    







    public static void broadcastPrefAction(final Context context,
                                           final String action,
                                           final String pref,
                                           final boolean value) {
        final Intent intent = new Intent(action)
                .putExtra("pref", pref)
                .putExtra("branch", GeckoSharedPrefs.APP_PREFS_NAME)
                .putExtra("enabled", value);
        broadcastAction(context, intent);
    }

    private static void fillIntentWithProfileInfo(final Context context, final Intent intent) {
        
        
        
        
        GeckoProfile profile = GeckoProfile.get(context);
        if (profile != null) {
            intent.putExtra("profileName", profile.getName())
                  .putExtra("profilePath", profile.getDir().getAbsolutePath());
        }
    }

    



    public static void broadcastHealthReportUploadPref(final Context context, final boolean value) {
        broadcastPrefAction(context,
                            HealthReportConstants.ACTION_HEALTHREPORT_UPLOAD_PREF,
                            PREFS_HEALTHREPORT_UPLOAD_ENABLED,
                            value);
    }

    



    public static void broadcastHealthReportUploadPref(final Context context) {
        final boolean value = getBooleanPref(context, PREFS_HEALTHREPORT_UPLOAD_ENABLED, true);
        broadcastHealthReportUploadPref(context, value);
    }

    public static void broadcastHealthReportPrune(final Context context) {
        final Intent intent = new Intent(HealthReportConstants.ACTION_HEALTHREPORT_PRUNE);
        broadcastAction(context, intent);
    }

    



    public static void broadcastStumblerPref(final Context context, final boolean value) {
       Intent intent = new Intent(ACTION_STUMBLER_UPLOAD_PREF)
                .putExtra("pref", PREFS_GEO_REPORTING)
                .putExtra("branch", GeckoSharedPrefs.APP_PREFS_NAME)
                .putExtra("enabled", value)
                .putExtra("moz_mozilla_api_key", AppConstants.MOZ_MOZILLA_API_KEY);
       if (GeckoAppShell.getGeckoInterface() != null) {
           intent.putExtra("user_agent", GeckoAppShell.getGeckoInterface().getDefaultUAString());
       }
       broadcastAction(context, intent);
    }

    



    public static void broadcastStumblerPref(final Context context) {
        final boolean value = getBooleanPref(context, PREFS_GEO_REPORTING, false);
        broadcastStumblerPref(context, value);
    }

    











    public static boolean getBooleanPref(final Context context, final String name, boolean def) {
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);
        return prefs.getBoolean(name, def);
    }

    


















    private boolean onLocaleSelected(final String currentLocale, final String newValue) {
        final Context context = getApplicationContext();

        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final LocaleManager localeManager = BrowserLocaleManager.getInstance();

                if (TextUtils.isEmpty(newValue)) {
                    BrowserLocaleManager.getInstance().resetToSystemLocale(context);
                    Telemetry.sendUIEvent(TelemetryContract.Event.LOCALE_BROWSER_RESET);
                } else {
                    if (null == localeManager.setSelectedLocale(context, newValue)) {
                        localeManager.updateConfiguration(context, Locale.getDefault());
                    }
                    Telemetry.sendUIEvent(TelemetryContract.Event.LOCALE_BROWSER_UNSELECTED, Method.NONE,
                                          currentLocale == null ? "unknown" : currentLocale);
                    Telemetry.sendUIEvent(TelemetryContract.Event.LOCALE_BROWSER_SELECTED, Method.NONE, newValue);
                }

                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        onLocaleChanged(Locale.getDefault());
                    }
                });
            }
        });

        return true;
    }

    private void refreshSuggestedSites() {
        final ContentResolver cr = getApplicationContext().getContentResolver();

        
        
        cr.notifyChange(SuggestedSites.CONTENT_URI, null);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        Log.d(LOGTAG, "onConfigurationChanged: " + newConfig.locale);

        if (lastLocale.equals(newConfig.locale)) {
            Log.d(LOGTAG, "Old locale same as new locale. Short-circuiting.");
            return;
        }

        final LocaleManager localeManager = BrowserLocaleManager.getInstance();
        final Locale changed = localeManager.onSystemConfigurationChanged(this, getResources(), newConfig, lastLocale);
        if (changed != null) {
            onLocaleChanged(changed);
        }
    }

    










    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (PREFS_BROWSER_LOCALE.equals(key)) {
            onLocaleSelected(Locales.getLanguageTag(lastLocale),
                             sharedPreferences.getString(key, null));
        } else if (PREFS_SUGGESTED_SITES.equals(key)) {
            refreshSuggestedSites();
        }
    }

    public interface PrefHandler {
        
        
        public boolean setupPref(Context context, Preference pref);
        public void onChange(Context context, Preference pref, Object newValue);
    }

    @SuppressWarnings("serial")
    private final Map<String, PrefHandler> handlers = new HashMap<String, PrefHandler>() {{
        put(ClearOnShutdownPref.PREF, new ClearOnShutdownPref());
        put(AndroidImportPreference.PREF_KEY, new AndroidImportPreference.Handler());
    }};

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String prefName = preference.getKey();
        Log.i(LOGTAG, "Changed " + prefName + " = " + newValue);

        Telemetry.sendUIEvent(TelemetryContract.Event.EDIT, Method.SETTINGS, prefName);

        if (PREFS_MP_ENABLED.equals(prefName)) {
            showDialog((Boolean) newValue ? DIALOG_CREATE_MASTER_PASSWORD : DIALOG_REMOVE_MASTER_PASSWORD);

            
            
            return false;
        }

        if (PREFS_BROWSER_LOCALE.equals(prefName)) {
            
            
            return onLocaleSelected(Locales.getLanguageTag(lastLocale), (String) newValue);
        }

        if (PREFS_MENU_CHAR_ENCODING.equals(prefName)) {
            setCharEncodingState(((String) newValue).equals("true"));
        } else if (PREFS_UPDATER_AUTODOWNLOAD.equals(prefName)) {
            UpdateServiceHelper.setAutoDownloadPolicy(this, UpdateService.AutoDownloadPolicy.get((String) newValue));
        } else if (PREFS_UPDATER_URL.equals(prefName)) {
            UpdateServiceHelper.setUpdateUrl(this, (String) newValue);
        } else if (PREFS_HEALTHREPORT_UPLOAD_ENABLED.equals(prefName)) {
            
            
            
            
            broadcastHealthReportUploadPref(this, (Boolean) newValue);
        } else if (PREFS_GEO_REPORTING.equals(prefName)) {
            broadcastStumblerPref(this, (Boolean) newValue);
            
            newValue = (Boolean) newValue ? 1 : 0;
        } else if (handlers.containsKey(prefName)) {
            PrefHandler handler = handlers.get(prefName);
            handler.onChange(this, preference, newValue);
        }

        
        if (isGeckoPref(prefName)) {
            PrefsHelper.setPref(prefName, newValue, true );
        }

        if (preference instanceof ListPreference) {
            
            int newIndex = ((ListPreference) preference).findIndexOfValue((String) newValue);
            CharSequence newEntry = ((ListPreference) preference).getEntries()[newIndex];
            ((ListPreference) preference).setSummary(newEntry);
        } else if (preference instanceof LinkPreference) {
            setResult(RESULT_CODE_EXIT_SETTINGS);
            finishChoosingTransition();
        } else if (preference instanceof FontSizePreference) {
            final FontSizePreference fontSizePref = (FontSizePreference) preference;
            fontSizePref.setSummary(fontSizePref.getSavedFontSizeName());
        }

        return true;
    }

    private EditText getTextBox(int aHintText) {
        EditText input = new FloatingHintEditText(this);
        int inputtype = InputType.TYPE_CLASS_TEXT;
        inputtype |= InputType.TYPE_TEXT_VARIATION_PASSWORD | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
        input.setInputType(inputtype);

        input.setHint(aHintText);
        return input;
    }

    private class PasswordTextWatcher implements TextWatcher {
        EditText input1;
        EditText input2;
        AlertDialog dialog;

        PasswordTextWatcher(EditText aInput1, EditText aInput2, AlertDialog aDialog) {
            input1 = aInput1;
            input2 = aInput2;
            dialog = aDialog;
        }

        @Override
        public void afterTextChanged(Editable s) {
            if (dialog == null)
                return;

            String text1 = input1.getText().toString();
            String text2 = input2.getText().toString();
            boolean disabled = TextUtils.isEmpty(text1) || TextUtils.isEmpty(text2) || !text1.equals(text2);
            dialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(!disabled);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) { }
    }

    private class EmptyTextWatcher implements TextWatcher {
        EditText input;
        AlertDialog dialog;

        EmptyTextWatcher(EditText aInput, AlertDialog aDialog) {
            input = aInput;
            dialog = aDialog;
        }

        @Override
        public void afterTextChanged(Editable s) {
            if (dialog == null)
                return;

            String text = input.getText().toString();
            boolean disabled = TextUtils.isEmpty(text);
            dialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(!disabled);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) { }
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LinearLayout linearLayout = new LinearLayout(this);
        linearLayout.setOrientation(LinearLayout.VERTICAL);
        AlertDialog dialog;
        switch(id) {
            case DIALOG_CREATE_MASTER_PASSWORD:
                final EditText input1 = getTextBox(R.string.masterpassword_password);
                final EditText input2 = getTextBox(R.string.masterpassword_confirm);
                linearLayout.addView(input1);
                linearLayout.addView(input2);

                builder.setTitle(R.string.masterpassword_create_title)
                       .setView((View) linearLayout)
                       .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                JSONObject jsonPref = new JSONObject();
                                try {
                                    jsonPref.put("name", PREFS_MP_ENABLED);
                                    jsonPref.put("flush", true);
                                    jsonPref.put("type", "string");
                                    jsonPref.put("value", input1.getText().toString());

                                    GeckoEvent event = GeckoEvent.createBroadcastEvent("Preferences:Set", jsonPref.toString());
                                    GeckoAppShell.sendEventToGecko(event);
                                } catch(Exception ex) {
                                    Log.e(LOGTAG, "Error setting master password", ex);
                                }
                                return;
                            }
                        })
                        .setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                return;
                            }
                        });
                        dialog = builder.create();
                        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
                            @Override
                            public void onShow(DialogInterface dialog) {
                                input1.setText("");
                                input2.setText("");
                                input1.requestFocus();
                            }
                        });

                        PasswordTextWatcher watcher = new PasswordTextWatcher(input1, input2, dialog);
                        input1.addTextChangedListener((TextWatcher) watcher);
                        input2.addTextChangedListener((TextWatcher) watcher);

                break;
            case DIALOG_REMOVE_MASTER_PASSWORD:
                final EditText input = getTextBox(R.string.masterpassword_password);
                linearLayout.addView(input);

                builder.setTitle(R.string.masterpassword_remove_title)
                       .setView((View) linearLayout)
                       .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                PrefsHelper.setPref(PREFS_MP_ENABLED, input.getText().toString());
                            }
                        })
                        .setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                return;
                            }
                        });
                        dialog = builder.create();
                        dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                            @Override
                            public void onDismiss(DialogInterface dialog) {
                                input.setText("");
                            }
                        });
                        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
                            @Override
                            public void onShow(DialogInterface dialog) {
                                input.setText("");
                            }
                        });
                        input.addTextChangedListener(new EmptyTextWatcher(input, dialog));
                break;
            default:
                return null;
        }

        return dialog;
    }

    
    private int getGeckoPreferences(final PreferenceGroup screen, ArrayList<String> prefs) {
        return PrefsHelper.getPrefs(prefs, new PrefsHelper.PrefHandlerBase() {
            private Preference getField(String prefName) {
                return screen.findPreference(prefName);
            }

            
            class CheckBoxPrefSetter {
                public void setBooleanPref(Preference preference, boolean value) {
                    if ((preference instanceof CheckBoxPreference) &&
                       ((CheckBoxPreference) preference).isChecked() != value) {
                        ((CheckBoxPreference) preference).setChecked(value);
                    }
                }
            }

            class TwoStatePrefSetter extends CheckBoxPrefSetter {
                @Override
                public void setBooleanPref(Preference preference, boolean value) {
                    if ((preference instanceof TwoStatePreference) &&
                       ((TwoStatePreference) preference).isChecked() != value) {
                        ((TwoStatePreference) preference).setChecked(value);
                    }
                }
            }

            @Override
            public void prefValue(String prefName, final boolean value) {
                final Preference pref = getField(prefName);
                final CheckBoxPrefSetter prefSetter;
                if (Versions.preICS) {
                    prefSetter = new CheckBoxPrefSetter();
                } else {
                    prefSetter = new TwoStatePrefSetter();
                }
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        prefSetter.setBooleanPref(pref, value);
                    }
                });
            }

            @Override
            public void prefValue(String prefName, final String value) {
                final Preference pref = getField(prefName);
                if (pref instanceof EditTextPreference) {
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            ((EditTextPreference) pref).setText(value);
                        }
                    });
                } else if (pref instanceof ListPreference) {
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            ((ListPreference) pref).setValue(value);
                            
                            CharSequence selectedEntry = ((ListPreference) pref).getEntry();
                            ((ListPreference) pref).setSummary(selectedEntry);
                        }
                    });
                } else if (pref instanceof FontSizePreference) {
                    final FontSizePreference fontSizePref = (FontSizePreference) pref;
                    fontSizePref.setSavedFontSize(value);
                    final String fontSizeName = fontSizePref.getSavedFontSizeName();
                    ThreadUtils.postToUiThread(new Runnable() {
                        @Override
                        public void run() {
                            fontSizePref.setSummary(fontSizeName); 
                        }
                    });
                }
            }

            @Override
            public void prefValue(String prefName, final int value) {
                final Preference pref = getField(prefName);
                Log.w(LOGTAG, "Unhandled int value for pref [" + pref + "]");
            }

            @Override
            public boolean isObserver() {
                return true;
            }

            @Override
            public void finish() {
                
                ThreadUtils.postToUiThread(new Runnable() {
                    @Override
                    public void run() {
                        screen.setEnabled(true);
                    }
                });
            }
        });
    }

    @Override
    public boolean isGeckoActivityOpened() {
        return false;
    }

    







    public static void setResourceToOpen(final Intent intent, final String resource) {
        if (intent == null) {
            throw new IllegalArgumentException("intent must not be null");
        }
        if (resource == null) {
            return;
        }

        if (Versions.preHC) {
            intent.putExtra("resource", resource);
        } else {
            intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT, GeckoPreferenceFragment.class.getName());

            Bundle fragmentArgs = new Bundle();
            fragmentArgs.putString("resource", resource);
            intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT_ARGUMENTS, fragmentArgs);
        }
    }
}
