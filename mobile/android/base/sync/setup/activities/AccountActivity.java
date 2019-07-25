



package org.mozilla.gecko.sync.setup.activities;

import java.util.Locale;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.InvalidSyncKeyException;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.Toast;

public class AccountActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG        = "AccountActivity";

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
    enableCredEntry(false);
    
    try {
      key = ActivityUtils.validateSyncKey(synckeyInput.getText().toString());
    } catch (InvalidSyncKeyException e) {
      enableCredEntry(true);
      
      Toast toast = Toast.makeText(mContext, R.string.sync_new_recoverykey_status_incorrect, Toast.LENGTH_LONG);
      toast.show();
      return;
    }
    username = usernameInput.getText().toString().toLowerCase(Locale.US);
    password = passwordInput.getText().toString();
    if (serverCheckbox.isChecked()) {
      server = serverInput.getText().toString();
    }

    
    

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
    if (usernameInput.length() == 0 ||
        passwordInput.length() == 0 ||
        synckeyInput.length() == 0  ||
        (serverCheckbox.isChecked() &&
         serverInput.length() == 0)) {
      return false;
    }
    return true;
  }

  


  private void authCallback() {
    
    
    final SyncAccountParameters syncAccount = new SyncAccountParameters(mContext, mAccountManager,
        username, key, password, server);
    final Account account = SyncAccounts.createSyncAccount(syncAccount);
    final boolean result = (account != null);

    final Intent intent = new Intent(); 
    intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, syncAccount.username);
    intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNTTYPE_SYNC);
    intent.putExtra(AccountManager.KEY_AUTHTOKEN, Constants.ACCOUNTTYPE_SYNC);
    setAccountAuthenticatorResult(intent.getExtras());

    if (!result) {
      
      setResult(RESULT_CANCELED, intent);
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          authFailure();
        }
      });
      return;
    }

    
    

    
    setResult(RESULT_OK, intent);
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        authSuccess();
      }
    });
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
