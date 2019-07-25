




































package org.mozilla.gecko.sync;

import org.mozilla.gecko.R;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;




public class StubActivity extends Activity {
  private static final String LOG_TAG = "StubActivity";

  
  @Override
  public void onCreate(Bundle savedInstanceState) {
      Log.i(LOG_TAG, "In StubActivity onCreate.");
      super.onCreate(savedInstanceState);
      setContentView(R.layout.sync_stub);
      Log.i(LOG_TAG, "Done with onCreate.");
  }
}
