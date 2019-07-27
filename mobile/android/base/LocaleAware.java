



package org.mozilla.gecko;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.LocaleManager;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.StrictMode;
import android.support.v4.app.FragmentActivity;










public class LocaleAware {
  public static void initializeLocale(Context context) {
    final LocaleManager localeManager = BrowserLocaleManager.getInstance();
    final StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();
    StrictMode.allowThreadDiskWrites();
    try {
      localeManager.getAndApplyPersistedLocale(context);
    } finally {
      StrictMode.setThreadPolicy(savedPolicy);
    }
  }

  public static class LocaleAwareFragmentActivity extends FragmentActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
      LocaleAware.initializeLocale(getApplicationContext());
      super.onCreate(savedInstanceState);
    }
  }

  public static class LocaleAwareActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
      LocaleAware.initializeLocale(getApplicationContext());
      super.onCreate(savedInstanceState);
    }
  }
}
