



package org.mozilla.gecko.sync.setup.activities;

import java.util.Locale;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.GlobalConstants;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.InvalidSyncKeyException;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;
import org.mozilla.gecko.sync.setup.auth.AccountAuthenticator;
import org.mozilla.gecko.sync.setup.auth.AuthenticationResult;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.Toast;

public class AccountActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG = "AccountActivity";

  private AccountManager      mAccountManager;
  private Context             mContext;
  private String              username;
  private String              password;
  private String              key;
  private String              server = Constants.AUTH_NODE_DEFAULT;

  
  private EditText            serverInput;
  private EditText            usernameInput;
  private EditText            passwordInput;
  private EditText            synckeyInput;
  private CheckBox            serverCheckbox;
  private Button              connectButton;
  private Button              cancelButton;
  private ProgressDialog      progressDialog;

  private AccountAuthenticator accountAuthenticator;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    setTheme(R.style.SyncTheme);
    super.onCreate(savedInstanceState);
    setContentView(R.layout.sync_account);
    mContext = getApplicationContext();
    Logger.debug(LOG_TAG, "AccountManager.get(" + mContext + ")");
    mAccountManager = AccountManager.get(mContext);

    
    Logger.debug(LOG_TAG, "Setting screen-on flag.");
    Window w = getWindow();
    w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    
    usernameInput = (EditText) findViewById(R.id.usernameInput);
    passwordInput = (EditText) findViewById(R.id.passwordInput);
    synckeyInput = (EditText) findViewById(R.id.keyInput);
    serverInput = (EditText) findViewById(R.id.serverInput);

    TextWatcher inputValidator = makeInputValidator();

    usernameInput.addTextChangedListener(inputValidator);
    passwordInput.addTextChangedListener(inputValidator);
    synckeyInput.addTextChangedListener(inputValidator);
    serverInput.addTextChangedListener(inputValidator);

    connectButton = (Button) findViewById(R.id.accountConnectButton);
    cancelButton = (Button) findViewById(R.id.accountCancelButton);
    serverCheckbox = (CheckBox) findViewById(R.id.checkbox_server);

    serverCheckbox.setOnCheckedChangeListener(new OnCheckedChangeListener() {
      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        Logger.info(LOG_TAG, "Toggling checkbox: " + isChecked);
        if (!isChecked) { 
          serverInput.setVisibility(View.GONE);
          findViewById(R.id.server_error).setVisibility(View.GONE);
          serverInput.setText("");
        } else {
          serverInput.setVisibility(View.VISIBLE);
          serverInput.setEnabled(true);
        }
        
        activateView(connectButton, validateInputs());
      }
    });
  }

  @Override
  public void onResume() {
    super.onResume();
    clearCredentials();
    usernameInput.requestFocus();
    cancelButton.setOnClickListener(new OnClickListener() {

      @Override
      public void onClick(View v) {
        cancelClickHandler(v);
      }

    });
  }

  public void cancelClickHandler(View target) {
    finish();
  }

  public void cancelConnectHandler(View target) {
    if (accountAuthenticator != null) {
      accountAuthenticator.isCanceled = true;
      accountAuthenticator = null;
    }
    displayVerifying(false);
    activateView(connectButton, true);
    clearCredentials();
    usernameInput.requestFocus();
  }

  private void clearCredentials() {
    
    passwordInput.setText("");
  }
  



  public void connectClickHandler(View target) {
    Logger.debug(LOG_TAG, "connectClickHandler for view " + target);
    
    try {
      key = ActivityUtils.validateSyncKey(synckeyInput.getText().toString());
    } catch (InvalidSyncKeyException e) {
      
      Toast toast = Toast.makeText(mContext, R.string.sync_new_recoverykey_status_incorrect, Toast.LENGTH_LONG);
      toast.show();
      return;
    }
    username = usernameInput.getText().toString().toLowerCase(Locale.US);
    password = passwordInput.getText().toString();
    key      = synckeyInput.getText().toString();
    server   = Constants.AUTH_NODE_DEFAULT;

    if (serverCheckbox.isChecked()) {
      String userServer = serverInput.getText().toString();
      if (userServer != null) {
        userServer = userServer.trim();
        if (userServer.length() != 0) {
          if (!userServer.startsWith("https://") &&
              !userServer.startsWith("http://")) {
            
            userServer = "https://" + userServer;
            serverInput.setText(userServer);
          }
          server = userServer;
        }
      }
    }

    clearErrors();
    displayVerifying(true);
    cancelButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        cancelConnectHandler(v);
        
        cancelButton.setOnClickListener(new OnClickListener() {
          public void onClick(View v) {
            cancelClickHandler(v);
          }
        });
      }
    });

    accountAuthenticator = new AccountAuthenticator(this);
    accountAuthenticator.authenticate(server, username, password);
  }

  private TextWatcher makeInputValidator() {
    return new TextWatcher() {

      @Override
      public void afterTextChanged(Editable s) {
        activateView(connectButton, validateInputs());
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count,
          int after) {
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }
    };
  }

  private boolean validateInputs() {
    if (usernameInput.length() == 0 ||
        passwordInput.length() == 0 ||
        synckeyInput.length() == 0  ||
        (serverCheckbox.isChecked() &&
         serverInput.length() == 0)) {
      return false;
    }
    return true;
  }

  


  public void authCallback(final AuthenticationResult result) {
    displayVerifying(false);
    if (result != AuthenticationResult.SUCCESS) {
      Logger.debug(LOG_TAG, "displayFailure()");
      displayFailure(result);
      return;
    }
    
    SyncAccountParameters syncAccount = new SyncAccountParameters(
        mContext, mAccountManager, username, key, password, server);
    createAccountOnThread(syncAccount);
  }

  private void createAccountOnThread(final SyncAccountParameters syncAccount) {
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        Account account = SyncAccounts.createSyncAccount(syncAccount);
        boolean isSuccess = (account != null);
        if (!isSuccess) {
          setResult(RESULT_CANCELED);
          runOnUiThread(new Runnable() {
            @Override
            public void run() {
              displayFailure(AuthenticationResult.FAILURE_ACCOUNT);
            }
          });
          return;
        }

        
        clearErrors();

        Bundle resultBundle = new Bundle();
        resultBundle.putString(AccountManager.KEY_ACCOUNT_NAME, syncAccount.username);
        resultBundle.putString(AccountManager.KEY_ACCOUNT_TYPE, GlobalConstants.ACCOUNTTYPE_SYNC);
        resultBundle.putString(AccountManager.KEY_AUTHTOKEN, GlobalConstants.ACCOUNTTYPE_SYNC);
        setAccountAuthenticatorResult(resultBundle);

        setResult(RESULT_OK);
        runOnUiThread(new Runnable() {
          @Override
          public void run() {
            authSuccess();
          }
        });
      }
    });
  }

  private void displayVerifying(final boolean isVerifying) {
    if (isVerifying) {
      progressDialog = ProgressDialog.show(AccountActivity.this, "", getString(R.string.sync_verifying_label), true);
    } else {
      progressDialog.dismiss();
    }
  }

  private void displayFailure(final AuthenticationResult result) {
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Intent intent;
        switch (result) {
        case FAILURE_USERNAME:
          
        case FAILURE_PASSWORD:
          findViewById(R.id.cred_error).setVisibility(View.VISIBLE);
          usernameInput.requestFocus();
          break;
        case FAILURE_SERVER:
          findViewById(R.id.server_error).setVisibility(View.VISIBLE);
          serverInput.requestFocus();
          break;
        case FAILURE_ACCOUNT:
          intent = new Intent(mContext, SetupFailureActivity.class);
          intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
          intent.putExtra(Constants.INTENT_EXTRA_IS_ACCOUNTERROR, true);
          startActivity(intent);
          break;
        case FAILURE_OTHER:
        default:
          
          Logger.debug(LOG_TAG, "displaying default failure.");
          intent = new Intent(mContext, SetupFailureActivity.class);
          intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
          startActivity(intent);
        }
      }
    });
    return;
  }

  


  public void authSuccess() {
    
    Intent intent = new Intent(mContext, SetupSuccessActivity.class);
    intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
    startActivity(intent);
    finish();
  }

  private void activateView(View view, boolean toActivate) {
    view.setEnabled(toActivate);
    view.setClickable(toActivate);
  }

  private void clearErrors() {
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        findViewById(R.id.cred_error).setVisibility(View.GONE);
        findViewById(R.id.server_error).setVisibility(View.GONE);
      }
    });
  }
}
