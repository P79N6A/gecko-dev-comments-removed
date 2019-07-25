





































package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.repositories.android.Authorities;
import org.mozilla.gecko.sync.setup.Constants;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;

public class AccountActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG        = "AccountActivity";

  private final static String DEFAULT_SERVER = "https://auth.services.mozilla.com/";

  private AccountManager      mAccountManager;
  private Context             mContext;
  private String              username;
  private String              password;
  private String              key;
  private String              server;

  
  private EditText            serverInput;
  private EditText            usernameInput;
  private EditText            passwordInput;
  private EditText            synckeyInput;
  private CheckBox            serverCheckbox;
  private Button              connectButton;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    setTheme(R.style.SyncTheme);
    super.onCreate(savedInstanceState);
    setContentView(R.layout.sync_account);
    mContext = getApplicationContext();
    Log.d(LOG_TAG, "AccountManager.get(" + mContext + ")");
    mAccountManager = AccountManager.get(mContext);

    
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
    serverCheckbox = (CheckBox) findViewById(R.id.checkbox_server);

    serverCheckbox.setOnCheckedChangeListener(new OnCheckedChangeListener() {
      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        Log.i(LOG_TAG, "Toggling checkbox: " + isChecked);
        
        if (!isChecked) { 
          serverInput.setVisibility(View.GONE);
          serverInput.setText("");
        } else {
          serverInput.setVisibility(View.VISIBLE);
        }
        
        activateView(connectButton, validateInputs());
      }
    });
  }

  @Override
  public void onStart() {
    super.onStart();
    
    usernameInput.setText("");
    passwordInput.setText("");
    synckeyInput.setText("");
    passwordInput.setText("");
  }

  public void cancelClickHandler(View target) {
    finish();
  }

  



  public void connectClickHandler(View target) {
    Log.d(LOG_TAG, "connectClickHandler for view " + target);
    username = usernameInput.getText().toString();
    password = passwordInput.getText().toString();
    key = synckeyInput.getText().toString();
    if (serverCheckbox.isChecked()) {
      server = serverInput.getText().toString();
    }
    enableCredEntry(false);

    
    

    authCallback();
  }

  
  private void enableCredEntry(boolean toEnable) {
    usernameInput.setEnabled(toEnable);
    passwordInput.setEnabled(toEnable);
    synckeyInput.setEnabled(toEnable);
    if (!toEnable) {
      serverInput.setEnabled(toEnable);
    } else {
      serverInput.setEnabled(serverCheckbox.isChecked());
    }
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
    if (usernameInput.length() == 0 || passwordInput.length() == 0
        || synckeyInput.length() == 0
        || (serverCheckbox.isChecked() && serverInput.length() == 0)) {
      return false;
    }
    return true;
  }

  


  private void authCallback() {
    
    
    Log.d(LOG_TAG, "Using account manager " + mAccountManager);
    final Intent intent = createAccount(mContext, mAccountManager,
                                        username,
                                        key, password, server);
    setAccountAuthenticatorResult(intent.getExtras());

    
    

    
    

    
    setResult(RESULT_OK, intent);

    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        authSuccess();
      }
    });
  }

  
  public static Intent createAccount(Context context,
                                     AccountManager accountManager,
                                     String username,
                                     String syncKey,
                                     String password, String serverURL) {

    final Account account = new Account(username, Constants.ACCOUNTTYPE_SYNC);
    final Bundle userbundle = new Bundle();

    
    userbundle.putString(Constants.OPTION_SYNCKEY, syncKey);
    if (serverURL != null) {
      Log.i(LOG_TAG, "Setting explicit server URL: " + serverURL);
      userbundle.putString(Constants.OPTION_SERVER, serverURL);
    } else {
      userbundle.putString(Constants.OPTION_SERVER, DEFAULT_SERVER);
    }
    Log.d(LOG_TAG, "Adding account for " + Constants.ACCOUNTTYPE_SYNC);
    boolean result = accountManager.addAccountExplicitly(account, password, userbundle);

    Log.d(LOG_TAG, "Account: " + account.toString() + " added successfully? " + result);
    if (!result) {
      Log.e(LOG_TAG, "Error adding account!");
    }

    
    ContentResolver.setMasterSyncAutomatically(true);
    ContentResolver.setSyncAutomatically(account, Authorities.BROWSER_AUTHORITY, true);

    
    
    Log.d(LOG_TAG, "Finished setting syncables.");

    
    Log.i(LOG_TAG, "Clearing preferences for this account.");
    try {
      Utils.getSharedPreferences(context, username, serverURL).edit().clear().commit();
    } catch (Exception e) {
      Log.e(LOG_TAG, "Could not clear prefs path!", e);
    }

    final Intent intent = new Intent();
    intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, username);
    intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNTTYPE_SYNC);
    intent.putExtra(AccountManager.KEY_AUTHTOKEN, Constants.ACCOUNTTYPE_SYNC);
    return intent;
  }

  private void authFailure() {
    enableCredEntry(true);
    Intent intent = new Intent(mContext, SetupFailureActivity.class);
    intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
    startActivity(intent);
  }

  private void authSuccess() {
    Intent intent = new Intent(mContext, SetupSuccessActivity.class);
    intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
    startActivity(intent);
    finish();
  }

  private void activateView(View view, boolean toActivate) {
    view.setEnabled(toActivate);
    view.setClickable(toActivate);
  }
}
