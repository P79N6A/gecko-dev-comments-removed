



package org.mozilla.gecko.fxa.activities;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountClient;
import org.mozilla.gecko.background.fxa.FxAccountClient10.RequestDelegate;
import org.mozilla.gecko.background.fxa.FxAccountClient20;
import org.mozilla.gecko.background.fxa.FxAccountClient20.LoginResponse;
import org.mozilla.gecko.background.fxa.FxAccountClientException.FxAccountClientRemoteException;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.background.fxa.PasswordStretcher;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.Engaged;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.tasks.FxAccountSignInTask;
import org.mozilla.gecko.sync.setup.activities.ActivityUtils;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;




public abstract class FxAccountAbstractUpdateCredentialsActivity extends FxAccountAbstractSetupActivity {
  protected static final String LOG_TAG = FxAccountAbstractUpdateCredentialsActivity.class.getSimpleName();

  protected AndroidFxAccount fxAccount;

  protected final int layoutResourceId;

  public FxAccountAbstractUpdateCredentialsActivity(int layoutResourceId) {
    
    
    
    
    super(CANNOT_RESUME_WHEN_NO_ACCOUNTS_EXIST);
    this.layoutResourceId = layoutResourceId;
  }

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    super.onCreate(icicle);
    setContentView(layoutResourceId);

    emailEdit = (AutoCompleteTextView) ensureFindViewById(null, R.id.email, "email edit");
    passwordEdit = (EditText) ensureFindViewById(null, R.id.password, "password edit");
    showPasswordButton = (Button) ensureFindViewById(null, R.id.show_password, "show password button");
    remoteErrorTextView = (TextView) ensureFindViewById(null, R.id.remote_error, "remote error text view");
    button = (Button) ensureFindViewById(null, R.id.button, "update credentials");
    progressBar = (ProgressBar) ensureFindViewById(null, R.id.progress, "progress bar");

    minimumPasswordLength = 1; 
    createButton();
    addListeners();
    updateButtonState();
    createShowPasswordButton();

    emailEdit.setEnabled(false);

    TextView view = (TextView) findViewById(R.id.forgot_password_link);
    ActivityUtils.linkTextView(view, R.string.fxaccount_sign_in_forgot_password, R.string.fxaccount_link_forgot_password);

    updateFromIntentExtras();
  }

  protected class UpdateCredentialsDelegate implements RequestDelegate<LoginResponse> {
    public final String email;
    public final String serverURI;
    public final PasswordStretcher passwordStretcher;

    public UpdateCredentialsDelegate(String email, PasswordStretcher passwordStretcher, String serverURI) {
      this.email = email;
      this.serverURI = serverURI;
      this.passwordStretcher = passwordStretcher;
    }

    @Override
    public void handleError(Exception e) {
      showRemoteError(e, R.string.fxaccount_update_credentials_unknown_error);
    }

    @Override
    public void handleFailure(FxAccountClientRemoteException e) {
      if (e.isUpgradeRequired()) {
        Logger.error(LOG_TAG, "Got upgrade required from remote server; transitioning Firefox Account to Doghouse state.");
        final State state = fxAccount.getState();
        fxAccount.setState(state.makeDoghouseState());
        
        redirectToActivity(FxAccountStatusActivity.class);
        return;
      }
      showRemoteError(e, R.string.fxaccount_update_credentials_unknown_error);
    }

    @Override
    public void handleSuccess(LoginResponse result) {
      Logger.info(LOG_TAG, "Got success signing in.");

      if (fxAccount == null) {
        this.handleError(new IllegalStateException("fxAccount must not be null"));
        return;
      }

      byte[] unwrapkB;
      try {
        
        
        
        
        
        byte[] quickStretchedPW = passwordStretcher.getQuickStretchedPW(result.remoteEmail.getBytes("UTF-8"));
        unwrapkB = FxAccountUtils.generateUnwrapBKey(quickStretchedPW);
      } catch (Exception e) {
        this.handleError(e);
        return;
      }
      fxAccount.setState(new Engaged(email, result.uid, result.verified, unwrapkB, result.sessionToken, result.keyFetchToken));
      fxAccount.requestSync(FirefoxAccounts.FORCE);

      
      if (FxAccountUtils.LOG_PERSONAL_INFORMATION) {
        fxAccount.dump();
      }

      setResult(RESULT_OK);

      
      final Intent successIntent = makeSuccessIntent(email, result);
      if (successIntent != null) {
        startActivity(successIntent);
      }
      finish();
    }
  }

  public void updateCredentials(String email, String password) {
    String serverURI = fxAccount.getAccountServerURI();
    Executor executor = Executors.newSingleThreadExecutor();
    FxAccountClient client = new FxAccountClient20(serverURI, executor);
    PasswordStretcher passwordStretcher = makePasswordStretcher(password);
    try {
      hideRemoteError();
      RequestDelegate<LoginResponse> delegate = new UpdateCredentialsDelegate(email, passwordStretcher, serverURI);
      new FxAccountSignInTask(this, this, email, passwordStretcher, client, delegate).execute();
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception updating credentials for account.", e);
      showRemoteError(e, R.string.fxaccount_update_credentials_unknown_error);
    }
  }

  protected void createButton() {
    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        final String email = emailEdit.getText().toString();
        final String password = passwordEdit.getText().toString();
        updateCredentials(email, password);
      }
    });
  }
}
