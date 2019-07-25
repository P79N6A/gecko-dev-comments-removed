




































package org.mozilla.gecko.sync;

import android.content.Context;
import android.content.SharedPreferences;











public interface PrefsSource {
  public Context getContext();

  








  public SharedPreferences getPrefs(String name, int mode);
}
