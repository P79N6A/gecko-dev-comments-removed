





































package org.mozilla.gecko.sync.setup.activities;

import org.json.simple.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.jpake.JPakeClient;
import org.mozilla.gecko.sync.jpake.JPakeNoActivePairingException;
import org.mozilla.gecko.sync.setup.Constants;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class SetupSyncActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG     = "SetupSync";

  private boolean             pairWithPin = false;

  
  private EditText            row1;
  private EditText            row2;
  private EditText            row3;
  private Button              connectButton;
  private LinearLayout        pinError;

  
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

    
    mContext = getApplicationContext();
    Log.d(LOG_TAG, "AccountManager.get(" + mContext + ")");
    mAccountManager = AccountManager.get(mContext);

  }

  @Override
  public void onResume() {
    Log.i(LOG_TAG, "Called SetupSyncActivity.onResume.");
    super.onResume();

    if (!hasInternet()) {
      setContentView(R.layout.sync_setup_nointernet);
      return;
    }
    
    
    Account[] accts = mAccountManager.getAccountsByType(Constants.ACCOUNTTYPE_SYNC);

    if (accts.length == 0) { 
      displayReceiveNoPin();
      jClient = new JPakeClient(this);
      jClient.receiveNoPin();
      return;
    }

    
    Bundle extras = this.getIntent().getExtras();
    if (extras != null) {
      boolean isSetup = extras.getBoolean(Constants.INTENT_EXTRA_IS_SETUP);
      if (!isSetup) {
        pairWithPin = true;
        displayPairWithPin();
        return;
      }
    }
    
    Toast toast = Toast.makeText(mContext, R.string.sync_notification_oneaccount, Toast.LENGTH_LONG);
    toast.show();
    finish();
  }

  @Override
  public void onPause() {
    super.onPause();

    if (jClient != null) {
      jClient.abort(Constants.JPAKE_ERROR_USERABORT);
    }
  }

  @Override
  public void onNewIntent(Intent intent) {
    setIntent(intent);
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

  public void connectClickHandler(View target) {
    
    pinError.setVisibility(View.INVISIBLE);
    enablePinEntry(false);
    connectButton.requestFocus();
    activateButton(connectButton, false);

    
    String pin = row1.getText().toString();
    pin += row2.getText().toString() + row3.getText().toString();

    
    jClient = new JPakeClient(this);
    jClient.pairWithPin(pin, false);
  }

  public void showClickHandler(View target) {
    Uri uri = null;
    
    if (pairWithPin) {
      uri = Uri.parse(Constants.LINK_FIND_CODE);
    } else {
      uri = Uri.parse(Constants.LINK_FIND_ADD_DEVICE);
    }
    startActivity(new Intent(Intent.ACTION_VIEW, uri));
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
    if (!Constants.JPAKE_ERROR_USERABORT.equals(error) && !hasInternet()) {
      setContentView(R.layout.sync_setup_nointernet);
      return;
    }
    if (pairWithPin) {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          enablePinEntry(true);
          row1.setText("");
          row2.setText("");
          row3.setText("");
          row1.requestFocus();

          
          pinError.setVisibility(View.VISIBLE);
        }
      });
      return;
    }

    
    Log.d(LOG_TAG, "abort reason: " + error);
    if (!Constants.JPAKE_ERROR_USERABORT.equals(error)) {
      jClient = new JPakeClient(this);
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          displayReceiveNoPin();
          jClient.receiveNoPin();
        }
      });
    }
  }

  



  @SuppressWarnings("unchecked")
  public void onPaired() {
    if (!pairWithPin) {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          setContentView(R.layout.sync_setup_jpake_waiting);
        }
      });
      return;
    }

    
    Account[] accts = mAccountManager.getAccountsByType(Constants.ACCOUNTTYPE_SYNC);
    if (accts.length == 0) {
      
      Log.e(LOG_TAG, "No accounts present.");
      displayAbort(Constants.JPAKE_ERROR_INVALID);
      return;
    }

    
    Account account = accts[0];
    String username  = account.name;
    String password  = mAccountManager.getPassword(account);
    String syncKey   = mAccountManager.getUserData(account, Constants.OPTION_SYNCKEY);
    String serverURL = mAccountManager.getUserData(account, Constants.OPTION_SERVER);

    JSONObject jAccount = new JSONObject();
    jAccount.put(Constants.JSON_KEY_SYNCKEY,  syncKey);
    jAccount.put(Constants.JSON_KEY_ACCOUNT,  username);
    jAccount.put(Constants.JSON_KEY_PASSWORD, password);
    jAccount.put(Constants.JSON_KEY_SERVER,   serverURL);

    Log.d(LOG_TAG, "Extracted account data: " + jAccount.toJSONString());
    try {
      jClient.sendAndComplete(jAccount);
    } catch (JPakeNoActivePairingException e) {
      Log.e(LOG_TAG, "No active J-PAKE pairing.", e);
      
    }
  }

  



  public void onPairingStart() {
    if (pairWithPin) {
      
    }
  }

  





  public void onComplete(JSONObject jCreds) {
    if (!pairWithPin) {
      String accountName  = (String) jCreds.get(Constants.JSON_KEY_ACCOUNT);
      String password     = (String) jCreds.get(Constants.JSON_KEY_PASSWORD);
      String syncKey      = (String) jCreds.get(Constants.JSON_KEY_SYNCKEY);
      String serverURL    = (String) jCreds.get(Constants.JSON_KEY_SERVER);

      Log.d(LOG_TAG, "Using account manager " + mAccountManager);
      final Intent intent = AccountActivity.createAccount(mContext, mAccountManager,
                                                          accountName,
                                                          syncKey, password, serverURL);
      setAccountAuthenticatorResult(intent.getExtras());

      setResult(RESULT_OK, intent);
    }

    jClient = null; 
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        displayAccount(true);
      }
    });
  }

  



  private void activateButton(Button button, boolean toActivate) {
    button.setEnabled(toActivate);
    button.setClickable(toActivate);
  }

  private void enablePinEntry(boolean toEnable) {
    row1.setEnabled(toEnable);
    row2.setEnabled(toEnable);
    row3.setEnabled(toEnable);
  }

  






  private void displayAccount(boolean isSetup) {
    Intent intent = new Intent(mContext, SetupSuccessActivity.class);
    intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
    intent.putExtra(Constants.INTENT_EXTRA_IS_SETUP, isSetup);
    startActivity(intent);
    finish();
  }

  





  private boolean pinEntryCompleted() {
    if (row1.length() == 4 &&
        row2.length() == 4 &&
        row3.length() == 4) {
      return true;
    }
    return false;
  }

  private boolean hasInternet() {
    Log.d(LOG_TAG, "Checking internet connectivity.");
    ConnectivityManager connManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo wifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
    NetworkInfo mobile = connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);

    if (wifi.isConnected() || mobile.isConnected()) {
      Log.d(LOG_TAG, "Internet connected.");
      return true;
    }
    return false;
  }

  private void displayPairWithPin() {
    setContentView(R.layout.sync_setup_pair);
    connectButton = (Button) findViewById(R.id.pair_button_connect);
    pinError = (LinearLayout) findViewById(R.id.pair_error);

    row1 = (EditText) findViewById(R.id.pair_row1);
    row2 = (EditText) findViewById(R.id.pair_row2);
    row3 = (EditText) findViewById(R.id.pair_row3);

    row1.addTextChangedListener(new TextWatcher() {
      @Override
      public void afterTextChanged(Editable s) {
        if (s.length() == 4) {
          row2.requestFocus();
        }
        activateButton(connectButton, pinEntryCompleted());
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count,
          int after) {
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }

    });
    row2.addTextChangedListener(new TextWatcher() {
      @Override
      public void afterTextChanged(Editable s) {
        if (s.length() == 4) {
          row3.requestFocus();
        }
        activateButton(connectButton, pinEntryCompleted());
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count,
          int after) {
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }

    });

    row3.addTextChangedListener(new TextWatcher() {
      @Override
      public void afterTextChanged(Editable s) {
        activateButton(connectButton, pinEntryCompleted());
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count,
          int after) {
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }

    });
  }

  private void displayReceiveNoPin() {
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
  }
}
