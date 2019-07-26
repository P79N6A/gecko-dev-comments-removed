



package org.mozilla.gecko.sync.setup.activities;

import android.app.Activity;
import android.os.Bundle;





public abstract class SyncActivity extends Activity {

  @Override
  protected void onResume() {
    super.onResume();
    ActivityUtils.prepareLogging();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    ActivityUtils.prepareLogging();
  }
}
