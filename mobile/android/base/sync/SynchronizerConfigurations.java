




































package org.mozilla.gecko.sync;

import java.io.IOException;
import java.util.HashMap;
import java.util.Set;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;

import android.os.Bundle;

public class SynchronizerConfigurations {
  private HashMap<String, SynchronizerConfiguration> engines;

  protected HashMap<String, SynchronizerConfiguration> enginesMapFromBundleV1(Bundle engineBundle) throws IOException, ParseException, NonObjectJSONException {
    HashMap<String, SynchronizerConfiguration> engines = new HashMap<String, SynchronizerConfiguration>();
    Set<String> keySet = engineBundle.keySet();
    for (String engine : keySet) {
      String[] values = engineBundle.getStringArray(engine);
      String syncID                        = values[0];
      RepositorySessionBundle remoteBundle = new RepositorySessionBundle(values[1]);
      RepositorySessionBundle localBundle  = new RepositorySessionBundle(values[2]);
      engines.put(engine, new SynchronizerConfiguration(syncID, remoteBundle, localBundle));
    }
    return engines;
  }

  public SynchronizerConfigurations(Bundle bundle) throws IOException, ParseException, NonObjectJSONException, UnknownSynchronizerConfigurationVersionException {
    if (bundle == null) {
      bundle = new Bundle();
    }

    Bundle engineBundle = bundle.getBundle("engines");
    if (engineBundle == null) {
      this.initializeDefaultEnginesMap();
      return;
    }
    int version = engineBundle.getInt("version");
    if (version == 0) {
      
      this.initializeDefaultEnginesMap();
      return;
    }
    if (version == 1) {
      engines = enginesMapFromBundleV1(engineBundle);
      return;
    }
    throw new UnknownSynchronizerConfigurationVersionException(version);
  }

  private void initializeDefaultEnginesMap() {
    engines = new HashMap<String, SynchronizerConfiguration>();
  }

  public SynchronizerConfiguration forEngine(String engineName) {
    return engines.get(engineName);
  }
}
