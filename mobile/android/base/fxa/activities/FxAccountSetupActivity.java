



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;




public class FxAccountSetupActivity extends Activity {
  protected static final String LOG_TAG = FxAccountSetupActivity.class.getSimpleName();

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    super.onCreate(icicle);
    setContentView(R.layout.fxaccount_setup);
  }

  @Override
  public void onResume() {
    Logger.debug(LOG_TAG, "onResume()");

    super.onResume();

    
    Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setClassName(AppConstants.ANDROID_PACKAGE_NAME,
        AppConstants.ANDROID_PACKAGE_NAME + ".App");
    intent.setData(Uri.parse("about:accounts"));

    startActivity(intent);
    finish();
  }
}
