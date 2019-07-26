


package org.mozilla.gecko.background.sync;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.mozilla.gecko.background.helpers.AndroidSyncTestCase;
import org.mozilla.gecko.sync.PrefsSource;
import org.mozilla.gecko.sync.SyncConfiguration;

import android.content.SharedPreferences;

public class TestSyncConfiguration extends AndroidSyncTestCase implements PrefsSource {
  public static final String TEST_PREFS_NAME = "test";

  


  @Override
  public SharedPreferences getPrefs(String name, int mode) {
    return this.getApplicationContext().getSharedPreferences(name, mode);
  }

  public void testEnabledEngineNames() {
    SyncConfiguration config = null;
    SharedPreferences prefs = getPrefs(TEST_PREFS_NAME, 0);

    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    config.enabledEngineNames = new HashSet<String>();
    config.enabledEngineNames.add("test1");
    config.enabledEngineNames.add("test2");
    config.persistToPrefs();
    assertTrue(prefs.contains(SyncConfiguration.PREF_ENABLED_ENGINE_NAMES));
    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    Set<String> expected = new HashSet<String>();
    for (String name : new String[] { "test1", "test2" }) {
      expected.add(name);
    }
    assertEquals(expected, config.enabledEngineNames);

    config.enabledEngineNames = null;
    config.persistToPrefs();
    assertFalse(prefs.contains(SyncConfiguration.PREF_ENABLED_ENGINE_NAMES));
    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    assertNull(config.enabledEngineNames);
  }

  public void testSyncID() {
    SyncConfiguration config = null;
    SharedPreferences prefs = getPrefs(TEST_PREFS_NAME, 0);

    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    config.syncID = "test1";
    config.persistToPrefs();
    assertTrue(prefs.contains(SyncConfiguration.PREF_SYNC_ID));
    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    assertEquals("test1", config.syncID);
  }

  public void testStoreSelectedEnginesToPrefs() {
    SharedPreferences prefs = getPrefs(TEST_PREFS_NAME, 0);
    
    Map<String, Boolean> expectedEngines = new HashMap<String, Boolean>();
    expectedEngines.put("test1", true);
    expectedEngines.put("test2", false);
    expectedEngines.put("test3", true);

    SyncConfiguration.storeSelectedEnginesToPrefs(prefs, expectedEngines);

    
    assertTrue(prefs.contains(SyncConfiguration.PREF_USER_SELECTED_ENGINES_TO_SYNC));
    SyncConfiguration config = null;
    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    config.loadFromPrefs(prefs);
    assertEquals(expectedEngines, config.userSelectedEngines);
  }

  


  public void testSelectedEnginesHistoryAndForms() {
    SharedPreferences prefs = getPrefs(TEST_PREFS_NAME, 0);
    
    Map<String, Boolean> storedEngines = new HashMap<String, Boolean>();
    storedEngines.put("history", true);

    SyncConfiguration.storeSelectedEnginesToPrefs(prefs, storedEngines);

    
    storedEngines.put("forms", true);
    
    assertTrue(prefs.contains(SyncConfiguration.PREF_USER_SELECTED_ENGINES_TO_SYNC));
    SyncConfiguration config = null;
    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    config.loadFromPrefs(prefs);
    assertEquals(storedEngines, config.userSelectedEngines);
  }

  public void testsSelectedEnginesNoHistoryNorForms() {
    SharedPreferences prefs = getPrefs(TEST_PREFS_NAME, 0);
    
    Map<String, Boolean> storedEngines = new HashMap<String, Boolean>();
    storedEngines.put("forms", true);

    SyncConfiguration.storeSelectedEnginesToPrefs(prefs, storedEngines);

    
    assertTrue(prefs.contains(SyncConfiguration.PREF_USER_SELECTED_ENGINES_TO_SYNC));
    SyncConfiguration config = null;
    config = new SyncConfiguration(TEST_PREFS_NAME, this);
    config.loadFromPrefs(prefs);
    
    assertTrue(config.userSelectedEngines.isEmpty());
  }
}
