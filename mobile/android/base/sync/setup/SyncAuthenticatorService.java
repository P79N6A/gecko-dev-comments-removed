





































package org.mozilla.gecko.sync.setup;

import java.io.UnsupportedEncodingException;
import java.security.NoSuchAlgorithmException;

import org.mozilla.gecko.sync.crypto.KeyBundle;
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
import android.util.Log;

public class SyncAuthenticatorService extends Service {
  private static final String LOG_TAG = "SyncAuthService";
  private SyncAccountAuthenticator sAccountAuthenticator = null;

  @Override
  public void onCreate() {
    Log.d(LOG_TAG, "onCreate");
    sAccountAuthenticator = getAuthenticator();
  }

  @Override
  public IBinder onBind(Intent intent) {
    IBinder ret = null;
    if (intent.getAction().equals(
        android.accounts.AccountManager.ACTION_AUTHENTICATOR_INTENT))
      ret = getAuthenticator().getIBinder();
    return ret;
  }

  private SyncAccountAuthenticator getAuthenticator() {
    if (sAccountAuthenticator == null) {
      sAccountAuthenticator = new SyncAccountAuthenticator(this);
    }
    return sAccountAuthenticator;
  }

  private static class SyncAccountAuthenticator extends
      AbstractAccountAuthenticator {
    private Context mContext;
    public SyncAccountAuthenticator(Context context) {
      super(context);
      mContext = context;
    }

    @Override
    public Bundle addAccount(AccountAuthenticatorResponse response,
        String accountType, String authTokenType, String[] requiredFeatures,
        Bundle options) throws NetworkErrorException {
      Log.d(LOG_TAG, "addAccount()");
      final Intent intent = new Intent(mContext, SetupSyncActivity.class);
      intent.putExtra(AccountManager.KEY_ACCOUNT_AUTHENTICATOR_RESPONSE,
          response);
      intent.putExtra("accountType", Constants.ACCOUNTTYPE_SYNC);
      intent.putExtra(Constants.INTENT_EXTRA_IS_SETUP, true);

      final Bundle result = new Bundle();
      result.putParcelable(AccountManager.KEY_INTENT, intent);

      return result;
    }

    @Override
    public Bundle confirmCredentials(AccountAuthenticatorResponse response,
        Account account, Bundle options) throws NetworkErrorException {
      Log.d(LOG_TAG, "confirmCredentials()");
      return null;
    }

    @Override
    public Bundle editProperties(AccountAuthenticatorResponse response,
        String accountType) {
      Log.d(LOG_TAG, "editProperties");
      return null;
    }

    @Override
    public Bundle getAuthToken(AccountAuthenticatorResponse response,
        Account account, String authTokenType, Bundle options)
        throws NetworkErrorException {
      Log.d(LOG_TAG, "getAuthToken()");
      if (!authTokenType.equals(Constants.AUTHTOKEN_TYPE_PLAIN)) {
        final Bundle result = new Bundle();
        result.putString(AccountManager.KEY_ERROR_MESSAGE,
            "invalid authTokenType");
        return result;
      }

      
      
      Log.d(LOG_TAG, "AccountManager.get(" + mContext + ")");
      final AccountManager am = AccountManager.get(mContext);
      final String password = am.getPassword(account);
      if (password != null) {
        final Bundle result = new Bundle();

        
        result.putString(AccountManager.KEY_ACCOUNT_TYPE, Constants.ACCOUNTTYPE_SYNC);

        
        String serverURL = am.getUserData(account, Constants.OPTION_SERVER);
        result.putString(Constants.OPTION_SERVER, serverURL);

        
        result.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);

        
        try {
          String username = KeyBundle.usernameFromAccount(account.name);
          Log.i("rnewman", "Account " + account.name + " hashes to " + username);
          result.putString(Constants.OPTION_USERNAME, username);
        } catch (NoSuchAlgorithmException e) {
          
        } catch (UnsupportedEncodingException e) {
          
        }

        
        final String syncKey = am.getUserData(account, Constants.OPTION_SYNCKEY);
        Log.i("rnewman", "Setting Sync Key to " + syncKey);
        result.putString(Constants.OPTION_SYNCKEY, syncKey);

        
        result.putString(AccountManager.KEY_AUTHTOKEN, password);
        return result;
      }
      Log.w(LOG_TAG, "Returning null bundle for getAuthToken.");
      return null;
    }

    @Override
    public String getAuthTokenLabel(String authTokenType) {
      Log.d(LOG_TAG, "getAuthTokenLabel()");
      return null;
    }

    @Override
    public Bundle hasFeatures(AccountAuthenticatorResponse response,
        Account account, String[] features) throws NetworkErrorException {
      Log.d(LOG_TAG, "hasFeatures()");
      return null;
    }

    @Override
    public Bundle updateCredentials(AccountAuthenticatorResponse response,
        Account account, String authTokenType, Bundle options)
        throws NetworkErrorException {
      Log.d(LOG_TAG, "updateCredentials()");
      return null;
    }
  }
}
