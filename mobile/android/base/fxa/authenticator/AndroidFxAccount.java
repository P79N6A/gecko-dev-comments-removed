



package org.mozilla.gecko.fxa.authenticator;

import java.io.UnsupportedEncodingException;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Semaphore;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.background.ReadingListConstants;
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
import android.util.Log;









public class AndroidFxAccount {
  protected static final String LOG_TAG = AndroidFxAccount.class.getSimpleName();

  public static final int CURRENT_SYNC_PREFS_VERSION = 1;
  public static final int CURRENT_RL_PREFS_VERSION = 1;

  
  public static final int CURRENT_ACCOUNT_VERSION = 3;
  public static final String ACCOUNT_KEY_ACCOUNT_VERSION = "version";
  public static final String ACCOUNT_KEY_PROFILE = "profile";
  public static final String ACCOUNT_KEY_IDP_SERVER = "idpServerURI";

  public static final String ACCOUNT_KEY_TOKEN_SERVER = "tokenServerURI";       
  public static final String ACCOUNT_KEY_DESCRIPTOR = "descriptor";

  
  
  
  
  
  public static final String ACCOUNT_KEY_READING_LIST_AUTHORITY_INITIALIZED = "readingListAuthorityInitialized";

  public static final int CURRENT_BUNDLE_VERSION = 2;
  public static final String BUNDLE_KEY_BUNDLE_VERSION = "version";
  public static final String BUNDLE_KEY_STATE_LABEL = "stateLabel";
  public static final String BUNDLE_KEY_STATE = "state";

  
  
  
  
  private static final List<String> KNOWN_OAUTH_TOKEN_TYPES;
  static {
    final List<String> list = new ArrayList<>();
    if (AppConstants.MOZ_ANDROID_READING_LIST_SERVICE) {
      list.add(ReadingListConstants.AUTH_TOKEN_TYPE);
    }
    KNOWN_OAUTH_TOKEN_TYPES = Collections.unmodifiableList(list);
  }

  public static final Map<String, Boolean> DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP;
  static {
    final HashMap<String, Boolean> m = new HashMap<String, Boolean>();
    
    m.put(BrowserContract.AUTHORITY, true);
    if (AppConstants.MOZ_ANDROID_READING_LIST_SERVICE) {
      
      m.put(BrowserContract.READING_LIST_AUTHORITY, true);
    }
    DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP = Collections.unmodifiableMap(m);
  }

  private static final String PREF_KEY_LAST_SYNCED_TIMESTAMP = "lastSyncedTimestamp";

  protected final Context context;
  protected final AccountManager accountManager;
  protected final Account account;

  







  protected static final ConcurrentHashMap<String, ExtendedJSONObject> perAccountBundleCache =
      new ConcurrentHashMap<>();

  public static void invalidateCaches() {
    perAccountBundleCache.clear();
  }

  















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

  



  protected synchronized void persistBundle(ExtendedJSONObject bundle) {
    perAccountBundleCache.put(account.name, bundle);
    accountManager.setUserData(account, ACCOUNT_KEY_DESCRIPTOR, bundle.toJSONString());
  }

  protected ExtendedJSONObject unbundle() {
    return unbundle(true);
  }

  



  protected synchronized ExtendedJSONObject unbundle(boolean allowCachedBundle) {
    if (allowCachedBundle) {
      final ExtendedJSONObject cachedBundle = perAccountBundleCache.get(account.name);
      if (cachedBundle != null) {
        Logger.debug(LOG_TAG, "Returning cached account bundle.");
        return cachedBundle;
      }
    }

    final int version = getAccountVersion();
    if (version < CURRENT_ACCOUNT_VERSION) {
      
      
      return null;
    }

    if (version > CURRENT_ACCOUNT_VERSION) {
      
      return null;
    }

    String bundleString = accountManager.getUserData(account, ACCOUNT_KEY_DESCRIPTOR);
    if (bundleString == null) {
      return null;
    }
    final ExtendedJSONObject bundle = unbundleAccountV2(bundleString);
    perAccountBundleCache.put(account.name, bundle);
    Logger.info(LOG_TAG, "Account bundle persisted to cache.");
    return bundle;
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
    return b;
  }

  protected byte[] getBundleDataBytes(String key) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return null;
    }
    return o.getByteArrayHex(key);
  }

  protected void updateBundleValues(String key, String value, String... more) {
    if (more.length % 2 != 0) {
      throw new IllegalArgumentException("more must be a list of key, value pairs");
    }
    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      return;
    }
    descriptor.put(key, value);
    for (int i = 0; i + 1 < more.length; i += 2) {
      descriptor.put(more[i], more[i+1]);
    }
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

  public String getTokenServerURI() {
    return accountManager.getUserData(account, ACCOUNT_KEY_TOKEN_SERVER);
  }

  public String getOAuthServerURI() {
    
    if (FxAccountConstants.STAGE_AUTH_SERVER_ENDPOINT.equals(getAccountServerURI())) {
      return FxAccountConstants.STAGE_OAUTH_SERVER_ENDPOINT;
    } else {
      return FxAccountConstants.DEFAULT_OAUTH_SERVER_ENDPOINT;
    }
  }

  private String constructPrefsPath(String product, long version, String extra) throws GeneralSecurityException, UnsupportedEncodingException {
    String profile = getProfile();
    String username = account.name;

    if (profile == null) {
      throw new IllegalStateException("Missing profile. Cannot fetch prefs.");
    }

    if (username == null) {
      throw new IllegalStateException("Missing username. Cannot fetch prefs.");
    }

    final String fxaServerURI = getAccountServerURI();
    if (fxaServerURI == null) {
      throw new IllegalStateException("No account server URI. Cannot fetch prefs.");
    }

    
    final String serverURLThing = fxaServerURI + "!" + extra;
    return Utils.getPrefsPath(product, username, serverURLThing, profile, version);
  }

  


  public String getSyncPrefsPath() throws GeneralSecurityException, UnsupportedEncodingException {
    final String tokenServerURI = getTokenServerURI();
    if (tokenServerURI == null) {
      throw new IllegalStateException("No token server URI. Cannot fetch prefs.");
    }

    final String product = GlobalConstants.BROWSER_INTENT_PACKAGE + ".fxa";
    final long version = CURRENT_SYNC_PREFS_VERSION;
    return constructPrefsPath(product, version, tokenServerURI);
  }

  public String getReadingListPrefsPath() throws GeneralSecurityException, UnsupportedEncodingException {
    final String product = GlobalConstants.BROWSER_INTENT_PACKAGE + ".reading";
    final long version = CURRENT_RL_PREFS_VERSION;
    return constructPrefsPath(product, version, "");
  }

  public SharedPreferences getSyncPrefs() throws UnsupportedEncodingException, GeneralSecurityException {
    return context.getSharedPreferences(getSyncPrefsPath(), Utils.SHARED_PREFERENCES_MODE);
  }

  public SharedPreferences getReadingListPrefs() throws UnsupportedEncodingException, GeneralSecurityException {
    return context.getSharedPreferences(getReadingListPrefsPath(), Utils.SHARED_PREFERENCES_MODE);
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
      State state,
      final Map<String, Boolean> authoritiesToSyncAutomaticallyMap)
          throws UnsupportedEncodingException, GeneralSecurityException, URISyntaxException {
    return addAndroidAccount(context, email, profile, idpServerURI, tokenServerURI, state,
        authoritiesToSyncAutomaticallyMap,
        CURRENT_ACCOUNT_VERSION, false, null);
  }

  public static AndroidFxAccount addAndroidAccount(
      Context context,
      String email,
      String profile,
      String idpServerURI,
      String tokenServerURI,
      State state,
      final Map<String, Boolean> authoritiesToSyncAutomaticallyMap,
      final int accountVersion,
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
    userdata.putString(ACCOUNT_KEY_PROFILE, profile);
    if (DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP.containsKey(BrowserContract.READING_LIST_AUTHORITY)) {
      
      userdata.putString(ACCOUNT_KEY_READING_LIST_AUTHORITY_INITIALIZED, "1");
    }

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

    fxAccount.setAuthoritiesToSyncAutomaticallyMap(authoritiesToSyncAutomaticallyMap);

    return fxAccount;
  }

  public void clearSyncPrefs() throws UnsupportedEncodingException, GeneralSecurityException {
    getSyncPrefs().edit().clear().commit();
  }

  public void setAuthoritiesToSyncAutomaticallyMap(Map<String, Boolean> authoritiesToSyncAutomaticallyMap) {
    if (authoritiesToSyncAutomaticallyMap == null) {
      throw new IllegalArgumentException("authoritiesToSyncAutomaticallyMap must not be null");
    }

    for (String authority : DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP.keySet()) {
      boolean authorityEnabled = DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP.get(authority);
      final Boolean enabled = authoritiesToSyncAutomaticallyMap.get(authority);
      if (enabled != null) {
        authorityEnabled = enabled.booleanValue();
      }
      
      ContentResolver.setIsSyncable(account, authority, 1);
      
      ContentResolver.setSyncAutomatically(account, authority, authorityEnabled);
    }
  }

  public Map<String, Boolean> getAuthoritiesToSyncAutomaticallyMap() {
    final Map<String, Boolean> authoritiesToSync = new HashMap<>();
    for (String authority : DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP.keySet()) {
      final boolean enabled = ContentResolver.getSyncAutomatically(account, authority);
      authoritiesToSync.put(authority, enabled);
    }
    return authoritiesToSync;
  }

  




  public boolean isCurrentlySyncing() {
    boolean active = false;
    for (String authority : AndroidFxAccount.DEFAULT_AUTHORITIES_TO_SYNC_AUTOMATICALLY_MAP.keySet()) {
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
    updateBundleValues(
        BUNDLE_KEY_STATE_LABEL, state.getStateLabel().name(),
        BUNDLE_KEY_STATE, state.toJSONObject().toJSONString());
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
    if (stateLabelString == null || stateString == null) {
      throw new IllegalStateException("stateLabelString and stateString must not be null, but: " +
          "(stateLabelString == null) = " + (stateLabelString == null) +
          " and (stateString == null) = " + (stateString == null));
    }

    try {
      StateLabel stateLabel = StateLabel.valueOf(stateLabelString);
      Logger.debug(LOG_TAG, "Account is in state " + stateLabel);
      return StateFactory.fromJSONObject(stateLabel, new ExtendedJSONObject(stateString));
    } catch (Exception e) {
      throw new IllegalStateException("could not get state", e);
    }
  }

  


  public void dump() {
    if (!FxAccountUtils.LOG_PERSONAL_INFORMATION) {
      return;
    }
    ExtendedJSONObject o = toJSONObject();
    ArrayList<String> list = new ArrayList<String>(o.keySet());
    Collections.sort(list);
    for (String key : list) {
      FxAccountUtils.pii(LOG_TAG, key + ": " + o.get(key));
    }
  }

  







  public String getEmail() {
    return account.name;
  }

  






  public String getObfuscatedEmail() {
    return Utils.obfuscateEmail(account.name);
  }

  




  public Intent makeDeletedAccountIntent() {
    final Intent intent = new Intent(FxAccountConstants.ACCOUNT_DELETED_ACTION);
    final List<String> tokens = new ArrayList<>();

    intent.putExtra(FxAccountConstants.ACCOUNT_DELETED_INTENT_VERSION_KEY,
        Long.valueOf(FxAccountConstants.ACCOUNT_DELETED_INTENT_VERSION));
    intent.putExtra(FxAccountConstants.ACCOUNT_DELETED_INTENT_ACCOUNT_KEY, account.name);

    
    
    for (String tokenKey : KNOWN_OAUTH_TOKEN_TYPES) {
      final String authToken = accountManager.peekAuthToken(account, tokenKey);
      if (authToken != null) {
        tokens.add(authToken);
      }
    }

    
    intent.putExtra(FxAccountConstants.ACCOUNT_OAUTH_SERVICE_ENDPOINT_KEY, getOAuthServerURI());
    
    intent.putExtra(FxAccountConstants.ACCOUNT_DELETED_INTENT_ACCOUNT_AUTH_TOKENS, tokens.toArray(new String[tokens.size()]));
    return intent;
  }

  public void setLastSyncedTimestamp(long now) {
    try {
      getSyncPrefs().edit().putLong(PREF_KEY_LAST_SYNCED_TIMESTAMP, now).commit();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception setting last synced time; ignoring.", e);
    }
  }

  public long getLastSyncedTimestamp() {
    final long neverSynced = -1L;
    try {
      return getSyncPrefs().getLong(PREF_KEY_LAST_SYNCED_TIMESTAMP, neverSynced);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception getting last synced time; ignoring.", e);
      return neverSynced;
    }
  }

  
  public void unsafeTransitionToDefaultEndpoints() {
    unsafeTransitionToStageEndpoints(
        FxAccountConstants.DEFAULT_AUTH_SERVER_ENDPOINT,
        FxAccountConstants.DEFAULT_TOKEN_SERVER_ENDPOINT);
    }

  
  public void unsafeTransitionToStageEndpoints() {
    unsafeTransitionToStageEndpoints(
        FxAccountConstants.STAGE_AUTH_SERVER_ENDPOINT,
        FxAccountConstants.STAGE_TOKEN_SERVER_ENDPOINT);
  }

  protected void unsafeTransitionToStageEndpoints(String authServerEndpoint, String tokenServerEndpoint) {
    try {
      getReadingListPrefs().edit().clear().commit();
    } catch (UnsupportedEncodingException | GeneralSecurityException e) {
      
    }
    try {
      getSyncPrefs().edit().clear().commit();
    } catch (UnsupportedEncodingException | GeneralSecurityException e) {
      
    }
    State state = getState();
    setState(state.makeSeparatedState());
    accountManager.setUserData(account, ACCOUNT_KEY_IDP_SERVER, authServerEndpoint);
    accountManager.setUserData(account, ACCOUNT_KEY_TOKEN_SERVER, tokenServerEndpoint);
    ContentResolver.setIsSyncable(account, BrowserContract.READING_LIST_AUTHORITY, 1);
  }

  






  protected static final Semaphore sLock = new Semaphore(1, true );

  
  
  protected String lockTag = null;

  
  
  
  protected boolean locked = false;

  
  public synchronized void acquireSharedAccountStateLock(final String tag) throws InterruptedException {
    final long id = Thread.currentThread().getId();
    this.lockTag = tag;
    Log.d(Logger.DEFAULT_LOG_TAG, "Thread with tag and thread id acquiring lock: " + lockTag + ", " + id + " ...");
    sLock.acquire();
    locked = true;
    Log.d(Logger.DEFAULT_LOG_TAG, "Thread with tag and thread id acquiring lock: " + lockTag + ", " + id + " ... ACQUIRED");
  }

  
  public synchronized void releaseSharedAccountStateLock() {
    final long id = Thread.currentThread().getId();
    Log.d(Logger.DEFAULT_LOG_TAG, "Thread with tag and thread id releasing lock: " + lockTag + ", " + id + " ...");
    if (locked) {
      sLock.release();
      locked = false;
      Log.d(Logger.DEFAULT_LOG_TAG, "Thread with tag and thread id releasing lock: " + lockTag + ", " + id + " ... RELEASED");
    } else {
      Log.d(Logger.DEFAULT_LOG_TAG, "Thread with tag and thread id releasing lock: " + lockTag + ", " + id + " ... NOT LOCKED");
    }
  }

  @Override
  protected synchronized void finalize() {
    if (locked) {
      
      sLock.release();
      locked = false;
      final long id = Thread.currentThread().getId();
      Log.e(Logger.DEFAULT_LOG_TAG, "Thread with tag and thread id releasing lock: " + lockTag + ", " + id + " ... RELEASED DURING FINALIZE");
    }
  }
}
