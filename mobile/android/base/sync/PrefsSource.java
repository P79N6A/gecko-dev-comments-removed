



package org.mozilla.gecko.sync;

import android.content.SharedPreferences;











public interface PrefsSource {
  








  public SharedPreferences getPrefs(String name, int mode);
}
