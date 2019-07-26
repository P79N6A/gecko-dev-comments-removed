



package org.mozilla.gecko.sync.config;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.Utils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;














public class ConfigurationMigrator {
  public static final String LOG_TAG = "ConfigMigrator";

  







  protected static int copyPreferences(final SharedPreferences from, final Map<String, String> map, final Editor to) {
    int count = 0;

    
    for (Entry<String, ?> entry : from.getAll().entrySet()) {
      String fromKey = entry.getKey();
      String toKey = map.get(fromKey);
      if (toKey == null) {
        continue;
      }

      Object value = entry.getValue();
      if (value instanceof Boolean) {
         to.putBoolean(toKey, ((Boolean) value).booleanValue());
      } else if (value instanceof Float) {
         to.putFloat(toKey, ((Float) value).floatValue());
      } else if (value instanceof Integer) {
         to.putInt(toKey, ((Integer) value).intValue());
      } else if (value instanceof Long) {
         to.putLong(toKey, ((Long) value).longValue());
      } else if (value instanceof String) {
         to.putString(toKey, (String) value);
      } else {
        
      }

      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "' (" + value + ").");
      } else {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "'.");
      }
      count += 1;
    }

    return count;
  }

  protected final static String V0_PREF_CLUSTER_URL_IS_STALE = "clusterurlisstale";
  protected final static String V1_PREF_CLUSTER_URL_IS_STALE = V0_PREF_CLUSTER_URL_IS_STALE;
  protected final static String V0_PREF_EARLIEST_NEXT_SYNC = "earliestnextsync";
  protected final static String V1_PREF_EARLIEST_NEXT_SYNC = V0_PREF_EARLIEST_NEXT_SYNC;

  







  protected static int upgradeGlobals0to1(final SharedPreferences from, final SharedPreferences to) throws Exception {
    Map<String, String> map = new HashMap<String, String>();
    map.put(V0_PREF_CLUSTER_URL_IS_STALE, V1_PREF_CLUSTER_URL_IS_STALE);
    map.put(V0_PREF_EARLIEST_NEXT_SYNC, V1_PREF_EARLIEST_NEXT_SYNC);

    Editor editor = to.edit();
    int count = copyPreferences(from, map, editor);
    if (count > 0) {
      editor.commit();
    }
    return count;
  }

  







  protected static int downgradeGlobals1to0(final SharedPreferences from, final SharedPreferences to) throws Exception {
    Map<String, String> map = new HashMap<String, String>();
    map.put(V1_PREF_CLUSTER_URL_IS_STALE, V0_PREF_CLUSTER_URL_IS_STALE);
    map.put(V1_PREF_EARLIEST_NEXT_SYNC, V0_PREF_EARLIEST_NEXT_SYNC);

    Editor editor = to.edit();
    int count = copyPreferences(from, map, editor);
    if (count > 0) {
      editor.commit();
    }
    return count;
  }

  protected static final String V0_PREF_ACCOUNT_GUID = "account.guid";
  protected static final String V1_PREF_ACCOUNT_GUID = V0_PREF_ACCOUNT_GUID;
  protected static final String V0_PREF_CLIENT_NAME = "account.clientName";
  protected static final String V1_PREF_CLIENT_NAME = V0_PREF_CLIENT_NAME;
  protected static final String V0_PREF_NUM_CLIENTS = "account.numClients";
  protected static final String V1_PREF_NUM_CLIENTS = V0_PREF_NUM_CLIENTS;

  








  protected static int upgradeAndroidAccount0to1(final AccountManager accountManager, final Account account, final SharedPreferences to) throws Exception {
    final String V0_PREF_ACCOUNT_GUID = "account.guid";
    final String V1_PREF_ACCOUNT_GUID = V0_PREF_ACCOUNT_GUID;
    final String V0_PREF_CLIENT_NAME = "account.clientName";
    final String V1_PREF_CLIENT_NAME = V0_PREF_CLIENT_NAME;
    final String V0_PREF_NUM_CLIENTS = "account.numClients";
    final String V1_PREF_NUM_CLIENTS = V0_PREF_NUM_CLIENTS;

    String accountGUID = null;
    String clientName = null;
    long numClients = -1;
    try {
      accountGUID = accountManager.getUserData(account, V0_PREF_ACCOUNT_GUID);
    } catch (Exception e) {
      
    }
    try {
      clientName = accountManager.getUserData(account, V0_PREF_CLIENT_NAME);
    } catch (Exception e) {
      
    }
    try {
      numClients = Long.parseLong(accountManager.getUserData(account, V0_PREF_NUM_CLIENTS));
    } catch (Exception e) {
      
    }

    final Editor editor = to.edit();

    int count = 0;
    if (accountGUID != null) {
      final String fromKey = V0_PREF_ACCOUNT_GUID;
      final String toKey = V1_PREF_ACCOUNT_GUID;
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "' (" + accountGUID + ").");
      } else {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "'.");
      }
      editor.putString(toKey, accountGUID);
      count += 1;
    }
    if (clientName != null) {
      final String fromKey = V0_PREF_CLIENT_NAME;
      final String toKey = V1_PREF_CLIENT_NAME;
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "' (" + clientName + ").");
      } else {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "'.");
      }
      editor.putString(toKey, clientName);
      count += 1;
    }
    if (numClients > -1) {
      final String fromKey = V0_PREF_NUM_CLIENTS;
      final String toKey = V1_PREF_NUM_CLIENTS;
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "' (" + numClients + ").");
      } else {
        Logger.debug(LOG_TAG, "Migrated '" + fromKey + "' to '" + toKey + "'.");
      }
      editor.putLong(toKey, numClients);
      count += 1;
    }

    if (count > 0) {
      editor.commit();
    }
    return count;
  }

  








  protected static int downgradeAndroidAccount1to0(final SharedPreferences from, final AccountManager accountManager, final Account account) throws Exception {
    final String accountGUID = from.getString(V1_PREF_ACCOUNT_GUID, null);
    final String clientName = from.getString(V1_PREF_CLIENT_NAME, null);
    final long numClients = from.getLong(V1_PREF_NUM_CLIENTS, -1L);

    int count = 0;
    if (accountGUID != null) {
      Logger.debug(LOG_TAG, "Migrated account GUID.");
      accountManager.setUserData(account, V0_PREF_ACCOUNT_GUID, accountGUID);
      count += 1;
    }
    if (clientName != null) {
      Logger.debug(LOG_TAG, "Migrated client name.");
      accountManager.setUserData(account, V1_PREF_CLIENT_NAME, clientName);
      count += 1;
    }
    if (numClients > -1) {
      Logger.debug(LOG_TAG, "Migrated clients count.");
      accountManager.setUserData(account, V1_PREF_NUM_CLIENTS, new Long(numClients).toString());
      count += 1;
    }
    return count;
  }

  







  protected static int upgradeShared0to1(final SharedPreferences from, final SharedPreferences to) {
    final Map<String, String> map = new HashMap<String, String>();
    final String[] prefs = new String [] {
        "syncID",
        "clusterURL",
        "enabledEngineNames",

        "metaGlobalLastModified", "metaGlobalServerResponseBody",

        "crypto5KeysLastModified", "crypto5KeysServerResponseBody",

        "serverClientsTimestamp", "serverClientRecordTimestamp",

        "forms.remote", "forms.local", "forms.syncID",
        "tabs.remote", "tabs.local", "tabs.syncID",
        "passwords.remote", "passwords.local", "passwords.syncID",
        "history.remote", "history.local", "history.syncID",
        "bookmarks.remote", "bookmarks.local", "bookmarks.syncID",
    };
    for (String pref : prefs) {
      map.put(pref, pref);
    }

    Editor editor = to.edit();
    int count = copyPreferences(from, map, editor);
    if (count > 0) {
      editor.commit();
    }
    return count;
  }

  







  protected static int downgradeShared1to0(final SharedPreferences from, final SharedPreferences to) {
    
    return upgradeShared0to1(from, to);
  }

  public static void upgrade0to1(final Context context, final AccountManager accountManager, final Account account,
      final String product, final String username, final String serverURL, final String profile) throws Exception {

    final String GLOBAL_SHARED_PREFS = "sync.prefs.global";

    final SharedPreferences globalPrefs = context.getSharedPreferences(GLOBAL_SHARED_PREFS, Utils.SHARED_PREFERENCES_MODE);
    final SharedPreferences accountPrefs = Utils.getSharedPreferences(context, product, username, serverURL, profile, 0);
    final SharedPreferences newPrefs = Utils.getSharedPreferences(context, product, username, serverURL, profile, 1);

    upgradeGlobals0to1(globalPrefs, newPrefs);
    upgradeAndroidAccount0to1(accountManager, account, newPrefs);
    upgradeShared0to1(accountPrefs, newPrefs);
  }

  public static void downgrade1to0(final Context context, final AccountManager accountManager, final Account account,
      final String product, final String username, final String serverURL, final String profile) throws Exception {

    final String GLOBAL_SHARED_PREFS = "sync.prefs.global";

    final SharedPreferences globalPrefs = context.getSharedPreferences(GLOBAL_SHARED_PREFS, Utils.SHARED_PREFERENCES_MODE);
    final SharedPreferences accountPrefs = Utils.getSharedPreferences(context, product, username, serverURL, profile, 0);
    final SharedPreferences oldPrefs = Utils.getSharedPreferences(context, product, username, serverURL, profile, 1);

    downgradeGlobals1to0(oldPrefs, globalPrefs);
    downgradeAndroidAccount1to0(oldPrefs, accountManager, account);
    downgradeShared1to0(oldPrefs, accountPrefs);
  }

  

















  public static void ensurePrefsAreVersion(final long desiredVersion,
      final Context context, final AccountManager accountManager, final Account account,
      final String product, final String username, final String serverURL, final String profile) throws Exception {
    if (desiredVersion < 0 || desiredVersion > SyncConfiguration.CURRENT_PREFS_VERSION) {
      throw new IllegalArgumentException("Cannot migrate to unknown version " + desiredVersion + ".");
    }

    SharedPreferences versionPrefs = context.getSharedPreferences("sync.prefs.version", Utils.SHARED_PREFERENCES_MODE);

    
    
    
    long currentVersion = versionPrefs.getLong(SyncConfiguration.PREF_PREFS_VERSION, 0);
    if (currentVersion == desiredVersion) {
      Logger.info(LOG_TAG, "Current version (" + currentVersion + ") is desired version; no need to migrate.");
      return;
    }

    if (currentVersion < 0 || currentVersion > SyncConfiguration.CURRENT_PREFS_VERSION) {
      throw new IllegalStateException("Cannot migrate from unknown version " + currentVersion + ".");
    }

    
    if (currentVersion == 0 && desiredVersion == 1) {
      Logger.info(LOG_TAG, "Upgrading from version 0 to version 1.");
      upgrade0to1(context, accountManager, account, product, username, serverURL, profile);
    } else if (currentVersion == 1 && desiredVersion == 0) {
      Logger.info(LOG_TAG, "Upgrading from version 0 to version 1.");
      upgrade0to1(context, accountManager, account, product, username, serverURL, profile);
    } else {
      Logger.warn(LOG_TAG, "Don't know how to migrate from version " + currentVersion + " to " + desiredVersion + ".");
    }

    Logger.info(LOG_TAG, "Migrated from version " + currentVersion + " to version " + desiredVersion + ".");
    versionPrefs.edit().putLong(SyncConfiguration.PREF_PREFS_VERSION, desiredVersion).commit();
  }
}
