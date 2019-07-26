



package org.mozilla.gecko.sync;

import java.net.URI;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;






public class Sync11Configuration extends SyncConfiguration {
  private static final String LOG_TAG = "Sync11Configuration";
  private static final String API_VERSION = "1.1";

  public Sync11Configuration(String username,
                             AuthHeaderProvider authHeaderProvider,
                             SharedPreferences prefs) {
    super(username, authHeaderProvider, prefs);
  }

  public Sync11Configuration(String username,
                             AuthHeaderProvider authHeaderProvider,
                             SharedPreferences prefs,
                             KeyBundle keyBundle) {
    super(username, authHeaderProvider, prefs, keyBundle);
  }

  @Override
  public String getAPIVersion() {
    return API_VERSION;
  }

  @Override
  public String storageURL() {
    return clusterURL + API_VERSION + "/" + username + "/storage";
  }

  @Override
  protected String infoBaseURL() {
    return clusterURL + API_VERSION + "/" + username + "/info/";
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

  @Override
  public void setClusterURL(URI u) {
    setClusterURL(u, this.getPrefs());
  }
}