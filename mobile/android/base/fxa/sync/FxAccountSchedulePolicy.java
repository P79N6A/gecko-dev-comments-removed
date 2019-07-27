



package org.mozilla.gecko.fxa.sync;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State.Action;
import org.mozilla.gecko.sync.BackoffHandler;

import android.accounts.Account;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;

public class FxAccountSchedulePolicy implements SchedulePolicy {
  private static final String LOG_TAG = "FxAccountSchedulePolicy";

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  public static final long POLL_INTERVAL_PENDING_VERIFICATION = 60;         

  
  
  
  public static final long POLL_INTERVAL_ERROR_STATE_SEC = 24 * 60 * 60;    

  
  
  public static final long POLL_INTERVAL_SINGLE_DEVICE_SEC = 18 * 60 * 60;  

  
  
  
  
  public static final long POLL_INTERVAL_MULTI_DEVICE_SEC = 12 * 60 * 60;   

  
  
  private static volatile long POLL_INTERVAL_CURRENT_SEC = POLL_INTERVAL_SINGLE_DEVICE_SEC;

  
  
  public static final long RATE_LIMIT_FUNDAMENTAL_SEC = 90;                 

  








  public static final long RATE_LIMIT_BACKGROUND_SEC = 60 * 60;             

  private final AndroidFxAccount account;
  private final Context context;

  public FxAccountSchedulePolicy(Context context, AndroidFxAccount account) {
    this.account = account;
    this.context = context;
  }

  





  private static long delay(long millis) {
    return System.currentTimeMillis() + millis;
  }

  




  protected void requestPeriodicSync(final long intervalSeconds) {
    final String authority = BrowserContract.AUTHORITY;
    final Account account = this.account.getAndroidAccount();
    this.context.getContentResolver();
    Logger.info(LOG_TAG, "Scheduling periodic sync for " + intervalSeconds + ".");
    ContentResolver.addPeriodicSync(account, authority, Bundle.EMPTY, intervalSeconds);
    POLL_INTERVAL_CURRENT_SEC = intervalSeconds;
  }

  @Override
  public void onSuccessfulSync(int otherClientsCount) {
    this.account.setLastSyncedTimestamp(System.currentTimeMillis());
    
    
    
    long interval = (otherClientsCount > 0) ? POLL_INTERVAL_MULTI_DEVICE_SEC : POLL_INTERVAL_SINGLE_DEVICE_SEC;
    requestPeriodicSync(interval);
  }

  @Override
  public void onHandleFinal(Action needed) {
    switch (needed) {
    case NeedsPassword:
    case NeedsUpgrade:
      requestPeriodicSync(POLL_INTERVAL_ERROR_STATE_SEC);
      break;
    case NeedsVerification:
      requestPeriodicSync(POLL_INTERVAL_PENDING_VERIFICATION);
      break;
    case None:
      
      
      break;
    }
  }

  @Override
  public void onUpgradeRequired() {
    
    
    requestPeriodicSync(POLL_INTERVAL_ERROR_STATE_SEC);
  }

  @Override
  public void onUnauthorized() {
    
    
    requestPeriodicSync(POLL_INTERVAL_ERROR_STATE_SEC);
  }

  @Override
  public void configureBackoffMillisOnBackoff(BackoffHandler backoffHandler, long backoffMillis, boolean onlyExtend) {
    if (onlyExtend) {
      backoffHandler.extendEarliestNextRequest(delay(backoffMillis));
    } else {
      backoffHandler.setEarliestNextRequest(delay(backoffMillis));
    }

    
    
    
    
    if (backoffMillis > (POLL_INTERVAL_CURRENT_SEC * 1000)) {
      
      
      
      requestPeriodicSync((long) Math.ceil((1.05 * backoffMillis) / 1000));
    }
  }

  




  @Override
  public void configureBackoffMillisBeforeSyncing(BackoffHandler fundamentalRateHandler, BackoffHandler backgroundRateHandler) {
    fundamentalRateHandler.setEarliestNextRequest(delay(RATE_LIMIT_FUNDAMENTAL_SEC * 1000));
    backgroundRateHandler.setEarliestNextRequest(delay(RATE_LIMIT_BACKGROUND_SEC * 1000));
  }
}