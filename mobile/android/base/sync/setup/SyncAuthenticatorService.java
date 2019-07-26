



package org.mozilla.gecko.sync.setup;

import java.io.UnsupportedEncodingException;
import java.security.NoSuchAlgorithmException;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.setup.activities.SetupSyncActivity;

import android.accounts.AbstractAccountAuthenticator;
import android.accounts.Account;
import android.accounts.AccountAuthenticatorResponse;
import android.accounts.AccountManager;
import android.accounts.NetworkErrorException;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
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

    
    result.putString(AccountManager.KEY_ACCOUNT_TYPE, SyncConstants.ACCOUNTTYPE_SYNC);

    
    String serverURL = am.getUserData(account, Constants.OPTION_SERVER);
    result.putString(Constants.OPTION_SERVER, serverURL);

    
    result.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);

    
    try {
      String username = Utils.usernameFromAccount(account.name);
      Logger.pii(LOG_TAG, "Account " + account.name + " hashes to " + username + ".");
      Logger.debug(LOG_TAG, "Setting username. Null? " + (username == null));
      result.putString(Constants.OPTION_USERNAME, username);
    } catch (NoSuchAlgorithmException e) {
      
      Logger.debug(LOG_TAG, "Exception in account lookup: " + e);
    } catch (UnsupportedEncodingException e) {
      
      Logger.debug(LOG_TAG, "Exception in account lookup: " + e);
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
      intent.putExtra("accountType", SyncConstants.ACCOUNTTYPE_SYNC);
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
    public Bundle getAccountRemovalAllowed(final AccountAuthenticatorResponse response, Account account)
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

      
      
      
      
      
      
      
      
      
      
      
      final Intent intent = SyncAccounts.makeSyncAccountDeletedIntent(mContext, AccountManager.get(mContext), account);
      Logger.info(LOG_TAG, "Account named " + account.name + " being removed; " +
          "broadcasting secure intent " + intent.getAction() + ".");
      mContext.sendBroadcast(intent, SyncConstants.PER_ACCOUNT_TYPE_PERMISSION);

      return result;
    }
  }
}
