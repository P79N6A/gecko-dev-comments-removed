


package org.mozilla.gecko.background.testhelpers;

import java.io.IOException;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.SyncConfigurationException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.delegates.GlobalSessionCallback;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.net.BasicAuthHeaderProvider;

import android.content.Context;
import android.content.SharedPreferences;




public class MockPrefsGlobalSession extends GlobalSession {

  public MockSharedPreferences prefs;

  public MockPrefsGlobalSession(
      SyncConfiguration config, GlobalSessionCallback callback, Context context,
      ClientsDataDelegate clientsDelegate)
      throws SyncConfigurationException, IllegalArgumentException, IOException,
      ParseException, NonObjectJSONException {
    super(config, callback, context, clientsDelegate, callback);
  }

  public static MockPrefsGlobalSession getSession(
      String username, String password,
      KeyBundle syncKeyBundle, GlobalSessionCallback callback, Context context,
      ClientsDataDelegate clientsDelegate)
      throws SyncConfigurationException, IllegalArgumentException, IOException,
      ParseException, NonObjectJSONException {
    return getSession(username, new BasicAuthHeaderProvider(username, password), null,
         syncKeyBundle, callback, context, clientsDelegate);
  }

  public static MockPrefsGlobalSession getSession(
      String username, AuthHeaderProvider authHeaderProvider, String prefsPath,
      KeyBundle syncKeyBundle, GlobalSessionCallback callback, Context context,
      ClientsDataDelegate clientsDelegate)
      throws SyncConfigurationException, IllegalArgumentException, IOException,
      ParseException, NonObjectJSONException {

    final SharedPreferences prefs = new MockSharedPreferences();
    final SyncConfiguration config = new SyncConfiguration(username, authHeaderProvider, prefs);
    config.syncKeyBundle = syncKeyBundle;
    return new MockPrefsGlobalSession(config, callback, context, clientsDelegate);
  }

  @Override
  public Context getContext() {
    return null;
  }
}
