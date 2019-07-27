



package org.mozilla.gecko.fxa.activities;

import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.background.preferences.PreferenceFragment;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.Married;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.sync.FxAccountSyncStatusHelper;
import org.mozilla.gecko.fxa.tasks.FxAccountCodeResender;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.SharedPreferencesClientsDataDelegate;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.util.HardwareUtils;

import android.accounts.Account;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.widget.Toast;








public class FxAccountStatusFragment
    extends PreferenceFragment
    implements OnPreferenceClickListener, OnPreferenceChangeListener {
  private static final String LOG_TAG = FxAccountStatusFragment.class.getSimpleName();

  


  private static final Date EARLIEST_VALID_SYNCED_DATE;
  static {
    final Calendar c = GregorianCalendar.getInstance();
    c.set(2000, Calendar.JANUARY, 1, 0, 0, 0);
    EARLIEST_VALID_SYNCED_DATE = c.getTime();
  }

  
  
  
  
  
  
  private static final long DELAY_IN_MILLISECONDS_BEFORE_REQUESTING_SYNC = 5 * 1000;
  private static final long LAST_SYNCED_TIME_UPDATE_INTERVAL_IN_MILLISECONDS = 60 * 1000;

  
  
  private static boolean ALWAYS_SHOW_AUTH_SERVER = false;

  
  
  private static boolean ALWAYS_SHOW_SYNC_SERVER = false;

  
  
  
  
  private final int NUMBER_OF_CLICKS_TO_TOGGLE_DEBUG =
      
      (!AppConstants.MOZILLA_OFFICIAL || AppConstants.NIGHTLY_BUILD || AppConstants.DEBUG_BUILD) ? 5 : -1 ;
  private int debugClickCount = 0;

  protected PreferenceCategory accountCategory;
  protected Preference emailPreference;
  protected Preference authServerPreference;

  protected Preference needsPasswordPreference;
  protected Preference needsUpgradePreference;
  protected Preference needsVerificationPreference;
  protected Preference needsMasterSyncAutomaticallyEnabledPreference;
  protected Preference needsFinishMigratingPreference;

  protected PreferenceCategory syncCategory;

  protected CheckBoxPreference bookmarksPreference;
  protected CheckBoxPreference historyPreference;
  protected CheckBoxPreference tabsPreference;
  protected CheckBoxPreference passwordsPreference;
  protected CheckBoxPreference readingListPreference;

  protected EditTextPreference deviceNamePreference;
  protected Preference syncServerPreference;
  protected Preference morePreference;
  protected Preference syncNowPreference;

  protected volatile AndroidFxAccount fxAccount;
  
  
  protected volatile SharedPreferencesClientsDataDelegate clientsDataDelegate;

  
  protected Handler handler;

  
  
  
  protected Runnable requestSyncRunnable;

  
  protected Runnable lastSyncedTimeUpdateRunnable;

  protected final InnerSyncStatusDelegate syncStatusDelegate = new InnerSyncStatusDelegate();

  protected Preference ensureFindPreference(String key) {
    Preference preference = findPreference(key);
    if (preference == null) {
      throw new IllegalStateException("Could not find preference with key: " + key);
    }
    return preference;
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    
    
    
    
    HardwareUtils.init(getActivity());

    addPreferences();
  }

  protected void addPreferences() {
    addPreferencesFromResource(R.xml.fxaccount_status_prefscreen);

    accountCategory = (PreferenceCategory) ensureFindPreference("signed_in_as_category");
    emailPreference = ensureFindPreference("email");
    authServerPreference = ensureFindPreference("auth_server");

    needsPasswordPreference = ensureFindPreference("needs_credentials");
    needsUpgradePreference = ensureFindPreference("needs_upgrade");
    needsVerificationPreference = ensureFindPreference("needs_verification");
    needsMasterSyncAutomaticallyEnabledPreference = ensureFindPreference("needs_master_sync_automatically_enabled");
    needsFinishMigratingPreference = ensureFindPreference("needs_finish_migrating");

    syncCategory = (PreferenceCategory) ensureFindPreference("sync_category");

    bookmarksPreference = (CheckBoxPreference) ensureFindPreference("bookmarks");
    historyPreference = (CheckBoxPreference) ensureFindPreference("history");
    tabsPreference = (CheckBoxPreference) ensureFindPreference("tabs");
    passwordsPreference = (CheckBoxPreference) ensureFindPreference("passwords");

    if (!FxAccountUtils.LOG_PERSONAL_INFORMATION) {
      removeDebugButtons();
    } else {
      connectDebugButtons();
      ALWAYS_SHOW_AUTH_SERVER = true;
      ALWAYS_SHOW_SYNC_SERVER = true;
    }

    emailPreference.setOnPreferenceClickListener(this);

    needsPasswordPreference.setOnPreferenceClickListener(this);
    needsVerificationPreference.setOnPreferenceClickListener(this);
    needsFinishMigratingPreference.setOnPreferenceClickListener(this);

    bookmarksPreference.setOnPreferenceClickListener(this);
    historyPreference.setOnPreferenceClickListener(this);
    tabsPreference.setOnPreferenceClickListener(this);
    passwordsPreference.setOnPreferenceClickListener(this);

    deviceNamePreference = (EditTextPreference) ensureFindPreference("device_name");
    deviceNamePreference.setOnPreferenceChangeListener(this);

    syncServerPreference = ensureFindPreference("sync_server");
    morePreference = ensureFindPreference("more");
    morePreference.setOnPreferenceClickListener(this);

    syncNowPreference = ensureFindPreference("sync_now");
    syncNowPreference.setEnabled(true);
    syncNowPreference.setOnPreferenceClickListener(this);

    if (HardwareUtils.hasMenuButton()) {
      syncCategory.removePreference(morePreference);
    }
  }

  



  @Override
  public void onResume() {
    super.onResume();
  }

  @Override
  public boolean onPreferenceClick(Preference preference) {
    if (preference == emailPreference) {
      debugClickCount += 1;
      if (NUMBER_OF_CLICKS_TO_TOGGLE_DEBUG > 0 && debugClickCount >= NUMBER_OF_CLICKS_TO_TOGGLE_DEBUG) {
        debugClickCount = 0;
        FxAccountUtils.LOG_PERSONAL_INFORMATION = !FxAccountUtils.LOG_PERSONAL_INFORMATION;
        Toast.makeText(getActivity(), "Toggled logging Firefox Account personal information!", Toast.LENGTH_LONG).show();
        hardRefresh(); 
      }
      return true;
    }

    if (preference == needsPasswordPreference) {
      Intent intent = new Intent(getActivity(), FxAccountUpdateCredentialsActivity.class);
      final Bundle extras = getExtrasForAccount();
      if (extras != null) {
        intent.putExtras(extras);
      }
      
      
      intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
      startActivity(intent);

      return true;
    }

    if (preference == needsFinishMigratingPreference) {
      final Intent intent = new Intent(getActivity(), FxAccountFinishMigratingActivity.class);
      final Bundle extras = getExtrasForAccount();
      if (extras != null) {
        intent.putExtras(extras);
      }
      
      
      intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
      startActivity(intent);

      return true;
    }

    if (preference == needsVerificationPreference) {
      FxAccountCodeResender.resendCode(getActivity().getApplicationContext(), fxAccount);

      Intent intent = new Intent(getActivity(), FxAccountConfirmAccountActivity.class);
      
      
      intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
      startActivity(intent);

      return true;
    }

    if (preference == bookmarksPreference ||
        preference == historyPreference ||
        preference == passwordsPreference ||
        preference == tabsPreference) {
      saveEngineSelections();
      return true;
    }

    if (preference == morePreference) {
      getActivity().openOptionsMenu();
      return true;
    }

    if (preference == syncNowPreference) {
      if (fxAccount != null) {
        FirefoxAccounts.requestSync(fxAccount.getAndroidAccount(), FirefoxAccounts.FORCE, null, null);
      }
      return true;
    }

    return false;
  }

  protected Bundle getExtrasForAccount() {
    final Bundle extras = new Bundle();
    final ExtendedJSONObject o = new ExtendedJSONObject();
    o.put(FxAccountAbstractSetupActivity.JSON_KEY_AUTH, fxAccount.getAccountServerURI());
    final ExtendedJSONObject services = new ExtendedJSONObject();
    services.put(FxAccountAbstractSetupActivity.JSON_KEY_SYNC, fxAccount.getTokenServerURI());
    o.put(FxAccountAbstractSetupActivity.JSON_KEY_SERVICES, services);
    extras.putString(FxAccountAbstractSetupActivity.EXTRA_EXTRAS, o.toJSONString());
    return extras;
  }

  protected void setCheckboxesEnabled(boolean enabled) {
    bookmarksPreference.setEnabled(enabled);
    historyPreference.setEnabled(enabled);
    tabsPreference.setEnabled(enabled);
    passwordsPreference.setEnabled(enabled);
    
    deviceNamePreference.setEnabled(enabled);
    syncNowPreference.setEnabled(enabled);
  }

  





  protected void showOnlyOneErrorPreference(Preference errorPreferenceToShow) {
    final Preference[] errorPreferences = new Preference[] {
        this.needsPasswordPreference,
        this.needsUpgradePreference,
        this.needsVerificationPreference,
        this.needsMasterSyncAutomaticallyEnabledPreference,
        this.needsFinishMigratingPreference,
    };
    for (Preference errorPreference : errorPreferences) {
      final boolean currentlyShown = null != findPreference(errorPreference.getKey());
      final boolean shouldBeShown = errorPreference == errorPreferenceToShow;
      if (currentlyShown == shouldBeShown) {
        continue;
      }
      if (shouldBeShown) {
        syncCategory.addPreference(errorPreference);
      } else {
        syncCategory.removePreference(errorPreference);
      }
    }
  }

  protected void showNeedsPassword() {
    syncCategory.setTitle(R.string.fxaccount_status_sync);
    showOnlyOneErrorPreference(needsPasswordPreference);
    setCheckboxesEnabled(false);
  }

  protected void showNeedsUpgrade() {
    syncCategory.setTitle(R.string.fxaccount_status_sync);
    showOnlyOneErrorPreference(needsUpgradePreference);
    setCheckboxesEnabled(false);
  }

  protected void showNeedsVerification() {
    syncCategory.setTitle(R.string.fxaccount_status_sync);
    showOnlyOneErrorPreference(needsVerificationPreference);
    setCheckboxesEnabled(false);
  }

  protected void showNeedsMasterSyncAutomaticallyEnabled() {
    syncCategory.setTitle(R.string.fxaccount_status_sync);
    needsMasterSyncAutomaticallyEnabledPreference.setTitle(AppConstants.Versions.preLollipop ?
                                                   R.string.fxaccount_status_needs_master_sync_automatically_enabled :
                                                   R.string.fxaccount_status_needs_master_sync_automatically_enabled_v21);
    showOnlyOneErrorPreference(needsMasterSyncAutomaticallyEnabledPreference);
    setCheckboxesEnabled(false);
  }

  protected void showNeedsFinishMigrating() {
    syncCategory.setTitle(R.string.fxaccount_status_sync);
    showOnlyOneErrorPreference(needsFinishMigratingPreference);
    setCheckboxesEnabled(false);
  }

  protected void showConnected() {
    syncCategory.setTitle(R.string.fxaccount_status_sync_enabled);
    showOnlyOneErrorPreference(null);
    setCheckboxesEnabled(true);
  }

  protected class InnerSyncStatusDelegate implements FirefoxAccounts.SyncStatusListener {
    protected final Runnable refreshRunnable = new Runnable() {
      @Override
      public void run() {
        refresh();
      }
    };

    @Override
    public Context getContext() {
      return FxAccountStatusFragment.this.getActivity();
    }

    @Override
    public Account getAccount() {
      return fxAccount.getAndroidAccount();
    }

    @Override
    public void onSyncStarted() {
      if (fxAccount == null) {
        return;
      }
      Logger.info(LOG_TAG, "Got sync started message; refreshing.");
      getActivity().runOnUiThread(refreshRunnable);
    }

    @Override
    public void onSyncFinished() {
      if (fxAccount == null) {
        return;
      }
      Logger.info(LOG_TAG, "Got sync finished message; refreshing.");
      getActivity().runOnUiThread(refreshRunnable);
    }
  }

  








  public void refresh(AndroidFxAccount fxAccount) {
    if (fxAccount == null) {
      throw new IllegalArgumentException("fxAccount must not be null");
    }
    this.fxAccount = fxAccount;
    try {
      this.clientsDataDelegate = new SharedPreferencesClientsDataDelegate(fxAccount.getSyncPrefs(), getActivity().getApplicationContext());
    } catch (Exception e) {
      Logger.error(LOG_TAG, "Got exception fetching Sync prefs associated to Firefox Account; aborting.", e);
      
      
      throw new IllegalStateException(e);
    }

    handler = new Handler(); 

    
    
    
    requestSyncRunnable = new RequestSyncRunnable();
    lastSyncedTimeUpdateRunnable = new LastSyncTimeUpdateRunnable();

    
    
    
    
    
    
    FxAccountSyncStatusHelper.getInstance().startObserving(syncStatusDelegate);

    refresh();
  }

  @Override
  public void onPause() {
    super.onPause();
    FxAccountSyncStatusHelper.getInstance().stopObserving(syncStatusDelegate);

    
    if (lastSyncedTimeUpdateRunnable != null) {
      handler.removeCallbacks(lastSyncedTimeUpdateRunnable);
    }
  }

  protected void hardRefresh() {
    
    
    
    final PreferenceScreen statusScreen = (PreferenceScreen) ensureFindPreference("status_screen");
    statusScreen.removeAll();
    addPreferences();

    refresh();
  }

  protected void refresh() {
    
    
    
    if (fxAccount == null) {
      throw new IllegalArgumentException("fxAccount must not be null");
    }

    emailPreference.setTitle(fxAccount.getEmail());
    updateAuthServerPreference();
    updateSyncServerPreference();

    try {
      
      
      
      
      

      
      State state = fxAccount.getState();
      switch (state.getNeededAction()) {
      case NeedsUpgrade:
        showNeedsUpgrade();
        break;
      case NeedsPassword:
        showNeedsPassword();
        break;
      case NeedsVerification:
        showNeedsVerification();
        break;
      case NeedsFinishMigrating:
        showNeedsFinishMigrating();
        break;
      case None:
        showConnected();
        break;
      }

      
      
      
      
      final boolean masterSyncAutomatically = ContentResolver.getMasterSyncAutomatically();
      if (!masterSyncAutomatically) {
        showNeedsMasterSyncAutomaticallyEnabled();
        return;
      }
    } finally {
      
      updateSelectedEngines();
    }

    final String clientName = clientsDataDelegate.getClientName();
    deviceNamePreference.setSummary(clientName);
    deviceNamePreference.setText(clientName);

    updateSyncNowPreference();
  }

  
  private String getLastSyncedString(final long startTime) {
    if (new Date(startTime).before(EARLIEST_VALID_SYNCED_DATE)) {
      return getActivity().getString(R.string.fxaccount_status_never_synced);
    }
    final CharSequence relativeTimeSpanString = DateUtils.getRelativeTimeSpanString(startTime);
    return getActivity().getResources().getString(R.string.fxaccount_status_last_synced, relativeTimeSpanString);
  }

  protected void updateSyncNowPreference() {
    final boolean currentlySyncing = fxAccount.isCurrentlySyncing();
    syncNowPreference.setEnabled(!currentlySyncing);
    if (currentlySyncing) {
      syncNowPreference.setTitle(R.string.fxaccount_status_syncing);
    } else {
      syncNowPreference.setTitle(R.string.fxaccount_status_sync_now);
    }
    scheduleAndUpdateLastSyncedTime();
  }

  private void scheduleAndUpdateLastSyncedTime() {
    final String lastSynced = getLastSyncedString(fxAccount.getLastSyncedTimestamp());
    syncNowPreference.setSummary(lastSynced);
    handler.postDelayed(lastSyncedTimeUpdateRunnable, LAST_SYNCED_TIME_UPDATE_INTERVAL_IN_MILLISECONDS);
  }

  protected void updateAuthServerPreference() {
    final String authServer = fxAccount.getAccountServerURI();
    final boolean shouldBeShown = ALWAYS_SHOW_AUTH_SERVER || !FxAccountConstants.DEFAULT_AUTH_SERVER_ENDPOINT.equals(authServer);
    final boolean currentlyShown = null != findPreference(authServerPreference.getKey());
    if (currentlyShown != shouldBeShown) {
      if (shouldBeShown) {
        accountCategory.addPreference(authServerPreference);
      } else {
        accountCategory.removePreference(authServerPreference);
      }
    }
    
    
    authServerPreference.setSummary(authServer);
  }

  protected void updateSyncServerPreference() {
    final String syncServer = fxAccount.getTokenServerURI();
    final boolean shouldBeShown = ALWAYS_SHOW_SYNC_SERVER || !FxAccountConstants.DEFAULT_TOKEN_SERVER_ENDPOINT.equals(syncServer);
    final boolean currentlyShown = null != findPreference(syncServerPreference.getKey());
    if (currentlyShown != shouldBeShown) {
      if (shouldBeShown) {
        syncCategory.addPreference(syncServerPreference);
      } else {
        syncCategory.removePreference(syncServerPreference);
      }
    }
    
    
    syncServerPreference.setSummary(syncServer);
  }

  






  protected void updateSelectedEngines() {
    try {
      SharedPreferences syncPrefs = fxAccount.getSyncPrefs();
      Map<String, Boolean> engines = SyncConfiguration.getUserSelectedEngines(syncPrefs);
      if (engines != null) {
        bookmarksPreference.setChecked(engines.containsKey("bookmarks") && engines.get("bookmarks"));
        historyPreference.setChecked(engines.containsKey("history") && engines.get("history"));
        passwordsPreference.setChecked(engines.containsKey("passwords") && engines.get("passwords"));
        tabsPreference.setChecked(engines.containsKey("tabs") && engines.get("tabs"));
        return;
      }

      
      Set<String> enabledNames = SyncConfiguration.getEnabledEngineNames(syncPrefs);
      if (enabledNames != null) {
        bookmarksPreference.setChecked(enabledNames.contains("bookmarks"));
        historyPreference.setChecked(enabledNames.contains("history"));
        passwordsPreference.setChecked(enabledNames.contains("passwords"));
        tabsPreference.setChecked(enabledNames.contains("tabs"));
        return;
      }

      
      
      
      
      bookmarksPreference.setChecked(true);
      historyPreference.setChecked(true);
      passwordsPreference.setChecked(true);
      tabsPreference.setChecked(true);
      setCheckboxesEnabled(false);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception getting engines to select; ignoring.", e);
      return;
    }
  }

  



  protected void saveEngineSelections() {
    final Map<String, Boolean> engineSelections = new HashMap<String, Boolean>();
    engineSelections.put("bookmarks", bookmarksPreference.isChecked());
    engineSelections.put("history", historyPreference.isChecked());
    engineSelections.put("passwords", passwordsPreference.isChecked());
    engineSelections.put("tabs", tabsPreference.isChecked());

    
    
    
    new Thread(new PersistEngineSelectionsRunnable(engineSelections)).start();
  }

  protected void requestDelayedSync() {
    Logger.info(LOG_TAG, "Posting a delayed request for a sync sometime soon.");
    handler.removeCallbacks(requestSyncRunnable);
    handler.postDelayed(requestSyncRunnable, DELAY_IN_MILLISECONDS_BEFORE_REQUESTING_SYNC);
  }

  


  protected void removeDebugButtons() {
    final PreferenceScreen statusScreen = (PreferenceScreen) ensureFindPreference("status_screen");
    final PreferenceCategory debugCategory = (PreferenceCategory) ensureFindPreference("debug_category");
    statusScreen.removePreference(debugCategory);
  }

  






  protected class PersistEngineSelectionsRunnable implements Runnable {
    private final Map<String, Boolean> engineSelections;

    protected PersistEngineSelectionsRunnable(Map<String, Boolean> engineSelections) {
      this.engineSelections = engineSelections;
    }

    @Override
    public void run() {
      try {
        
        AndroidFxAccount fxAccount = FxAccountStatusFragment.this.fxAccount;
        if (fxAccount == null) {
          return;
        }
        Logger.info(LOG_TAG, "Persisting engine selections: " + engineSelections.toString());
        SyncConfiguration.storeSelectedEnginesToPrefs(fxAccount.getSyncPrefs(), engineSelections);
        requestDelayedSync();
      } catch (Exception e) {
        Logger.warn(LOG_TAG, "Got exception persisting selected engines; ignoring.", e);
        return;
      }
    }
  }

  





  protected class RequestSyncRunnable implements Runnable {
    @Override
    public void run() {
      
      AndroidFxAccount fxAccount = FxAccountStatusFragment.this.fxAccount;
      if (fxAccount == null) {
        return;
      }
      Logger.info(LOG_TAG, "Requesting a sync sometime soon.");
      fxAccount.requestSync();
    }
  }

  


  protected class LastSyncTimeUpdateRunnable implements Runnable  {
    @Override
    public void run() {
      scheduleAndUpdateLastSyncedTime();
    }
  }

  


  protected class DebugPreferenceClickListener implements OnPreferenceClickListener {
    @Override
    public boolean onPreferenceClick(Preference preference) {
      final String key = preference.getKey();
      if ("debug_refresh".equals(key)) {
        Logger.info(LOG_TAG, "Refreshing.");
        refresh();
      } else if ("debug_dump".equals(key)) {
        fxAccount.dump();
      } else if ("debug_force_sync".equals(key)) {
        Logger.info(LOG_TAG, "Force syncing.");
        fxAccount.requestSync(FirefoxAccounts.FORCE);
        
      } else if ("debug_forget_certificate".equals(key)) {
        State state = fxAccount.getState();
        try {
          Married married = (Married) state;
          Logger.info(LOG_TAG, "Moving to Cohabiting state: Forgetting certificate.");
          fxAccount.setState(married.makeCohabitingState());
          refresh();
        } catch (ClassCastException e) {
          Logger.info(LOG_TAG, "Not in Married state; can't forget certificate.");
          
        }
      } else if ("debug_invalidate_certificate".equals(key)) {
        State state = fxAccount.getState();
        try {
          Married married = (Married) state;
          Logger.info(LOG_TAG, "Invalidating certificate.");
          fxAccount.setState(married.makeCohabitingState().withCertificate("INVALID CERTIFICATE"));
          refresh();
        } catch (ClassCastException e) {
          Logger.info(LOG_TAG, "Not in Married state; can't invalidate certificate.");
          
        }
      } else if ("debug_require_password".equals(key)) {
        Logger.info(LOG_TAG, "Moving to Separated state: Forgetting password.");
        State state = fxAccount.getState();
        fxAccount.setState(state.makeSeparatedState());
        refresh();
      } else if ("debug_require_upgrade".equals(key)) {
        Logger.info(LOG_TAG, "Moving to Doghouse state: Requiring upgrade.");
        State state = fxAccount.getState();
        fxAccount.setState(state.makeDoghouseState());
        refresh();
      } else if ("debug_migrated_from_sync11".equals(key)) {
        Logger.info(LOG_TAG, "Moving to MigratedFromSync11 state: Requiring password.");
        State state = fxAccount.getState();
        fxAccount.setState(state.makeMigratedFromSync11State(null));
        refresh();
      } else if ("debug_make_account_stage".equals(key)) {
        Logger.info(LOG_TAG, "Moving Account endpoints, in place, to stage.  Deleting Sync and RL prefs and requiring password.");
        fxAccount.unsafeTransitionToStageEndpoints();
        refresh();
      } else if ("debug_make_account_default".equals(key)) {
        Logger.info(LOG_TAG, "Moving Account endpoints, in place, to default (production).  Deleting Sync and RL prefs and requiring password.");
        fxAccount.unsafeTransitionToDefaultEndpoints();
        refresh();
      } else {
        return false;
      }
      return true;
    }
  }

  



  protected void connectDebugButtons() {
    
    final OnPreferenceClickListener listener = new DebugPreferenceClickListener();

    
    
    final PreferenceCategory debugCategory = (PreferenceCategory) ensureFindPreference("debug_category");
    debugCategory.setTitle(debugCategory.getKey());

    for (int i = 0; i < debugCategory.getPreferenceCount(); i++) {
      final Preference button = debugCategory.getPreference(i);
      button.setTitle(button.getKey()); 
      button.setOnPreferenceClickListener(listener);
    }
  }

  @Override
  public boolean onPreferenceChange(Preference preference, Object newValue) {
    if (preference == deviceNamePreference) {
      String newClientName = (String) newValue;
      if (TextUtils.isEmpty(newClientName)) {
        newClientName = clientsDataDelegate.getDefaultClientName();
      }
      final long now = System.currentTimeMillis();
      clientsDataDelegate.setClientName(newClientName, now);
      requestDelayedSync(); 
      hardRefresh(); 
      return true;
    }

    
    return true;
  }
}
