



package org.mozilla.gecko.sync;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.CryptoRecord;

import android.content.SharedPreferences;

public class PersistedMetaGlobal {
  public static final String LOG_TAG = "PersistedMetaGlobal";

  public static final String META_GLOBAL_SERVER_RESPONSE_BODY = "metaGlobalServerResponseBody";
  public static final String META_GLOBAL_LAST_MODIFIED        = "metaGlobalLastModified";

  protected SharedPreferences prefs;

  public PersistedMetaGlobal(SharedPreferences prefs) {
    this.prefs = prefs;
  }

  










  public MetaGlobal metaGlobal(String metaUrl, String credentials) {
    String json = prefs.getString(META_GLOBAL_SERVER_RESPONSE_BODY, null);
    if (json == null) {
      return null;
    }
    MetaGlobal metaGlobal = null;
    try {
      CryptoRecord cryptoRecord = CryptoRecord.fromJSONRecord(json);
      MetaGlobal mg = new MetaGlobal(metaUrl, credentials);
      mg.setFromRecord(cryptoRecord);
      metaGlobal = mg;
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception decrypting persisted meta/global.", e);
    }
    return metaGlobal;
  }

  public void persistMetaGlobal(MetaGlobal metaGlobal) {
    if (metaGlobal == null) {
      Logger.debug(LOG_TAG, "Clearing persisted meta/global.");
      prefs.edit().remove(META_GLOBAL_SERVER_RESPONSE_BODY).commit();
      return;
    }
    try {
      CryptoRecord cryptoRecord = metaGlobal.asCryptoRecord();
      String json = cryptoRecord.toJSONString();
      Logger.debug(LOG_TAG, "Persisting meta/global.");
      prefs.edit().putString(META_GLOBAL_SERVER_RESPONSE_BODY, json).commit();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception encrypting while persisting meta/global.", e);
    }
  }

  public long lastModified() {
    return prefs.getLong(META_GLOBAL_LAST_MODIFIED, -1);
  }

  public void persistLastModified(long lastModified) {
    if (lastModified <= 0) {
      Logger.debug(LOG_TAG, "Clearing persisted meta/global last modified timestamp.");
      prefs.edit().remove(META_GLOBAL_LAST_MODIFIED).commit();
      return;
    }
    Logger.debug(LOG_TAG, "Persisting meta/global last modified timestamp " + lastModified + ".");
    prefs.edit().putLong(META_GLOBAL_LAST_MODIFIED, lastModified).commit();
  }

  public void purge() {
    persistLastModified(-1);
    persistMetaGlobal(null);
  }
}
