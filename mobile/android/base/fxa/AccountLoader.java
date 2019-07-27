



package org.mozilla.gecko.fxa;

import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.sync.setup.SyncAccounts;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.support.v4.content.AsyncTaskLoader;

















public class AccountLoader extends AsyncTaskLoader<Account> {
  protected Account account = null;
  protected BroadcastReceiver broadcastReceiver = null;

  public AccountLoader(Context context) {
    super(context);
  }

  
  @Override
  public Account loadInBackground() {
    final Context context = getContext();
    Account foundAccount = FirefoxAccounts.getFirefoxAccount(context);
    if (foundAccount == null) {
      final Account[] syncAccounts = SyncAccounts.syncAccounts(context);
      if (syncAccounts != null && syncAccounts.length > 0) {
        foundAccount = syncAccounts[0];
      }
    }
    return foundAccount;
  }

  
  @Override
  public void deliverResult(Account data) {
    if (isReset()) {
      
      releaseResources(data);
      return;
    }

    
    
    Account oldData = account;
    account = data;

    if (isStarted()) {
      
      
      super.deliverResult(data);
    }

    
    if (oldData != null && oldData != data) {
      releaseResources(oldData);
    }
  }

  
  @Override
  protected void onStartLoading() {
    if (account != null) {
      
      deliverResult(account);
    }

    
    if (broadcastReceiver == null) {
      broadcastReceiver = makeNewObserver();
      registerObserver(broadcastReceiver);
    }

    if (takeContentChanged() || account == null) {
      
      
      
      
      forceLoad();
    }
  }

  @Override
  protected void onStopLoading() {
    
    
    cancelLoad();

    
    
    
  }

  @Override
  protected void onReset() {
    
    
    
    stopLoading();

    
    if (account != null) {
      releaseResources(account);
      account = null;
    }

    
    if (broadcastReceiver != null) {
      final BroadcastReceiver observer = broadcastReceiver;
      broadcastReceiver = null;
      unregisterObserver(observer);
    }
  }

  @Override
  public void onCanceled(Account data) {
    
    super.onCanceled(data);

    
    
    releaseResources(data);
  }

  private void releaseResources(Account data) {
    
    
    
  }

  
  protected BroadcastReceiver makeNewObserver() {
    final BroadcastReceiver broadcastReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        
        
        
        onContentChanged();
      }
    };
    return broadcastReceiver;
  }

  protected void registerObserver(BroadcastReceiver observer) {
    final IntentFilter intentFilter = new IntentFilter();
    
    intentFilter.addAction(AccountManager.LOGIN_ACCOUNTS_CHANGED_ACTION);
    
    intentFilter.addAction(FxAccountConstants.ACCOUNT_STATE_CHANGED_ACTION);

    
    
    final Handler handler = null;
    getContext().registerReceiver(observer, intentFilter, FxAccountConstants.PER_ACCOUNT_TYPE_PERMISSION, handler);
  }

  protected void unregisterObserver(BroadcastReceiver observer) {
    getContext().unregisterReceiver(observer);
  }
}
