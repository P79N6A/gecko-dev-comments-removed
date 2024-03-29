



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.setup.activities.ActivityUtils;

import android.os.Bundle;
import android.widget.TextView;




public class FxAccountCreateAccountNotAllowedActivity extends FxAccountAbstractActivity {
  protected static final String LOG_TAG = FxAccountCreateAccountNotAllowedActivity.class.getSimpleName();

  public FxAccountCreateAccountNotAllowedActivity() {
    super(CANNOT_RESUME_WHEN_ACCOUNTS_EXIST);
  }

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    super.onCreate(icicle);
    setContentView(R.layout.fxaccount_create_account_not_allowed);
    TextView view = (TextView) findViewById(R.id.learn_more_link);
    ActivityUtils.linkTextView(view, R.string.fxaccount_account_create_not_allowed_learn_more, R.string.fxaccount_link_create_not_allowed);
  }
}
