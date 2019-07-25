




































package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.repositories.android.Authorities;
import org.mozilla.gecko.sync.setup.Constants;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.TextView;

public class AccountActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG = "AccountActivity";
  private AccountManager mAccountManager;
  private Context mContext;
  private String username;
  private String password;
  private String key;
  private String server;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.sync_account);
    mContext = getApplicationContext();
    mAccountManager = AccountManager.get(getApplicationContext());
  }

  protected void toggleServerField(boolean enabled) {
    TextView serverField = (TextView) findViewById(R.id.server);
    Log.i(LOG_TAG, "Toggling checkbox: " + enabled);
    serverField.setFocusable(enabled);
    serverField.setClickable(enabled);
  }

  @Override
  public void onStart() {
    super.onStart();
    
    setContentView(R.layout.sync_account);
    CheckBox serverCheckbox = (CheckBox) findViewById(R.id.checkbox_server);

    Log.i(LOG_TAG, "Setting onCheckedChangeListener.");
    OnCheckedChangeListener listener = new OnCheckedChangeListener() {

      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        Log.i(LOG_TAG, "Toggling checkbox: " + isChecked);
        toggleServerField(isChecked);
      }
    };
    serverCheckbox.setOnCheckedChangeListener(listener);
    
    listener.onCheckedChanged(serverCheckbox, serverCheckbox.isChecked());
  }

  public void cancelClickHandler(View target) {
    moveTaskToBack(true);
  }

  



  public void connectClickHandler(View target) {
    Log.d(LOG_TAG, "connectClickHandler for view " + target);
    username = ((EditText) findViewById(R.id.username)).getText().toString();
    password = ((EditText) findViewById(R.id.password)).getText().toString();
    key = ((EditText) findViewById(R.id.key)).getText().toString();
    CheckBox serverCheckbox = (CheckBox) findViewById(R.id.checkbox_server);
    EditText serverField = (EditText) findViewById(R.id.server);
    if (serverCheckbox.isChecked()) {
      server = serverField.getText().toString();
    }

    
    

    authCallback();
  }

  


  private void authCallback() {
    
    
    final Intent intent = createAccount(mAccountManager, username, key, password, server);
    setAccountAuthenticatorResult(intent.getExtras());

    
    

    

    
    setResult(RESULT_OK, intent);

    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        authSuccess();
      }
    });
  }

  
  public static Intent createAccount(AccountManager accountManager, String username, String syncKey, String password, String serverURL) {
    final Account account = new Account(username, Constants.ACCOUNTTYPE_SYNC);
    final Bundle userbundle = new Bundle();

    
    userbundle.putString(Constants.OPTION_SYNCKEY, syncKey);
    if (serverURL != null) {
      Log.i(LOG_TAG, "Setting explicit server URL: " + serverURL);
      userbundle.putString(Constants.OPTION_SERVER, serverURL);
    }
    accountManager.addAccountExplicitly(account, password, userbundle);

    Log.d(LOG_TAG, "Account: " + account.toString());

    
    ContentResolver.setMasterSyncAutomatically(true);
    ContentResolver.setSyncAutomatically(account, Authorities.BROWSER_AUTHORITY, true);
    
    
    Log.d(LOG_TAG, "Finished setting syncables.");

    final Intent intent = new Intent();
    intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, username);
    intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNTTYPE_SYNC);
    intent.putExtra(AccountManager.KEY_AUTHTOKEN, Constants.ACCOUNTTYPE_SYNC);
    return intent;
  }

  private void authFailure() {
    Intent intent = new Intent(mContext, SetupFailureActivity.class);
    startActivity(intent);
  }

  private void authSuccess() {
    Intent intent = new Intent(mContext, SetupSuccessActivity.class);
    startActivity(intent);
    finish();
  }
}
