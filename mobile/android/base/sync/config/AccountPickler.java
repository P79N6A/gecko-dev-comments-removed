



package org.mozilla.gecko.sync.config;

import java.io.FileOutputStream;
import java.io.PrintStream;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;

import android.accounts.Account;
import android.content.Context;

































public class AccountPickler {
  public static final String LOG_TAG = "AccountPickler";

  public static final long VERSION = 1;

  






  public static boolean deletePickle(final Context context, final String filename) {
    return context.deleteFile(filename);
  }

  
































  public static void pickle(final Context context, final String filename,
      final SyncAccountParameters params, final boolean syncAutomatically) {
    final ExtendedJSONObject o = params.asJSON();
    o.put(Constants.JSON_KEY_SYNC_AUTOMATICALLY, Boolean.valueOf(syncAutomatically));
    o.put(Constants.JSON_KEY_VERSION, new Long(VERSION));
    o.put(Constants.JSON_KEY_TIMESTAMP, new Long(System.currentTimeMillis()));

    PrintStream ps = null;
    try {
      final FileOutputStream fos = context.openFileOutput(filename, Context.MODE_PRIVATE);
      ps = new PrintStream(fos);
      ps.print(o.toJSONString());
      Logger.debug(LOG_TAG, "Persisted " + o.keySet().size() + " account settings to " + filename + ".");
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Caught exception persisting account settings to " + filename + "; ignoring.", e);
    } finally {
      if (ps != null) {
        ps.close();
      }
    }
  }

  








  public static Account unpickle(final Context context, final String filename) {
    final String jsonString = Utils.readFile(context, filename);
    if (jsonString == null) {
      Logger.info(LOG_TAG, "Pickle file '" + filename + "' not found; aborting.");
      return null;
    }

    ExtendedJSONObject json = null;
    try {
      json = ExtendedJSONObject.parseJSONObject(jsonString);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception reading pickle file '" + filename + "'; aborting.", e);
      return null;
    }

    SyncAccountParameters params = null;
    try {
      
      params = new SyncAccountParameters(context, null, json);
    } catch (IllegalArgumentException e) {
      Logger.warn(LOG_TAG, "Un-pickled data included null username, password, or serverURL; aborting.", e);
      return null;
    }

    
    boolean syncAutomatically = true;
    if (json.containsKey(Constants.JSON_KEY_SYNC_AUTOMATICALLY)) {
      if (Boolean.FALSE.equals(json.get(Constants.JSON_KEY_SYNC_AUTOMATICALLY))) {
        syncAutomatically = false;
      }
    }

    final Account account = SyncAccounts.createSyncAccountPreservingExistingPreferences(params, syncAutomatically);
    if (account == null) {
      Logger.warn(LOG_TAG, "Failed to add Android Account; aborting.");
      return null;
    }

    Integer version   = json.getIntegerSafely(Constants.JSON_KEY_VERSION);
    Integer timestamp = json.getIntegerSafely(Constants.JSON_KEY_TIMESTAMP);
    if (version == null || timestamp == null) {
      Logger.warn(LOG_TAG, "Did not find version or timestamp in pickle file; ignoring.");
      version = new Integer(-1);
      timestamp = new Integer(-1);
    }

    Logger.info(LOG_TAG, "Un-pickled Android account named " + params.username + " (version " + version + ", pickled at " + timestamp + ").");

    return account;
  }
}
