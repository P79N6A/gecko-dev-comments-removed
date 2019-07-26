



package org.mozilla.gecko.sync;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.crypto.PersistedCrypto5Keys;
import org.mozilla.gecko.sync.stage.GlobalSyncStage.Stage;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

public class SyncConfiguration implements CredentialsSource {

  public class EditorBranch implements Editor {

    private String prefix;
    private Editor editor;

    public EditorBranch(SyncConfiguration config, String prefix) {
      if (!prefix.endsWith(".")) {
        throw new IllegalArgumentException("No trailing period in prefix.");
      }
      this.prefix = prefix;
      this.editor = config.getEditor();
    }

    public void apply() {
      
      this.editor.commit();
    }

    @Override
    public Editor clear() {
      this.editor = this.editor.clear();
      return this;
    }

    @Override
    public boolean commit() {
      return this.editor.commit();
    }

    @Override
    public Editor putBoolean(String key, boolean value) {
      this.editor = this.editor.putBoolean(prefix + key, value);
      return this;
    }

    @Override
    public Editor putFloat(String key, float value) {
      this.editor = this.editor.putFloat(prefix + key, value);
      return this;
    }

    @Override
    public Editor putInt(String key, int value) {
      this.editor = this.editor.putInt(prefix + key, value);
      return this;
    }

    @Override
    public Editor putLong(String key, long value) {
      this.editor = this.editor.putLong(prefix + key, value);
      return this;
    }

    @Override
    public Editor putString(String key, String value) {
      this.editor = this.editor.putString(prefix + key, value);
      return this;
    }

    
    
    public Editor putStringSet(String key, Set<String> value) {
      throw new RuntimeException("putStringSet not available.");
    }

    @Override
    public Editor remove(String key) {
      this.editor = this.editor.remove(prefix + key);
      return this;
    }

  }

  





  public class ConfigurationBranch implements SharedPreferences {

    private SyncConfiguration config;
    private String prefix;                

    public ConfigurationBranch(SyncConfiguration syncConfiguration,
        String prefix) {
      if (!prefix.endsWith(".")) {
        throw new IllegalArgumentException("No trailing period in prefix.");
      }
      this.config = syncConfiguration;
      this.prefix = prefix;
    }

    @Override
    public boolean contains(String key) {
      return config.getPrefs().contains(prefix + key);
    }

    @Override
    public Editor edit() {
      return new EditorBranch(config, prefix);
    }

    @Override
    public Map<String, ?> getAll() {
      
      return null;
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
      return config.getPrefs().getBoolean(prefix + key, defValue);
    }

    @Override
    public float getFloat(String key, float defValue) {
      return config.getPrefs().getFloat(prefix + key, defValue);
    }

    @Override
    public int getInt(String key, int defValue) {
      return config.getPrefs().getInt(prefix + key, defValue);
    }

    @Override
    public long getLong(String key, long defValue) {
      return config.getPrefs().getLong(prefix + key, defValue);
    }

    @Override
    public String getString(String key, String defValue) {
      return config.getPrefs().getString(prefix + key, defValue);
    }

    
    
    public Set<String> getStringSet(String key, Set<String> defValue) {
      throw new RuntimeException("getStringSet not available.");
    }

    @Override
    public void registerOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener listener) {
      config.getPrefs().registerOnSharedPreferenceChangeListener(listener);
    }

    @Override
    public void unregisterOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener listener) {
      config.getPrefs().unregisterOnSharedPreferenceChangeListener(listener);
    }
  }

  public static final String DEFAULT_USER_API = "https://auth.services.mozilla.com/user/1.0/";

  private static final String LOG_TAG = "SyncConfiguration";

  
  public String          userAPI;
  public URI             serverURL;
  public URI             clusterURL;
  public String          username;
  public KeyBundle       syncKeyBundle;

  public CollectionKeys  collectionKeys;
  public InfoCollections infoCollections;
  public MetaGlobal      metaGlobal;
  public String          password;
  public String          syncID;

  








  public Set<String> enabledEngineNames;

  









  public Collection<String> stagesToSync;

  



















  public Map<String, Boolean> userSelectedEngines;
  public long userSelectedEnginesTimestamp;

  
  
  
  public String          prefsPath;
  public PrefsSource     prefsSource;

  public static final String PREF_PREFS_VERSION = "prefs.version";
  public static final long CURRENT_PREFS_VERSION = 1;

  public static final String CLIENTS_COLLECTION_TIMESTAMP = "serverClientsTimestamp";  
  public static final String CLIENT_RECORD_TIMESTAMP = "serverClientRecordTimestamp";  

  public static final String PREF_CLUSTER_URL = "clusterURL";
  public static final String PREF_SYNC_ID = "syncID";

  public static final String PREF_ENABLED_ENGINE_NAMES = "enabledEngineNames";
  public static final String PREF_USER_SELECTED_ENGINES_TO_SYNC = "userSelectedEngines";
  public static final String PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP = "userSelectedEnginesTimestamp";

  public static final String PREF_EARLIEST_NEXT_SYNC = "earliestnextsync";
  public static final String PREF_CLUSTER_URL_IS_STALE = "clusterurlisstale";

  public static final String PREF_ACCOUNT_GUID = "account.guid";
  public static final String PREF_CLIENT_NAME = "account.clientName";
  public static final String PREF_NUM_CLIENTS = "account.numClients";

  



  public SyncConfiguration(String prefsPath, PrefsSource prefsSource) {
    this.prefsPath   = prefsPath;
    this.prefsSource = prefsSource;
    this.loadFromPrefs(getPrefs());
  }

  public SharedPreferences getPrefs() {
    Logger.trace(LOG_TAG, "Returning prefs for " + prefsPath);
    return prefsSource.getPrefs(prefsPath, Utils.SHARED_PREFERENCES_MODE);
  }

  




  public static Set<String> validEngineNames() {
    Set<String> engineNames = new HashSet<String>();
    for (Stage stage : Stage.getNamedStages()) {
      engineNames.add(stage.getRepositoryName());
    }
    return engineNames;
  }

  





  public ConfigurationBranch getBranch(String prefix) {
    return new ConfigurationBranch(this, prefix);
  }

  







  public static Set<String> getEnabledEngineNames(SharedPreferences prefs) {
    String json = prefs.getString(PREF_ENABLED_ENGINE_NAMES, null);
    if (json == null) {
      return null;
    }
    try {
      ExtendedJSONObject o = ExtendedJSONObject.parseJSONObject(json);
      return new HashSet<String>(o.keySet());
    } catch (Exception e) {
      
      return null;
    }
  }

  








  public static Map<String, Boolean> getUserSelectedEngines(SharedPreferences prefs) {
    String json = prefs.getString(PREF_USER_SELECTED_ENGINES_TO_SYNC, null);
    if (json == null) {
      return null;
    }
    try {
      ExtendedJSONObject o = ExtendedJSONObject.parseJSONObject(json);
      Map<String, Boolean> map = new HashMap<String, Boolean>();
      for (Entry<String, Object> e : o.entryIterable()) {
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
    for (Entry<String, Boolean> e : selectedEngines.entrySet()) {
      jObj.put(e.getKey(), e.getValue());
    }
    String json = jObj.toJSONString();
    long currentTime = System.currentTimeMillis();
    Editor edit = prefs.edit();
    edit.putString(PREF_USER_SELECTED_ENGINES_TO_SYNC, json);
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
    userSelectedEngines = getUserSelectedEngines(prefs);
    userSelectedEnginesTimestamp = prefs.getLong(PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP, 0);
    
    
    
  }

  public void persistToPrefs() {
    this.persistToPrefs(this.getPrefs());
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
      ExtendedJSONObject o = new ExtendedJSONObject();
      for (String engineName : enabledEngineNames) {
        o.put(engineName, 0);
      }
      edit.putString(PREF_ENABLED_ENGINE_NAMES, o.toJSONString());
    }
    if (userSelectedEngines == null) {
      edit.remove(PREF_USER_SELECTED_ENGINES_TO_SYNC);
      edit.remove(PREF_USER_SELECTED_ENGINES_TO_SYNC_TIMESTAMP);
    }
    
    
    edit.commit();
    
  }

  @Override
  public String credentials() {
    return username + ":" + password;
  }

  public CollectionKeys getCollectionKeys() {
    return collectionKeys;
  }

  public void setCollectionKeys(CollectionKeys k) {
    collectionKeys = k;
  }

  public String nodeWeaveURL() {
    return this.nodeWeaveURL((this.serverURL == null) ? null : this.serverURL.toASCIIString());
  }

  public String nodeWeaveURL(String serverURL) {
    String userPart = username + "/node/weave";
    if (serverURL == null) {
      return DEFAULT_USER_API + userPart;
    }
    if (!serverURL.endsWith("/")) {
      serverURL = serverURL + "/";
    }
    return serverURL + "user/1.0/" + userPart;
  }

  protected String infoBaseURL() {
    return clusterURL + GlobalSession.API_VERSION + "/" + username + "/info/";
  }

  public String infoCollectionsURL() {
    return infoBaseURL() + "collections";
  }

  public String infoCollectionCountsURL() {
    return infoBaseURL() + "collection_counts";
  }

  public String metaURL() {
    return clusterURL + GlobalSession.API_VERSION + "/" + username + "/storage/meta/global";
  }

  public String storageURL(boolean trailingSlash) {
    return clusterURL + GlobalSession.API_VERSION + "/" + username +
           (trailingSlash ? "/storage/" : "/storage");
  }

  public URI collectionURI(String collection) throws URISyntaxException {
    return new URI(storageURL(true) + collection);
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
    String uri = storageURL(true) + collection + uriParams;
    return new URI(uri);
  }

  public URI wboURI(String collection, String id) throws URISyntaxException {
    return new URI(storageURL(true) + collection + "/" + id);
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

  protected void setAndPersistClusterURL(URI u, SharedPreferences prefs) {
    boolean shouldPersist = (prefs != null) && (clusterURL == null);

    Logger.trace(LOG_TAG, "Setting cluster URL to " + u.toASCIIString() +
                          (shouldPersist ? ". Persisting." : ". Not persisting."));
    clusterURL = u;
    if (shouldPersist) {
      Editor edit = prefs.edit();
      edit.putString(PREF_CLUSTER_URL, clusterURL.toASCIIString());
      edit.commit();
    }
  }

  protected void setClusterURL(URI u, SharedPreferences prefs) {
    if (u == null) {
      Logger.warn(LOG_TAG, "Refusing to set cluster URL to null.");
      return;
    }
    URI uri = u.normalize();
    if (uri.toASCIIString().endsWith("/")) {
      setAndPersistClusterURL(u, prefs);
      return;
    }
    setAndPersistClusterURL(uri.resolve("/"), prefs);
    Logger.trace(LOG_TAG, "Set cluster URL to " + clusterURL.toASCIIString() + ", given input " + u.toASCIIString());
  }

  public void setClusterURL(URI u) {
    setClusterURL(u, this.getPrefs());
  }

  


  public Editor getEditor() {
    return this.getPrefs().edit();
  }

  



  public void persistServerClientRecordTimestamp(long timestamp) {
    getEditor().putLong(SyncConfiguration.CLIENT_RECORD_TIMESTAMP, timestamp).commit();
  }

  public long getPersistedServerClientRecordTimestamp() {
    return getPrefs().getLong(SyncConfiguration.CLIENT_RECORD_TIMESTAMP, 0);
  }

  public void persistServerClientsTimestamp(long timestamp) {
    getEditor().putLong(SyncConfiguration.CLIENTS_COLLECTION_TIMESTAMP, timestamp).commit();
  }

  public long getPersistedServerClientsTimestamp() {
    return getPrefs().getLong(SyncConfiguration.CLIENTS_COLLECTION_TIMESTAMP, 0);
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
