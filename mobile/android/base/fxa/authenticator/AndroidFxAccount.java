



package org.mozilla.gecko.fxa.authenticator;

import java.io.UnsupportedEncodingException;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;

import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.StateLabel;
import org.mozilla.gecko.fxa.login.StateFactory;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.setup.Constants;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;









public class AndroidFxAccount {
  protected static final String LOG_TAG = AndroidFxAccount.class.getSimpleName();

  public static final int CURRENT_PREFS_VERSION = 1;

  
  public static final int CURRENT_ACCOUNT_VERSION = 3;
  public static final String ACCOUNT_KEY_ACCOUNT_VERSION = "version";
  public static final String ACCOUNT_KEY_PROFILE = "profile";
  public static final String ACCOUNT_KEY_IDP_SERVER = "idpServerURI";

  
  public static final String ACCOUNT_KEY_AUDIENCE = "audience";                 
  public static final String ACCOUNT_KEY_TOKEN_SERVER = "tokenServerURI";       
  public static final String ACCOUNT_KEY_DESCRIPTOR = "descriptor";

  public static final int CURRENT_BUNDLE_VERSION = 2;
  public static final String BUNDLE_KEY_BUNDLE_VERSION = "version";
  public static final String BUNDLE_KEY_STATE_LABEL = "stateLabel";
  public static final String BUNDLE_KEY_STATE = "state";

  protected static final List<String> ANDROID_AUTHORITIES = Collections.unmodifiableList(Arrays.asList(
      new String[] { BrowserContract.AUTHORITY }));

  protected final Context context;
  protected final AccountManager accountManager;
  protected final Account account;

  















  public AndroidFxAccount(Context applicationContext, Account account) {
    this.context = applicationContext;
    this.account = account;
    this.accountManager = AccountManager.get(this.context);
  }

  







  public void pickle(final String filename) {
    AccountPickler.pickle(this, filename);
  }

  public Account getAndroidAccount() {
    return this.account;
  }

  protected int getAccountVersion() {
    String v = accountManager.getUserData(account, ACCOUNT_KEY_ACCOUNT_VERSION);
    if (v == null) {
      return 0;         
    }

    try {
      return Integer.parseInt(v, 10);
    } catch (NumberFormatException ex) {
      return 0;
    }
  }

  



  protected void persistBundle(ExtendedJSONObject bundle) {
    accountManager.setUserData(account, ACCOUNT_KEY_DESCRIPTOR, bundle.toJSONString());
  }

  



  protected ExtendedJSONObject unbundle() {
    final int version = getAccountVersion();
    if (version < CURRENT_ACCOUNT_VERSION) {
      
      
      return null;
    }

    if (version > CURRENT_ACCOUNT_VERSION) {
      
      return null;
    }

    String bundle = accountManager.getUserData(account, ACCOUNT_KEY_DESCRIPTOR);
    if (bundle == null) {
      return null;
    }
    return unbundleAccountV2(bundle);
  }

  protected String getBundleData(String key) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return null;
    }
    return o.getString(key);
  }

  protected boolean getBundleDataBoolean(String key, boolean def) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return def;
    }
    Boolean b = o.getBoolean(key);
    if (b == null) {
      return def;
    }
    return b.booleanValue();
  }

  protected byte[] getBundleDataBytes(String key) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return null;
    }
    return o.getByteArrayHex(key);
  }

  protected void updateBundleDataBytes(String key, byte[] value) {
    updateBundleValue(key, value == null ? null : Utils.byte2Hex(value));
  }

  protected void updateBundleValue(String key, boolean value) {
    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      return;
    }
    descriptor.put(key, value);
    persistBundle(descriptor);
  }

  protected void updateBundleValue(String key, String value) {
    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      return;
    }
    descriptor.put(key, value);
    persistBundle(descriptor);
  }

  private ExtendedJSONObject unbundleAccountV1(String bundle) {
    ExtendedJSONObject o;
    try {
      o = new ExtendedJSONObject(bundle);
    } catch (Exception e) {
      return null;
    }
    if (CURRENT_BUNDLE_VERSION == o.getIntegerSafely(BUNDLE_KEY_BUNDLE_VERSION)) {
      return o;
    }
    return null;
  }

  private ExtendedJSONObject unbundleAccountV2(String bundle) {
    return unbundleAccountV1(bundle);
  }

  



  public String getProfile() {
    return accountManager.getUserData(account, ACCOUNT_KEY_PROFILE);
  }

  public String getAccountServerURI() {
    return accountManager.getUserData(account, ACCOUNT_KEY_IDP_SERVER);
  }

  public String getAudience() {
    return accountManager.getUserData(account, ACCOUNT_KEY_AUDIENCE);
  }

  public String getTokenServerURI() {
    return accountManager.getUserData(account, ACCOUNT_KEY_TOKEN_SERVER);
  }

  


  public String getSyncPrefsPath() throws GeneralSecurityException, UnsupportedEncodingException {
    String profile = getProfile();
    String username = account.name;

    if (profile == null) {
      throw new IllegalStateException("Missing profile. Cannot fetch prefs.");
    }

    if (username == null) {
      throw new IllegalStateException("Missing username. Cannot fetch prefs.");
    }

    final String tokenServerURI = getTokenServerURI();
    if (tokenServerURI == null) {
      throw new IllegalStateException("No token server URI. Cannot fetch prefs.");
    }

    final String fxaServerURI = getAccountServerURI();
    if (fxaServerURI == null) {
      throw new IllegalStateException("No account server URI. Cannot fetch prefs.");
    }

    final String product = GlobalConstants.BROWSER_INTENT_PACKAGE + ".fxa";
    final long version = CURRENT_PREFS_VERSION;

    
    final String serverURLThing = fxaServerURI + "!" + tokenServerURI;
    return Utils.getPrefsPath(product, username, serverURLThing, profile, version);
  }

  public SharedPreferences getSyncPrefs() throws UnsupportedEncodingException, GeneralSecurityException {
    return context.getSharedPreferences(getSyncPrefsPath(), Utils.SHARED_PREFERENCES_MODE);
  }

  








  public ExtendedJSONObject toJSONObject() {
    ExtendedJSONObject o = unbundle();
    o.put("email", account.name);
    try {
      o.put("emailUTF8", Utils.byte2Hex(account.name.getBytes("UTF-8")));
    } catch (UnsupportedEncodingException e) {
      
    }
    return o;
  }

  public static AndroidFxAccount addAndroidAccount(
      Context context,
      String email,
      String profile,
      String idpServerURI,
      String tokenServerURI,
      State state)
          throws UnsupportedEncodingException, GeneralSecurityException, URISyntaxException {
    return addAndroidAccount(context, email, profile, idpServerURI, tokenServerURI, state,
        CURRENT_ACCOUNT_VERSION, true, false, null);
  }

  public static AndroidFxAccount addAndroidAccount(
      Context context,
      String email,
      String profile,
      String idpServerURI,
      String tokenServerURI,
      State state,
      final int accountVersion,
      final boolean syncEnabled,
      final boolean fromPickle,
      ExtendedJSONObject bundle)
          throws UnsupportedEncodingException, GeneralSecurityException, URISyntaxException {
    if (email == null) {
      throw new IllegalArgumentException("email must not be null");
    }
    if (profile == null) {
      throw new IllegalArgumentException("profile must not be null");
    }
    if (idpServerURI == null) {
      throw new IllegalArgumentException("idpServerURI must not be null");
    }
    if (tokenServerURI == null) {
      throw new IllegalArgumentException("tokenServerURI must not be null");
    }
    if (state == null) {
      throw new IllegalArgumentException("state must not be null");
    }

    
    if (accountVersion != CURRENT_ACCOUNT_VERSION) {
      throw new IllegalStateException("Could not create account of version " + accountVersion +
          ". Current version is " + CURRENT_ACCOUNT_VERSION + ".");
    }

    
    
    Bundle userdata = new Bundle();
    userdata.putString(ACCOUNT_KEY_ACCOUNT_VERSION, "" + CURRENT_ACCOUNT_VERSION);
    userdata.putString(ACCOUNT_KEY_IDP_SERVER, idpServerURI);
    userdata.putString(ACCOUNT_KEY_TOKEN_SERVER, tokenServerURI);
    userdata.putString(ACCOUNT_KEY_AUDIENCE, FxAccountUtils.getAudienceForURL(tokenServerURI));
    userdata.putString(ACCOUNT_KEY_PROFILE, profile);

    if (bundle == null) {
      bundle = new ExtendedJSONObject();
      
      bundle.put(BUNDLE_KEY_BUNDLE_VERSION, CURRENT_BUNDLE_VERSION);
    }
    bundle.put(BUNDLE_KEY_STATE_LABEL, state.getStateLabel().name());
    bundle.put(BUNDLE_KEY_STATE, state.toJSONObject().toJSONString());

    userdata.putString(ACCOUNT_KEY_DESCRIPTOR, bundle.toJSONString());

    Account account = new Account(email, FxAccountConstants.ACCOUNT_TYPE);
    AccountManager accountManager = AccountManager.get(context);
    
    
    
    boolean added = accountManager.addAccountExplicitly(account, null, userdata);
    if (!added) {
      return null;
    }

    
    
    
    
    
    for (String key : userdata.keySet()) {
      accountManager.setUserData(account, key, userdata.getString(key));
    }

    AndroidFxAccount fxAccount = new AndroidFxAccount(context, account);

    if (!fromPickle) {
      fxAccount.clearSyncPrefs();
    }

    if (syncEnabled) {
      fxAccount.enableSyncing();
    } else {
      fxAccount.disableSyncing();
    }

    return fxAccount;
  }

  public void clearSyncPrefs() throws UnsupportedEncodingException, GeneralSecurityException {
    getSyncPrefs().edit().clear().commit();
  }

  public static Iterable<String> getAndroidAuthorities() {
    return ANDROID_AUTHORITIES;
  }

  








  public boolean isSyncing() {
    boolean isSyncEnabled = true;
    for (String authority : getAndroidAuthorities()) {
      isSyncEnabled &= ContentResolver.getSyncAutomatically(account, authority);
    }
    return isSyncEnabled;
  }

  public void enableSyncing() {
    Logger.info(LOG_TAG, "Enabling sync for account named like " + getObfuscatedEmail());
    for (String authority : getAndroidAuthorities()) {
      ContentResolver.setSyncAutomatically(account, authority, true);
      ContentResolver.setIsSyncable(account, authority, 1);
    }
  }

  public void disableSyncing() {
    Logger.info(LOG_TAG, "Disabling sync for account named like " + getObfuscatedEmail());
    for (String authority : getAndroidAuthorities()) {
      ContentResolver.setSyncAutomatically(account, authority, false);
    }
  }

  




  public boolean isCurrentlySyncing() {
    boolean active = false;
    for (String authority : AndroidFxAccount.getAndroidAuthorities()) {
      active |= ContentResolver.isSyncActive(account, authority);
    }
    return active;
  }

  


  public void requestSync() {
    requestSync(FirefoxAccounts.SOON, null, null);
  }

  




  public void requestSync(EnumSet<FirefoxAccounts.SyncHint> syncHints) {
    requestSync(syncHints, null, null);
  }

  






  public void requestSync(EnumSet<FirefoxAccounts.SyncHint> syncHints, String[] stagesToSync, String[] stagesToSkip) {
    FirefoxAccounts.requestSync(getAndroidAccount(), syncHints, stagesToSync, stagesToSkip);
  }

  public synchronized void setState(State state) {
    if (state == null) {
      throw new IllegalArgumentException("state must not be null");
    }
    Logger.info(LOG_TAG, "Moving account named like " + getObfuscatedEmail() +
        " to state " + state.getStateLabel().toString());
    updateBundleValue(BUNDLE_KEY_STATE_LABEL, state.getStateLabel().name());
    updateBundleValue(BUNDLE_KEY_STATE, state.toJSONObject().toJSONString());
    broadcastAccountStateChangedIntent();
  }

  protected void broadcastAccountStateChangedIntent() {
    final Intent intent = new Intent(FxAccountConstants.ACCOUNT_STATE_CHANGED_ACTION);
    intent.putExtra(Constants.JSON_KEY_ACCOUNT, account.name);
    context.sendBroadcast(intent, FxAccountConstants.PER_ACCOUNT_TYPE_PERMISSION);
  }

  public synchronized State getState() {
    String stateLabelString = getBundleData(BUNDLE_KEY_STATE_LABEL);
    String stateString = getBundleData(BUNDLE_KEY_STATE);
    if (stateLabelString == null) {
      throw new IllegalStateException("stateLabelString must not be null");
    }
    if (stateString == null) {
      throw new IllegalStateException("stateString must not be null");
    }

    try {
      StateLabel stateLabel = StateLabel.valueOf(stateLabelString);
      return StateFactory.fromJSONObject(stateLabel, new ExtendedJSONObject(stateString));
    } catch (Exception e) {
      throw new IllegalStateException("could not get state", e);
    }
  }

  


  public void dump() {
    if (!FxAccountConstants.LOG_PERSONAL_INFORMATION) {
      return;
    }
    ExtendedJSONObject o = toJSONObject();
    ArrayList<String> list = new ArrayList<String>(o.keySet());
    Collections.sort(list);
    for (String key : list) {
      FxAccountConstants.pii(LOG_TAG, key + ": " + o.get(key));
    }
  }

  







  public String getEmail() {
    return account.name;
  }

  






  public String getObfuscatedEmail() {
    return Utils.obfuscateEmail(account.name);
  }

  








  public static Intent makeDeletedAccountIntent(final Context context, final Account account) {
    final Intent intent = new Intent(FxAccountConstants.ACCOUNT_DELETED_ACTION);

    intent.putExtra(FxAccountConstants.ACCOUNT_DELETED_INTENT_VERSION_KEY,
        Long.valueOf(FxAccountConstants.ACCOUNT_DELETED_INTENT_VERSION));
    intent.putExtra(FxAccountConstants.ACCOUNT_DELETED_INTENT_ACCOUNT_KEY, account.name);
    return intent;
  }
}
