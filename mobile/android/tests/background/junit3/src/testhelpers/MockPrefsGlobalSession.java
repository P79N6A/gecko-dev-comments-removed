


package org.mozilla.gecko.background.testhelpers;

import java.io.IOException;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.SyncConfigurationException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.delegates.ClientsDataDelegate;
import org.mozilla.gecko.sync.delegates.GlobalSessionCallback;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;




public class MockPrefsGlobalSession extends GlobalSession {

  public MockSharedPreferences prefs;

  public MockPrefsGlobalSession(String userAPI, String serverURL,
      String username, String password, String prefsPath,
      KeyBundle syncKeyBundle, GlobalSessionCallback callback, Context context,
      Bundle extras, ClientsDataDelegate clientsDelegate)
      throws SyncConfigurationException, IllegalArgumentException, IOException,
      ParseException, NonObjectJSONException {
    super(userAPI, serverURL, username, password, prefsPath, syncKeyBundle,
        callback, context, extras, clientsDelegate);
  }

  @Override
  public SharedPreferences getPrefs(String name, int mode) {
    if (prefs == null) {
      prefs = new MockSharedPreferences();
    }
    return prefs;
  }

  @Override
  public Context getContext() {
    return null;
  }

}