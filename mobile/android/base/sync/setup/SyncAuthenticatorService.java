



package org.mozilla.gecko.sync.setup;

import java.io.UnsupportedEncodingException;
import java.security.NoSuchAlgorithmException;

import org.mozilla.gecko.sync.GlobalConstants;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.SyncConfiguration;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.config.AccountPickler;
import org.mozilla.gecko.sync.config.ClientRecordTerminator;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;
import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;

import android.accounts.AbstractAccountAuthenticator;
import android.accounts.Account;
import android.accounts.AccountAuthenticatorResponse;
import android.accounts.AccountManager;
import android.accounts.NetworkErrorException;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;

public class SyncAuthenticatorService extends Service {
  private static final String LOG_TAG = "SyncAuthService";
  private SyncAccountAuthenticator sAccountAuthenticator = null;

  @Override
  public void onCreate() {
    Logger.debug(LOG_TAG, "onCreate");
    sAccountAuthenticator = getAuthenticator();
  }

  @Override
  public IBinder onBind(Intent intent) {
    if (intent.getAction().equals(android.accounts.AccountManager.ACTION_AUTHENTICATOR_INTENT)) {
      return getAuthenticator().getIBinder();
    }
    return null;
  }

  private SyncAccountAuthenticator getAuthenticator() {
    if (sAccountAuthenticator == null) {
      sAccountAuthenticator = new SyncAccountAuthenticator(this);
    }
    return sAccountAuthenticator;
  }

  



































  public static Bundle getPlainAuthToken(final Context context, final Account account)
      throws NetworkErrorException {
    
    
    final AccountManager am = AccountManager.get(context);
    final String password = am.getPassword(account);
    if (password == null) {
      Logger.warn(LOG_TAG, "Returning null bundle for getPlainAuthToken since Account password is null.");
      return null;
    }

    final Bundle result = new Bundle();

    
    result.putString(AccountManager.KEY_ACCOUNT_TYPE, GlobalConstants.ACCOUNTTYPE_SYNC);

    
    String serverURL = am.getUserData(account, Constants.OPTION_SERVER);
    result.putString(Constants.OPTION_SERVER, serverURL);

    
    result.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);

    
    try {
      String username = Utils.usernameFromAccount(account.name);
      Logger.pii(LOG_TAG, "Account " + account.name + " hashes to " + username + ".");
      Logger.debug(LOG_TAG, "Setting username. Null? " + (username == null));
      result.putString(Constants.OPTION_USERNAME, username);
    } catch (NoSuchAlgorithmException e) {
      
    } catch (UnsupportedEncodingException e) {
      
    }

    
    final String syncKey = am.getUserData(account, Constants.OPTION_SYNCKEY);
    Logger.debug(LOG_TAG, "Setting sync key. Null? " + (syncKey == null));
    result.putString(Constants.OPTION_SYNCKEY, syncKey);

    
    result.putString(AccountManager.KEY_AUTHTOKEN, password);
    return result;
  }

  private static class SyncAccountAuthenticator extends AbstractAccountAuthenticator {
    private Context mContext;
    public SyncAccountAuthenticator(Context context) {
      super(context);
      mContext = context;
    }

    @Override
    public Bundle addAccount(AccountAuthenticatorResponse response,
        String accountType, String authTokenType, String[] requiredFeatures,
        Bundle options) throws NetworkErrorException {
      Logger.debug(LOG_TAG, "addAccount()");
      final Intent intent = new Intent(mContext, SetupSyncActivity.class);
      intent.putExtra(AccountManager.KEY_ACCOUNT_AUTHENTICATOR_RESPONSE,
                      response);
      intent.putExtra("accountType", GlobalConstants.ACCOUNTTYPE_SYNC);
      intent.putExtra(Constants.INTENT_EXTRA_IS_SETUP, true);

      final Bundle result = new Bundle();
      result.putParcelable(AccountManager.KEY_INTENT, intent);

      return result;
    }

    @Override
    public Bundle confirmCredentials(AccountAuthenticatorResponse response,
                                     Account account,
                                     Bundle options) throws NetworkErrorException {
      Logger.debug(LOG_TAG, "confirmCredentials()");
      return null;
    }

    @Override
    public Bundle editProperties(AccountAuthenticatorResponse response,
                                 String accountType) {
      Logger.debug(LOG_TAG, "editProperties");
      return null;
    }

    @Override
    public Bundle getAuthToken(AccountAuthenticatorResponse response,
        Account account, String authTokenType, Bundle options)
        throws NetworkErrorException {
      Logger.debug(LOG_TAG, "getAuthToken()");

      if (Constants.AUTHTOKEN_TYPE_PLAIN.equals(authTokenType)) {
        return getPlainAuthToken(mContext, account);
      }

      final Bundle result = new Bundle();
      result.putString(AccountManager.KEY_ERROR_MESSAGE, "invalid authTokenType");
      return result;
    }

    @Override
    public String getAuthTokenLabel(String authTokenType) {
      Logger.debug(LOG_TAG, "getAuthTokenLabel()");
      return null;
    }

    @Override
    public Bundle hasFeatures(AccountAuthenticatorResponse response,
        Account account, String[] features) throws NetworkErrorException {
      Logger.debug(LOG_TAG, "hasFeatures()");
      return null;
    }

    @Override
    public Bundle updateCredentials(AccountAuthenticatorResponse response,
        Account account, String authTokenType, Bundle options)
        throws NetworkErrorException {
      Logger.debug(LOG_TAG, "updateCredentials()");
      return null;
    }

    










    @Override
    public Bundle getAccountRemovalAllowed(final AccountAuthenticatorResponse response, final Account account)
        throws NetworkErrorException {
      Bundle result = super.getAccountRemovalAllowed(response, account);

      if (result == null ||
          !result.containsKey(AccountManager.KEY_BOOLEAN_RESULT) ||
          result.containsKey(AccountManager.KEY_INTENT)) {
        return result;
      }

      final boolean removalAllowed = result.getBoolean(AccountManager.KEY_BOOLEAN_RESULT);
      if (!removalAllowed) {
        return result;
      }

      
      ThreadPool.run(new Runnable() {
        @Override
        public void run() {
          Logger.info(LOG_TAG, "Account named " + account.name + " being removed; " +
              "deleting saved pickle file '" + Constants.ACCOUNT_PICKLE_FILENAME + "'.");
          try {
            AccountPickler.deletePickle(mContext, Constants.ACCOUNT_PICKLE_FILENAME);
          } catch (Exception e) {
            
            Logger.warn(LOG_TAG, "Got exception deleting saved pickle file; ignoring.", e);
          }
        }
      });

      
      
      
      
      
      SyncAccountParameters tempParams = null;
      try {
        tempParams = SyncAccounts.blockingFromAndroidAccountV0(mContext, AccountManager.get(mContext), account);
      } catch (Exception e) {
        
      }
      final SyncAccountParameters params = tempParams;

      ThreadPool.run(new Runnable() {
        @Override
        public void run() {
          Logger.info(LOG_TAG, "Account named " + account.name + " being removed; " +
              "deleting client record from server.");

          if (params == null || params.username == null || params.password == null) {
            Logger.warn(LOG_TAG, "Account parameters were null; not deleting client record from server.");
            return;
          }

          
          
          
          
          
          final String product = GlobalConstants.BROWSER_INTENT_PACKAGE;
          final String profile = Constants.DEFAULT_PROFILE;
          final long version = SyncConfiguration.CURRENT_PREFS_VERSION;

          SharedPreferences prefs;
          try {
            prefs = Utils.getSharedPreferences(mContext, product, params.username, params.serverURL, profile, version);
          } catch (Exception e) {
            Logger.warn(LOG_TAG, "Caught exception fetching preferences; not deleting client record from server.", e);
            return;
          }

          final String clientGuid = prefs.getString(SyncConfiguration.PREF_ACCOUNT_GUID, null);
          final String clusterURL = prefs.getString(SyncConfiguration.PREF_CLUSTER_URL, null);

          if (clientGuid == null) {
            Logger.warn(LOG_TAG, "Client GUID was null; not deleting client record from server.");
            return;
          }

          if (clusterURL == null) {
            Logger.warn(LOG_TAG, "Cluster URL was null; not deleting client record from server.");
            return;
          }

          try {
            ClientRecordTerminator.deleteClientRecord(params.username, params.password, clusterURL, clientGuid);
          } catch (Exception e) {
            Logger.warn(LOG_TAG, "Got exception deleting client record from server; ignoring.", e);
          }
        }
      });

      return result;
    }
  }
}
