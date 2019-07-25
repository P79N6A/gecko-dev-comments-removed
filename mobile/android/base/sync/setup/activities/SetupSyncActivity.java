





































package org.mozilla.gecko.sync.setup.activities;

import org.json.simple.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.jpake.JPakeClient;
import org.mozilla.gecko.sync.setup.Constants;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

public class SetupSyncActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG = "SetupSync";
  private TextView            setupTitleView;
  private TextView            setupNoDeviceLinkTitleView;
  private TextView            setupSubtitleView;
  private TextView            pinTextView;
  private JPakeClient         jClient;
  private AccountManager      mAccountManager;
  private Context             mContext;

  public SetupSyncActivity() {
    super();
    Log.i(LOG_TAG, "SetupSyncActivity constructor called.");
  }

  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.i(LOG_TAG, "Called SetupSyncActivity.onCreate.");
    super.onCreate(savedInstanceState);
    setContentView(R.layout.sync_setup);

    
    setupTitleView = ((TextView) findViewById(R.id.setup_title));
    setupSubtitleView = (TextView) findViewById(R.id.setup_subtitle);
    setupNoDeviceLinkTitleView = (TextView) findViewById(R.id.link_nodevice);
    pinTextView = ((TextView) findViewById(R.id.text_pin));

    
    if (setupTitleView == null) {
      Log.e(LOG_TAG, "No title view.");
    }
    if (setupSubtitleView == null) {
      Log.e(LOG_TAG, "No subtitle view.");
    }
    if (setupNoDeviceLinkTitleView == null) {
      Log.e(LOG_TAG, "No 'no device' link view.");
    }

    
    mAccountManager = AccountManager.get(getApplicationContext());
    mContext = getApplicationContext();
  }

  @Override
  public void onResume() {
    Log.i(LOG_TAG, "Called SetupSyncActivity.onResume.");
    super.onResume();

    
    AccountManager mAccountManager = AccountManager.get(this);
    Account[] accts = mAccountManager
        .getAccountsByType(Constants.ACCOUNTTYPE_SYNC);
    if (accts.length > 0) {
      authSuccess(false);
      
      
      
    } else {
      
      jClient = new JPakeClient(this);
      jClient.receiveNoPin();
    }
  }

  
  public void manualClickHandler(View target) {
    Intent accountIntent = new Intent(this, AccountActivity.class);
    accountIntent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    startActivity(accountIntent);
    overridePendingTransition(0, 0);
  }

  public void cancelClickHandler(View target) {
    finish();
  }

  
  public void displayPin(String pin) {
    if (pin == null) {
      Log.w(LOG_TAG, "Asked to display null pin.");
      return;
    }
    
    int charPerLine = pin.length() / 3;
    String prettyPin = pin.substring(0, charPerLine) + "\n";
    prettyPin += pin.substring(charPerLine, 2 * charPerLine) + "\n";
    prettyPin += pin.substring(2 * charPerLine, pin.length());

    final String toDisplay = prettyPin;
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        TextView view = pinTextView;
        if (view == null) {
          Log.w(LOG_TAG, "Couldn't find view to display PIN.");
          return;
        }
        view.setText(toDisplay);
      }
    });
  }

  public void displayAbort(String error) {
    
    jClient = new JPakeClient(this);

    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        
        jClient.receiveNoPin();
      }
    });
  }

  



  public void onPaired() {
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Intent intent = new Intent(mContext, SetupWaitingActivity.class);
        
        startActivityForResult(intent, 0);
      }
    });
  }

  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    if (resultCode == RESULT_CANCELED) {
      displayAbort(Constants.JPAKE_ERROR_USERABORT);
    }
  }

  





  public void onComplete(JSONObject jCreds) {
    String accountName = (String) jCreds.get(Constants.JSON_KEY_ACCOUNT);
    String password    = (String) jCreds.get(Constants.JSON_KEY_PASSWORD);
    String syncKey     = (String) jCreds.get(Constants.JSON_KEY_SYNCKEY);
    String serverURL   = (String) jCreds.get(Constants.JSON_KEY_SERVER);

    final Intent intent = AccountActivity.createAccount(mAccountManager, accountName, syncKey, password, serverURL);
    setAccountAuthenticatorResult(intent.getExtras());

    setResult(RESULT_OK, intent);

    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        authSuccess(true);
      }
    });
  }

  private void authSuccess(boolean isSetup) {
    Intent intent = new Intent(mContext, SetupSuccessActivity.class);
    intent.putExtra(Constants.INTENT_EXTRA_IS_SETUP, isSetup);
    startActivity(intent);
    finish();
  }

  



  public void onPairingStart() {
    
    
  }
}
