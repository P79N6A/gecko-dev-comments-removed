



package org.mozilla.gecko.sync;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.mozilla.gecko.background.common.PrefsBranch;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.crypto.PersistedCrypto5Keys;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

public class SyncConfiguration {
  private static final String LOG_TAG = "SyncConfiguration";

  
  public URI             clusterURL;
  public KeyBundle       syncKeyBundle;

  public CollectionKeys  collectionKeys;
  public InfoCollections infoCollections;
  public MetaGlobal      metaGlobal;
  public String          syncID;

  protected final String username;

  








  public Set<String> enabledEngineNames;
  public Set<String> declinedEngineNames = new HashSet<String>();

  









  public Collection<String> stagesToSync;

  



















  public Map<String, Boolean> userSelectedEngines;
  public long userSelectedEnginesTimestamp;

  public SharedPreferences prefs;

  protected final AuthHeaderProvider authHeaderProvider;

  public static final String PREF_PREFS_VERSION = "prefs.version";
  public static final long CURRENT_PREFS_VERSION = 1;

  public static final String CLIENTS_COLLECTION_TIMESTAMP = "serverClientsTimestamp";  
  public static final String CLIENT_RECORD_TIMESTAMP = "serverClientRecordTimestamp";  
  public static final String MIGRATION_SENTINEL_CHECK_TIMESTAMP = "migrationSentinelCheckTimestamp";  

  public static final String PREF_CLUSTER_URL = "clusterURL";
  public static final String PREF_SYNC_ID = "syncID";

  public static final String PREF_ENABLED_ENGINE_NAMES = "enabledEngineNames";
  public static final String PREF_DECLINED_ENGINE_NAMES = "declinedEngineNames";
  public static final String PREF_USER_SELECTED_ENGINES_TO_SYNC = "userSelectedEngines";
  public static final String PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP = "userSelectedEnginesTimestamp";

  public static final String PREF_CLUSTER_URL_IS_STALE = "clusterurlisstale";

  public static final String PREF_ACCOUNT_GUID = "account.guid";
  public static final String PREF_CLIENT_NAME = "account.clientName";
  public static final String PREF_NUM_CLIENTS = "account.numClients";
  public static final String PREF_CLIENT_DATA_TIMESTAMP = "account.clientDataTimestamp";

  private static final String API_VERSION = "1.5";

  public SyncConfiguration(String username, AuthHeaderProvider authHeaderProvider, SharedPreferences prefs) {
    this.username = username;
    this.authHeaderProvider = authHeaderProvider;
    this.prefs = prefs;
    this.loadFromPrefs(prefs);
  }

  public SyncConfiguration(String username, AuthHeaderProvider authHeaderProvider, SharedPreferences prefs, KeyBundle syncKeyBundle) {
    this(username, authHeaderProvider, prefs);
    this.syncKeyBundle = syncKeyBundle;
  }

  public String getAPIVersion() {
    return API_VERSION;
  }

  public SharedPreferences getPrefs() {
    return this.prefs;
  }

  




  public static Set<String> validEngineNames() {
    Set<String> engineNames = new HashSet<String>();
    for (Stage stage : Stage.getNamedStages()) {
      engineNames.add(stage.getRepositoryName());
    }
    return engineNames;
  }

  





  public PrefsBranch getBranch(String prefix) {
    return new PrefsBranch(this.getPrefs(), prefix);
  }

  









  protected static Set<String> getEngineNamesFromPref(SharedPreferences prefs, String pref) {
    final String json = prefs.getString(pref, null);
    if (json == null) {
      return null;
    }
    try {
      final ExtendedJSONObject o = ExtendedJSONObject.parseJSONObject(json);
      return new HashSet<String>(o.keySet());
    } catch (Exception e) {
      return null;
    }
  }

  



  public static Set<String> getEnabledEngineNames(SharedPreferences prefs) {
      return getEngineNamesFromPref(prefs, PREF_ENABLED_ENGINE_NAMES);
  }

  


  public static Set<String> getDeclinedEngineNames(SharedPreferences prefs) {
    final Set<String> names = getEngineNamesFromPref(prefs, PREF_DECLINED_ENGINE_NAMES);
    if (names == null) {
        return new HashSet<String>();
    }
    return names;
  }

  








  public static Map<String, Boolean> getUserSelectedEngines(SharedPreferences prefs) {
    String json = prefs.getString(PREF_USER_SELECTED_ENGINES_TO_SYNC, null);
    if (json == null) {
      return null;
    }
    try {
      ExtendedJSONObject o = ExtendedJSONObject.parseJSONObject(json);
      Map<String, Boolean> map = new HashMap<String, Boolean>();
      for (Entry<String, Object> e : o.entrySet()) {
        String key = e.getKey();
        Boolean value = (Boolean) e.getValue();
        map.put(key, value);
        
        if ("history".equals(key)) {
          map.put("forms", value);
        }
      }
      
      if (!map.containsKey("history")) {
        map.remove("forms");
      }
      return map;
    } catch (Exception e) {
      return null;
    }
  }

  










  public static void storeSelectedEnginesToPrefs(SharedPreferences prefs, Map<String, Boolean> selectedEngines) {
    ExtendedJSONObject jObj = new ExtendedJSONObject();
    HashSet<String> declined = new HashSet<String>();
    for (Entry<String, Boolean> e : selectedEngines.entrySet()) {
      final Boolean enabled = e.getValue();
      final String engine = e.getKey();
      jObj.put(engine, enabled);
      if (!enabled) {
        declined.add(engine);
      }
    }

    
    
    if (selectedEngines.containsKey("history") && !selectedEngines.get("history")) {
      declined.add("forms");
    }

    String json = jObj.toJSONString();
    long currentTime = System.currentTimeMillis();
    Editor edit = prefs.edit();
    edit.putString(PREF_USER_SELECTED_ENGINES_TO_SYNC, json);
    edit.putString(PREF_DECLINED_ENGINE_NAMES, setToJSONObjectString(declined));
    edit.putLong(PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP, currentTime);
    Logger.error(LOG_TAG, "Storing user-selected engines at [" + currentTime + "].");
    edit.commit();
  }

  public void loadFromPrefs(SharedPreferences prefs) {
    if (prefs.contains(PREF_CLUSTER_URL)) {
      String u = prefs.getString(PREF_CLUSTER_URL, null);
      try {
        clusterURL = new URI(u);
        Logger.trace(LOG_TAG, "Set clusterURL from bundle: " + u);
      } catch (URISyntaxException e) {
        Logger.warn(LOG_TAG, "Ignoring bundle clusterURL (" + u + "): invalid URI.", e);
      }
    }
    if (prefs.contains(PREF_SYNC_ID)) {
      syncID = prefs.getString(PREF_SYNC_ID, null);
      Logger.trace(LOG_TAG, "Set syncID from bundle: " + syncID);
    }
    enabledEngineNames = getEnabledEngineNames(prefs);
    declinedEngineNames = getDeclinedEngineNames(prefs);
    userSelectedEngines = getUserSelectedEngines(prefs);
    userSelectedEnginesTimestamp = prefs.getLong(PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP, 0);
    
    
    
  }

  public void persistToPrefs() {
    this.persistToPrefs(this.getPrefs());
  }

  private static String setToJSONObjectString(Set<String> set) {
    ExtendedJSONObject o = new ExtendedJSONObject();
    for (String name : set) {
      o.put(name, 0);
    }
    return o.toJSONString();
  }

  public void persistToPrefs(SharedPreferences prefs) {
    Editor edit = prefs.edit();
    if (clusterURL == null) {
      edit.remove(PREF_CLUSTER_URL);
    } else {
      edit.putString(PREF_CLUSTER_URL, clusterURL.toASCIIString());
    }
    if (syncID != null) {
      edit.putString(PREF_SYNC_ID, syncID);
    }
    if (enabledEngineNames == null) {
      edit.remove(PREF_ENABLED_ENGINE_NAMES);
    } else {
      edit.putString(PREF_ENABLED_ENGINE_NAMES, setToJSONObjectString(enabledEngineNames));
    }
    if (declinedEngineNames == null || declinedEngineNames.isEmpty()) {
      edit.remove(PREF_DECLINED_ENGINE_NAMES);
    } else {
      edit.putString(PREF_DECLINED_ENGINE_NAMES, setToJSONObjectString(declinedEngineNames));
    }
    if (userSelectedEngines == null) {
      edit.remove(PREF_USER_SELECTED_ENGINES_TO_SYNC);
      edit.remove(PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP);
    }
    
    
    edit.commit();
    
  }

  public AuthHeaderProvider getAuthHeaderProvider() {
    return authHeaderProvider;
  }

  public CollectionKeys getCollectionKeys() {
    return collectionKeys;
  }

  public void setCollectionKeys(CollectionKeys k) {
    collectionKeys = k;
  }

  




  public String storageURL() {
    return clusterURL + "/storage";
  }

  protected String infoBaseURL() {
    return clusterURL + "/info/";
  }

  public String infoCollectionsURL() {
    return infoBaseURL() + "collections";
  }

  public String infoCollectionCountsURL() {
    return infoBaseURL() + "collection_counts";
  }

  public String metaURL() {
    return storageURL() + "/meta/global";
  }

  public URI collectionURI(String collection) throws URISyntaxException {
    return new URI(storageURL() + "/" + collection);
  }

  public URI collectionURI(String collection, boolean full) throws URISyntaxException {
    
    
    boolean anyParams = full;
    String  uriParams = "";
    if (anyParams) {
      StringBuilder params = new StringBuilder("?");
      if (full) {
        params.append("full=1");
      }
      uriParams = params.toString();
    }
    String uri = storageURL() + "/" + collection + uriParams;
    return new URI(uri);
  }

  public URI wboURI(String collection, String id) throws URISyntaxException {
    return new URI(storageURL() + "/" + collection + "/" + id);
  }

  public URI keysURI() throws URISyntaxException {
    return wboURI("crypto", "keys");
  }

  public URI getClusterURL() {
    return clusterURL;
  }

  public String getClusterURLString() {
    if (clusterURL == null) {
      return null;
    }
    return clusterURL.toASCIIString();
  }

  public void setClusterURL(URI u) {
    this.clusterURL = u;
  }

  


  public Editor getEditor() {
    return this.getPrefs().edit();
  }

  



  public void persistServerClientRecordTimestamp(long timestamp) {
    getEditor().putLong(SyncConfiguration.CLIENT_RECORD_TIMESTAMP, timestamp).commit();
  }

  public long getPersistedServerClientRecordTimestamp() {
    return getPrefs().getLong(SyncConfiguration.CLIENT_RECORD_TIMESTAMP, 0L);
  }

  public void persistServerClientsTimestamp(long timestamp) {
    getEditor().putLong(SyncConfiguration.CLIENTS_COLLECTION_TIMESTAMP, timestamp).commit();
  }

  public long getPersistedServerClientsTimestamp() {
    return getPrefs().getLong(SyncConfiguration.CLIENTS_COLLECTION_TIMESTAMP, 0L);
  }

  public void persistLastMigrationSentinelCheckTimestamp(long timestamp) {
    getEditor().putLong(SyncConfiguration.MIGRATION_SENTINEL_CHECK_TIMESTAMP, timestamp).commit();
  }

  public long getLastMigrationSentinelCheckTimestamp() {
    return getPrefs().getLong(SyncConfiguration.MIGRATION_SENTINEL_CHECK_TIMESTAMP, 0L);
  }

  public void purgeCryptoKeys() {
    if (collectionKeys != null) {
      collectionKeys.clear();
    }
    persistedCryptoKeys().purge();
  }

  public void purgeMetaGlobal() {
    metaGlobal = null;
    persistedMetaGlobal().purge();
  }

  public PersistedCrypto5Keys persistedCryptoKeys() {
    return new PersistedCrypto5Keys(getPrefs(), syncKeyBundle);
  }

  public PersistedMetaGlobal persistedMetaGlobal() {
    return new PersistedMetaGlobal(getPrefs());
  }
}
