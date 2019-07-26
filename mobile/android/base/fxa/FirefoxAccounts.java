



package org.mozilla.gecko.fxa;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;




public class FirefoxAccounts {
  





  public static boolean firefoxAccountsExist(final Context context) {
    return getFirefoxAccounts(context).length > 0;
  }

  





  public static Account[] getFirefoxAccounts(final Context context) {
    return AccountManager.get(context).getAccountsByType(FxAccountConstants.ACCOUNT_TYPE);
  }

  



  public static Account getFirefoxAccount(final Context context) {
    Account[] accounts = getFirefoxAccounts(context);
    if (accounts.length > 0) {
      return accounts[0];
    }
    return null;
  }
}