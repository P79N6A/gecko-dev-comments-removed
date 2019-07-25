



package org.mozilla.gecko.sync.setup.activities;

import java.util.HashMap;

import org.json.simple.JSONObject;
import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.jpake.JPakeClient;
import org.mozilla.gecko.sync.jpake.JPakeNoActivePairingException;
import org.mozilla.gecko.sync.setup.Constants;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class SetupSyncActivity extends AccountAuthenticatorActivity {
  private final static String LOG_TAG = "SetupSync";

  private boolean pairWithPin = false;

  
  private EditText            row1;
  private EditText            row2;
  private EditText            row3;
  private Button              connectButton;
  private LinearLayout        pinError;

  
  private TextView            setupTitleView;
  private TextView            setupNoDeviceLinkTitleView;
  private TextView            setupSubtitleView;
  private TextView            pinTextView1;
  private TextView            pinTextView2;
  private TextView            pinTextView3;
  private JPakeClient         jClient;

  
  private AccountManager      mAccountManager;
  private Context             mContext;

  public SetupSyncActivity() {
    super();
    Logger.info(LOG_TAG, "SetupSyncActivity constructor called.");
  }

  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    setTheme(R.style.SyncTheme);
    Logger.info(LOG_TAG, "Called SetupSyncActivity.onCreate.");
    super.onCreate(savedInstanceState);

    
    mContext = getApplicationContext();
    Logger.debug(LOG_TAG, "AccountManager.get(" + mContext + ")");
    mAccountManager = AccountManager.get(mContext);
  }

  @Override
  public void onResume() {
    Logger.info(LOG_TAG, "Called SetupSyncActivity.onResume.");
    super.onResume();

    if (!hasInternet()) {
      setContentView(R.layout.sync_setup_nointernet);
      return;
    }

    
    
    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        Account[] accts = mAccountManager.getAccountsByType(Constants.ACCOUNTTYPE_SYNC);
        finishResume(accts);
      }
    });
  }

  public void finishResume(Account[] accts) {
    Logger.debug(LOG_TAG, "Finishing Resume after fetching accounts.");

    
    Logger.debug(LOG_TAG, "Setting screen-on flag.");
    Window w = getWindow();
    w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    if (accts.length == 0) { 
      Logger.debug(LOG_TAG, "No accounts; starting J-PAKE receiver.");
      displayReceiveNoPin();
      if (jClient != null) {
        
        jClient.finished = true;
      }
      jClient = new JPakeClient(this);
      jClient.receiveNoPin();
      return;
    }

    
    Bundle extras = this.getIntent().getExtras();
    if (extras != null) {
      Logger.debug(LOG_TAG, "SetupSync with extras.");
      boolean isSetup = extras.getBoolean(Constants.INTENT_EXTRA_IS_SETUP);
      if (!isSetup) {
        Logger.debug(LOG_TAG, "Account exists; Pair a Device started.");
        pairWithPin = true;
        displayPairWithPin();
        return;
      }
    }
    
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Logger.debug(LOG_TAG, "Only one account supported. Redirecting.");
        
        
        Toast toast = Toast.makeText(mContext,
            R.string.sync_notification_oneaccount, Toast.LENGTH_LONG);
        toast.show();

        Intent intent = new Intent(Settings.ACTION_SYNC_SETTINGS);
        intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
        startActivity(intent);

        finish();
      }
    });
  }


  @Override
  public void onPause() {
    super.onPause();

    if (jClient != null) {
      jClient.abort(Constants.JPAKE_ERROR_USERABORT);
    }
    if (pairWithPin) {
      finish();
    }
  }

  @Override
  public void onNewIntent(Intent intent) {
    Logger.debug(LOG_TAG, "Started SetupSyncActivity with new intent.");
    setIntent(intent);
  }

  @Override
  public void onDestroy() {
    Logger.debug(LOG_TAG, "onDestroy() called.");
    super.onDestroy();
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
    Logger.debug(LOG_TAG, "Connect clicked.");
    
    pinError.setVisibility(View.INVISIBLE);
    enablePinEntry(false);
    connectButton.requestFocus();
    activateButton(connectButton, false);

    
    String pin = row1.getText().toString();
    pin += row2.getText().toString() + row3.getText().toString();

    
    if (jClient != null) {
      
      jClient.finished = true;
    }
    jClient = new JPakeClient(this);
    jClient.pairWithPin(pin);
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
      Logger.warn(LOG_TAG, "Asked to display null pin.");
      return;
    }
    
    int charPerLine = pin.length() / 3;
    final String pin1 = pin.substring(0, charPerLine);
    final String pin2 = pin.substring(charPerLine, 2 * charPerLine);
    final String pin3 = pin.substring(2 * charPerLine, pin.length());

    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        TextView view1 = pinTextView1;
        TextView view2 = pinTextView2;
        TextView view3 = pinTextView3;
        if (view1 == null || view2 == null || view3 == null) {
          Logger.warn(LOG_TAG, "Couldn't find view to display PIN.");
          return;
        }
        view1.setText(pin1);
        view1.setContentDescription(pin1.replaceAll("\\B", ", "));

        view2.setText(pin2);
        view2.setContentDescription(pin2.replaceAll("\\B", ", "));

        view3.setText(pin3);
        view3.setContentDescription(pin3.replaceAll("\\B", ", "));
      }
    });
  }

  



  public void displayAbort(String error) {
    if (!Constants.JPAKE_ERROR_USERABORT.equals(error) && !hasInternet()) {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          setContentView(R.layout.sync_setup_nointernet);
        }
      });
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

    
    Logger.debug(LOG_TAG, "abort reason: " + error);
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
  protected JSONObject makeAccountJSON(String username, String password,
                                       String syncKey, String serverURL) {

    JSONObject jAccount = new JSONObject();

    
    
    HashMap<String, String> fields = (HashMap<String, String>) jAccount;

    fields.put(Constants.JSON_KEY_SYNCKEY,  syncKey);
    fields.put(Constants.JSON_KEY_ACCOUNT,  username);
    fields.put(Constants.JSON_KEY_PASSWORD, password);
    fields.put(Constants.JSON_KEY_SERVER,   serverURL);

    Logger.debug(LOG_TAG, "Extracted account data: " + jAccount.toJSONString());
    return jAccount;
  }

  



  public void onPaired() {
    
    Account[] accts = mAccountManager.getAccountsByType(Constants.ACCOUNTTYPE_SYNC);
    if (accts.length == 0) {
      
      Logger.error(LOG_TAG, "No accounts present.");
      displayAbort(Constants.JPAKE_ERROR_INVALID);
      return;
    }

    
    Account account = accts[0];
    String username  = account.name;
    String password  = mAccountManager.getPassword(account);
    String syncKey   = mAccountManager.getUserData(account, Constants.OPTION_SYNCKEY);
    String serverURL = mAccountManager.getUserData(account, Constants.OPTION_SERVER);

    JSONObject jAccount = makeAccountJSON(username, password, syncKey, serverURL);
    try {
      jClient.sendAndComplete(jAccount);
    } catch (JPakeNoActivePairingException e) {
      Logger.error(LOG_TAG, "No active J-PAKE pairing.", e);
      displayAbort(Constants.JPAKE_ERROR_INVALID);
    }
  }

  



  public void onPairingStart() {
    if (!pairWithPin) {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          setContentView(R.layout.sync_setup_jpake_waiting);
        }
      });
      return;
    }
  }

  





  public void onComplete(JSONObject jCreds) {
    boolean result = true;

    if (!pairWithPin) {
      String accountName  = (String) jCreds.get(Constants.JSON_KEY_ACCOUNT);
      String password     = (String) jCreds.get(Constants.JSON_KEY_PASSWORD);
      String syncKey      = (String) jCreds.get(Constants.JSON_KEY_SYNCKEY);
      String serverURL    = (String) jCreds.get(Constants.JSON_KEY_SERVER);

      final SyncAccountParameters syncAccount = new SyncAccountParameters(mContext, mAccountManager,
          accountName, syncKey, password, serverURL);
      final Account account = SyncAccounts.createSyncAccount(syncAccount);
      result = (account != null);

      final Intent intent = new Intent(); 
      intent.putExtra(AccountManager.KEY_ACCOUNT_NAME, syncAccount.username);
      intent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNTTYPE_SYNC);
      intent.putExtra(AccountManager.KEY_AUTHTOKEN, Constants.ACCOUNTTYPE_SYNC);
      setAccountAuthenticatorResult(intent.getExtras());

      if (result) {
        setResult(RESULT_OK, intent);
      } else {
        setResult(RESULT_CANCELED, intent);
      }
    }

    jClient = null; 
    final boolean res = result;
    runOnUiThread(new Runnable() {
      @Override
      public void run() {
        displayResult(res);
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

  





  private void displayResult(boolean isSuccess) {
    Intent intent = null;
    if (isSuccess) {
      intent = new Intent(mContext, SetupSuccessActivity.class);
    }  else {
      intent = new Intent(mContext, SetupFailureActivity.class);
    }
    intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
    intent.putExtra(Constants.INTENT_EXTRA_IS_SETUP, !pairWithPin);
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
    Logger.debug(LOG_TAG, "Checking internet connectivity.");
    ConnectivityManager connManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo network = connManager.getActiveNetworkInfo();

    if (network != null && network.isConnected()) {
      Logger.debug(LOG_TAG, network + " is connected.");
      return true;
    }
    Logger.debug(LOG_TAG, "No connected networks.");
    return false;
  }

  



  private void displayPairWithPin() {
    Logger.debug(LOG_TAG, "PairWithPin initiated.");
    runOnUiThread(new Runnable() {

      @Override
      public void run() {
        setContentView(R.layout.sync_setup_pair);
        connectButton = (Button) findViewById(R.id.pair_button_connect);
        pinError = (LinearLayout) findViewById(R.id.pair_error);

        row1 = (EditText) findViewById(R.id.pair_row1);
        row2 = (EditText) findViewById(R.id.pair_row2);
        row3 = (EditText) findViewById(R.id.pair_row3);

        row1.addTextChangedListener(new TextWatcher() {
          @Override
          public void afterTextChanged(Editable s) {
             activateButton(connectButton, pinEntryCompleted());
             if (s.length() == 4) {
               row2.requestFocus();
             }
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
            activateButton(connectButton, pinEntryCompleted());
            if (s.length() == 4) {
              row3.requestFocus();
            }
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

        row1.requestFocus();
      }
    });
  }

  



  private void displayReceiveNoPin() {
    Logger.debug(LOG_TAG, "ReceiveNoPin initiated");
    runOnUiThread(new Runnable(){

      @Override
      public void run() {
        setContentView(R.layout.sync_setup);

        
        setupTitleView = ((TextView) findViewById(R.id.setup_title));
        setupSubtitleView = (TextView) findViewById(R.id.setup_subtitle);
        setupNoDeviceLinkTitleView = (TextView) findViewById(R.id.link_nodevice);
        pinTextView1 = ((TextView) findViewById(R.id.text_pin1));
        pinTextView2 = ((TextView) findViewById(R.id.text_pin2));
        pinTextView3 = ((TextView) findViewById(R.id.text_pin3));

        
        if (setupTitleView == null) {
          Logger.error(LOG_TAG, "No title view.");
        }
        if (setupSubtitleView == null) {
          Logger.error(LOG_TAG, "No subtitle view.");
        }
        if (setupNoDeviceLinkTitleView == null) {
          Logger.error(LOG_TAG, "No 'no device' link view.");
        }
      }
    });
  }
}
