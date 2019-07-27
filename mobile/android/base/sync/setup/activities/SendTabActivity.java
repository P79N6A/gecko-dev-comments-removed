



package org.mozilla.gecko.sync.setup.activities;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.activities.FxAccountGetStartedActivity;
import org.mozilla.gecko.fxa.activities.FxAccountStatusActivity;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State.Action;
import org.mozilla.gecko.sync.CommandProcessor;
import org.mozilla.gecko.sync.CommandRunner;
import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.android.ClientsDatabaseAccessor;
import org.mozilla.gecko.sync.repositories.domain.ClientRecord;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.LocaleAware.LocaleAwareActivity;
import org.mozilla.gecko.sync.syncadapter.SyncAdapter;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class SendTabActivity extends LocaleAwareActivity {
  private interface TabSender {
    public static final String[] STAGES_TO_SYNC = new String[] { "clients", "tabs" };

    



    String getAccountGUID();

    


    void sync();
  }

  private static class FxAccountTabSender implements TabSender {
    private final AndroidFxAccount fxAccount;

    public FxAccountTabSender(Context context, AndroidFxAccount fxAccount) {
      this.fxAccount = fxAccount;
    }

    @Override
    public String getAccountGUID() {
      try {
        final SharedPreferences prefs = this.fxAccount.getSyncPrefs();
        return prefs.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
      } catch (Exception e) {
        Logger.warn(LOG_TAG, "Could not get Firefox Account parameters or preferences; aborting.");
        return null;
      }
    }

    @Override
    public void sync() {
      fxAccount.requestSync(FirefoxAccounts.FORCE, STAGES_TO_SYNC, null);
    }
  }

  private static class Sync11TabSender implements TabSender {
    private final Account account;
    private final AccountManager accountManager;
    private final Context context;

    private Sync11TabSender(Context context, Account syncAccount, AccountManager accountManager) {
      this.context = context;
      this.account = syncAccount;
      this.accountManager = accountManager;
    }

    @Override
    public String getAccountGUID() {
      try {
        final SharedPreferences prefs = SyncAccounts.blockingPrefsFromDefaultProfileV0(this.context, this.accountManager, this.account);
        return prefs.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
      } catch (Exception e) {
        Logger.warn(LOG_TAG, "Could not get Sync account parameters or preferences; aborting.");
        return null;
      }
    }

    @Override
    public void sync() {
      SyncAdapter.requestImmediateSync(this.account, STAGES_TO_SYNC);
    }
  }

  public static final String LOG_TAG = "SendTabActivity";
  private ClientRecordArrayAdapter arrayAdapter;

  private TabSender tabSender;
  private SendTabData sendTabData;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    try {
      sendTabData = getSendTabData(getIntent());
    } catch (IllegalArgumentException e) {
      notifyAndFinish(false);
      return;
    }

    setContentView(R.layout.sync_send_tab);

    final ListView listview = (ListView) findViewById(R.id.device_list);
    listview.setItemsCanFocus(true);
    listview.setTextFilterEnabled(true);
    listview.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);

    arrayAdapter = new ClientRecordArrayAdapter(this, R.layout.sync_list_item);
    listview.setAdapter(arrayAdapter);

    TextView textView = (TextView) findViewById(R.id.title);
    textView.setText(sendTabData.title);

    textView = (TextView) findViewById(R.id.uri);
    textView.setText(sendTabData.uri);

    enableSend(false);

    
  }

  protected static SendTabData getSendTabData(Intent intent) throws IllegalArgumentException {
    if (intent == null) {
      Logger.warn(LOG_TAG, "intent was null; aborting without sending tab.");
      throw new IllegalArgumentException();
    }

    Bundle extras = intent.getExtras();
    if (extras == null) {
      Logger.warn(LOG_TAG, "extras was null; aborting without sending tab.");
      throw new IllegalArgumentException();
    }

    SendTabData sendTabData = SendTabData.fromBundle(extras);
    if (sendTabData == null) {
      Logger.warn(LOG_TAG, "send tab data was null; aborting without sending tab.");
      throw new IllegalArgumentException();
    }

    if (sendTabData.uri == null) {
      Logger.warn(LOG_TAG, "uri was null; aborting without sending tab.");
      throw new IllegalArgumentException();
    }

    if (sendTabData.title == null) {
      Logger.warn(LOG_TAG, "title was null; ignoring and sending tab anyway.");
    }

    return sendTabData;
  }

  



  protected synchronized void updateClientList(final TabSender sender, final ClientRecordArrayAdapter adapter) {
    
    
    new AsyncTask<Void, Void, Collection<ClientRecord>>() {

      @Override
      protected Collection<ClientRecord> doInBackground(Void... params) {
        return getOtherClients(sender);
      }

      @Override
      protected void onPostExecute(final Collection<ClientRecord> clientArray) {
        

        Logger.debug(LOG_TAG, "Got " + clientArray.size() + " clients.");
        adapter.setClientRecordList(clientArray);
        if (clientArray.size() == 1) {
          adapter.checkItem(0, true);
        }

        enableSend(adapter.getNumCheckedGUIDs() > 0);
      }
    }.execute();
  }

  @Override
  public void onResume() {
    ActivityUtils.prepareLogging();
    Logger.info(LOG_TAG, "Called SendTabActivity.onResume.");
    super.onResume();

    


    final Context applicationContext = getApplicationContext();
    final AccountManager accountManager = AccountManager.get(applicationContext);

    final Account[] fxAccounts = accountManager.getAccountsByType(FxAccountConstants.ACCOUNT_TYPE);
    if (fxAccounts.length > 0) {
      final AndroidFxAccount fxAccount = new AndroidFxAccount(applicationContext, fxAccounts[0]);
      if (fxAccount.getState().getNeededAction() != Action.None) {
        
        
        Logger.warn(LOG_TAG, "Firefox Account named like " + fxAccount.getObfuscatedEmail() +
            " needs action before it can send a tab; redirecting to status activity.");
        redirectToNewTask(FxAccountStatusActivity.class, false);
        return;
      }

      this.tabSender = new FxAccountTabSender(applicationContext, fxAccount);

      
      updateClientList(tabSender, this.arrayAdapter);

      Logger.info(LOG_TAG, "Allowing tab send for Firefox Account.");
      registerDisplayURICommand();
      return;
    }

    final Account[] syncAccounts = accountManager.getAccountsByType(SyncConstants.ACCOUNTTYPE_SYNC);
    if (syncAccounts.length > 0) {
      this.tabSender = new Sync11TabSender(applicationContext, syncAccounts[0], accountManager);

      
      updateClientList(tabSender, this.arrayAdapter);

      Logger.info(LOG_TAG, "Allowing tab send for Sync account.");
      registerDisplayURICommand();
      return;
    }

    
    redirectToNewTask(FxAccountGetStartedActivity.class, false);
  }

  private static void registerDisplayURICommand() {
    final CommandProcessor processor = CommandProcessor.getProcessor();
    processor.registerCommand("displayURI", new CommandRunner(3) {
      @Override
      public void executeCommand(final GlobalSession session, List<String> args) {
        CommandProcessor.displayURI(args, session.getContext());
      }
    });
  }

  public void sendClickHandler(View view) {
    Logger.info(LOG_TAG, "Send was clicked.");
    final List<String> remoteClientGuids = arrayAdapter.getCheckedGUIDs();

    if (remoteClientGuids == null) {
      
      Logger.warn(LOG_TAG, "guids was null; aborting without sending tab.");
      notifyAndFinish(false);
      return;
    }

    final TabSender sender = this.tabSender;
    if (sender == null) {
      
      Logger.warn(LOG_TAG, "tabSender was null; aborting without sending tab.");
      notifyAndFinish(false);
      return;
    }

    
    
    new AsyncTask<Void, Void, Boolean>() {

      @Override
      protected Boolean doInBackground(Void... params) {
        final CommandProcessor processor = CommandProcessor.getProcessor();

        final String accountGUID = sender.getAccountGUID();
        Logger.debug(LOG_TAG, "Retrieved local account GUID '" + accountGUID + "'.");
        if (accountGUID == null) {
          return false;
        }

        for (String remoteClientGuid : remoteClientGuids) {
          processor.sendURIToClientForDisplay(sendTabData.uri, remoteClientGuid, sendTabData.title, accountGUID, getApplicationContext());
        }

        Logger.info(LOG_TAG, "Requesting immediate clients stage sync.");
        sender.sync();

        return true;
      }

      @Override
      protected void onPostExecute(final Boolean success) {
        
        notifyAndFinish(success.booleanValue());
      }
    }.execute();
  }

  









  protected void notifyAndFinish(final boolean success) {
    int textId;
    if (success) {
      textId = R.string.sync_text_tab_sent;
    } else {
      textId = R.string.sync_text_tab_not_sent;
    }

    Toast.makeText(this, textId, Toast.LENGTH_LONG).show();
    finish();
  }

  public void enableSend(boolean shouldEnable) {
    View sendButton = findViewById(R.id.send_button);
    sendButton.setEnabled(shouldEnable);
    sendButton.setClickable(shouldEnable);
  }

  


  protected Map<String, ClientRecord> getAllClients() {
    ClientsDatabaseAccessor db = new ClientsDatabaseAccessor(this.getApplicationContext());
    try {
      return db.fetchAllClients();
    } catch (NullCursorException e) {
      Logger.warn(LOG_TAG, "NullCursorException while populating device list.", e);
      return null;
    } finally {
      db.close();
    }
  }

  


  protected Collection<ClientRecord> getOtherClients(final TabSender sender) {
    if (sender == null) {
      Logger.warn(LOG_TAG, "No tab sender when fetching other client IDs.");
      return new ArrayList<ClientRecord>(0);
    }

    final Map<String, ClientRecord> all = getAllClients();
    if (all == null) {
      return new ArrayList<ClientRecord>(0);
    }

    final String ourGUID = sender.getAccountGUID();
    if (ourGUID == null) {
      return all.values();
    }

    final ArrayList<ClientRecord> out = new ArrayList<ClientRecord>(all.size());
    for (Entry<String, ClientRecord> entry : all.entrySet()) {
      if (ourGUID.equals(entry.getKey())) {
        continue;
      }
      out.add(entry.getValue());
    }
    return out;
  }

  
  protected void redirectToNewTask(Class<? extends Activity> activityClass, boolean success) {
    Intent intent = new Intent(this, activityClass);
    
    
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    startActivity(intent);
    notifyAndFinish(success);
  }
}
