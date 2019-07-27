



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClient20.LoginResponse;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.StateLabel;

import android.content.Intent;





public class FxAccountFinishMigratingActivity extends FxAccountAbstractUpdateCredentialsActivity {
  protected static final String LOG_TAG = FxAccountFinishMigratingActivity.class.getSimpleName();

  public FxAccountFinishMigratingActivity() {
    super(R.layout.fxaccount_finish_migrating);
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
    final State state = fxAccount.getState();
    if (state.getStateLabel() != StateLabel.MigratedFromSync11) {
      Logger.warn(LOG_TAG, "Cannot finish migrating from Firefox Account in state: " + state.getStateLabel());
      setResult(RESULT_CANCELED);
      finish();
      return;
    }
    emailEdit.setText(fxAccount.getEmail());
  }

  @Override
  public Intent makeSuccessIntent(String email, LoginResponse result) {
    final Intent successIntent = new Intent(this, FxAccountMigrationFinishedActivity.class);
    
    
    successIntent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    return successIntent;
  }
}
