



package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.GlobalConstants;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.setup.Constants;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.widget.TextView;

public class SetupSuccessActivity extends Activity {
  private final static String LOG_TAG = "SetupSuccessActivity";
  private TextView setupSubtitle;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    setTheme(R.style.SyncTheme);
    super.onCreate(savedInstanceState);
    Bundle extras = this.getIntent().getExtras();
    setContentView(R.layout.sync_setup_success);
    setupSubtitle = ((TextView) findViewById(R.id.setup_success_subtitle));
    if (extras != null) {
      boolean isSetup = extras.getBoolean(Constants.INTENT_EXTRA_IS_SETUP);
      if (!isSetup) {
        setupSubtitle.setText(getString(R.string.sync_subtitle_manage));
      }
    }
  }

  @Override
  public void onDestroy() {
    Logger.debug(LOG_TAG, "onDestroy() called.");
    super.onDestroy();
  }

  
  public void settingsClickHandler(View target) {
    Intent intent = new Intent(Settings.ACTION_SYNC_SETTINGS);
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    startActivity(intent);
  }

  public void launchBrowser(View target) {
    Intent intent = new Intent(Intent.ACTION_MAIN);
    intent.setClassName(GlobalConstants.BROWSER_INTENT_PACKAGE, GlobalConstants.BROWSER_INTENT_CLASS);
    
    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    startActivity(intent);
  }
}
