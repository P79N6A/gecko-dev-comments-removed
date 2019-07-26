



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State;

import android.os.Bundle;
import android.widget.TextView;




public class FxAccountVerifiedAccountActivity extends FxAccountAbstractActivity {
  private static final String LOG_TAG = FxAccountVerifiedAccountActivity.class.getSimpleName();

  protected AndroidFxAccount fxAccount;

  protected TextView emailText;

  public FxAccountVerifiedAccountActivity() {
    super(CANNOT_RESUME_WHEN_NO_ACCOUNTS_EXIST);
  }

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    super.onCreate(icicle);
    setContentView(R.layout.fxaccount_account_verified);

    emailText = (TextView) ensureFindViewById(null, R.id.email, "email text");
  }

  @Override
  public void onResume() {
    super.onResume();
    this.fxAccount = getAndroidFxAccount();
    if (fxAccount == null) {
      Logger.warn(LOG_TAG, "Could not get Firefox Account.");
      setResult(RESULT_CANCELED);
      finish();
      return;
    }
    State state = fxAccount.getState();
    if (!state.verified) {
      Logger.warn(LOG_TAG, "Firefox Account is not verified; not displaying verified account activity.");
      setResult(RESULT_CANCELED);
      finish();
      return;
    }
    emailText.setText(fxAccount.getEmail());
  }
}
