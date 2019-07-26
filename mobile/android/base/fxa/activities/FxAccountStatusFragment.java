



package org.mozilla.gecko.fxa.activities;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.preferences.PreferenceFragment;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.Married;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.sync.SyncConfiguration;

import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;







public class FxAccountStatusFragment extends PreferenceFragment implements OnPreferenceClickListener {
  private static final String LOG_TAG = FxAccountStatusFragment.class.getSimpleName();

  
  
  
  
  
  
  private static final long DELAY_IN_MILLISECONDS_BEFORE_REQUESTING_SYNC = 5 * 1000;

  protected Preference emailPreference;

  protected Preference needsPasswordPreference;
  protected Preference needsUpgradePreference;
  protected Preference needsVerificationPreference;

  protected PreferenceCategory syncCategory;

  protected CheckBoxPreference bookmarksPreference;
  protected CheckBoxPreference historyPreference;
  protected CheckBoxPreference tabsPreference;
  protected CheckBoxPreference passwordsPreference;

  protected volatile AndroidFxAccount fxAccount;

  
  protected Handler handler;

  
  
  
  protected Runnable requestSyncRunnable;

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
    addPreferencesFromResource(R.xml.fxaccount_status_prefscreen);

    emailPreference = ensureFindPreference("email");

    needsPasswordPreference = ensureFindPreference("needs_credentials");
    needsUpgradePreference = ensureFindPreference("needs_upgrade");
    needsVerificationPreference = ensureFindPreference("needs_verification");

    syncCategory = (PreferenceCategory) ensureFindPreference("sync_category");

    bookmarksPreference = (CheckBoxPreference) ensureFindPreference("bookmarks");
    historyPreference = (CheckBoxPreference) ensureFindPreference("history");
    tabsPreference = (CheckBoxPreference) ensureFindPreference("tabs");
    passwordsPreference = (CheckBoxPreference) ensureFindPreference("passwords");

    if (!FxAccountConstants.LOG_PERSONAL_INFORMATION) {
      removeDebugButtons();
    } else {
      connectDebugButtons();
    }

    needsPasswordPreference.setOnPreferenceClickListener(this);
    needsVerificationPreference.setOnPreferenceClickListener(this);

    bookmarksPreference.setOnPreferenceClickListener(this);
    historyPreference.setOnPreferenceClickListener(this);
    tabsPreference.setOnPreferenceClickListener(this);
    passwordsPreference.setOnPreferenceClickListener(this);
  }

  



  @Override
  public void onResume() {
    super.onResume();
  }

  @Override
  public boolean onPreferenceClick(Preference preference) {
    if (preference == needsPasswordPreference) {
      Intent intent = new Intent(getActivity(), FxAccountUpdateCredentialsActivity.class);
      
      
      intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
      startActivity(intent);

      return true;
    }

    if (preference == needsVerificationPreference) {
      FxAccountConfirmAccountActivity.resendCode(getActivity().getApplicationContext(), fxAccount);

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

    return false;
  }

  protected void setCheckboxesEnabled(boolean enabled) {
    bookmarksPreference.setEnabled(enabled);
    historyPreference.setEnabled(enabled);
    tabsPreference.setEnabled(enabled);
    passwordsPreference.setEnabled(enabled);
  }

  





  protected void showOnlyOneErrorPreference(Preference errorPreferenceToShow) {
    final Preference[] errorPreferences = new Preference[] {
        this.needsPasswordPreference,
        this.needsUpgradePreference,
        this.needsVerificationPreference };
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

  protected void showConnected() {
    syncCategory.setTitle(R.string.fxaccount_status_sync_enabled);
    showOnlyOneErrorPreference(null);
    setCheckboxesEnabled(true);
  }

  








  public void refresh(AndroidFxAccount fxAccount) {
    if (fxAccount == null) {
      throw new IllegalArgumentException("fxAccount must not be null");
    }
    this.fxAccount = fxAccount;

    handler = new Handler(); 

    
    
    
    requestSyncRunnable = new RequestSyncRunnable();

    refresh();
  }

  protected void refresh() {
    
    
    
    if (fxAccount == null) {
      throw new IllegalArgumentException("fxAccount must not be null");
    }

    emailPreference.setTitle(fxAccount.getEmail());

    
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
    default:
      showConnected();
    }

    updateSelectedEngines();
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
      
      ContentResolver.requestSync(fxAccount.getAndroidAccount(), BrowserContract.AUTHORITY, Bundle.EMPTY);
      
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
        Logger.info(LOG_TAG, "Syncing.");
        final Bundle extras = new Bundle();
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_MANUAL, true);
        ContentResolver.requestSync(fxAccount.getAndroidAccount(), BrowserContract.AUTHORITY, extras);
        
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
      } else {
        return false;
      }
      return true;
    }
  }

  



  protected void connectDebugButtons() {
    
    final OnPreferenceClickListener listener = new DebugPreferenceClickListener();

    
    
    final Preference debugCategory = ensureFindPreference("debug_category");
    debugCategory.setTitle(debugCategory.getKey());

    String[] debugKeys = new String[] {
        "debug_refresh",
        "debug_dump",
        "debug_force_sync",
        "debug_forget_certificate",
        "debug_require_password",
        "debug_require_upgrade" };
    for (String debugKey : debugKeys) {
      final Preference button = ensureFindPreference(debugKey);
      button.setTitle(debugKey); 
      button.setOnPreferenceClickListener(listener);
    }
  }
}
