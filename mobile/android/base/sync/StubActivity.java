



package org.mozilla.gecko.sync;

import org.mozilla.gecko.R;

import android.app.Activity;
import android.os.Bundle;




public class StubActivity extends Activity {
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
      setContentView(R.layout.sync_stub);
  }
}
