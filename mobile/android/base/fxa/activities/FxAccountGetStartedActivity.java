



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;

import android.os.Bundle;




public class FxAccountGetStartedActivity extends FxAccountAbstractActivity {
  protected static final String LOG_TAG = FxAccountGetStartedActivity.class.getSimpleName();

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    super.onCreate(icicle);
    setContentView(R.layout.fxaccount_get_started);

    linkifyTextViews(null, new int[] { R.id.old_firefox });

    launchActivityOnClick(ensureFindViewById(null, R.id.get_started_button, "get started button"), FxAccountCreateAccountActivity.class);
  }
}
