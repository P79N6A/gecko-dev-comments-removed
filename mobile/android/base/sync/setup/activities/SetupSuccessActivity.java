



package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.SyncAccounts;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class SetupSuccessActivity extends SyncActivity {
  private TextView setupSubtitle;

  @Override
  public void onCreate(Bundle savedInstanceState) {
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
    super.onDestroy();
  }

  
  public void settingsClickHandler(View target) {
    SyncAccounts.openSyncSettings(this);
  }

  public void launchBrowser(View target) {
    ActivityUtils.openURLInFennec(this, null);
  }
}
