



package org.mozilla.gecko.fxa.sync;

import java.util.concurrent.CountDownLatch;

import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.Married;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.StateLabel;
import org.mozilla.gecko.tokenserver.TokenServerException;

import android.content.SyncResult;

public class FxAccountSyncDelegate {
  protected final CountDownLatch latch;
  protected final SyncResult syncResult;
  protected final AndroidFxAccount fxAccount;

  public FxAccountSyncDelegate(CountDownLatch latch, SyncResult syncResult, AndroidFxAccount fxAccount) {
    if (latch == null) {
      throw new IllegalArgumentException("latch must not be null");
    }
    if (syncResult == null) {
      throw new IllegalArgumentException("syncResult must not be null");
    }
    if (fxAccount == null) {
      throw new IllegalArgumentException("fxAccount must not be null");
    }
    this.latch = latch;
    this.syncResult = syncResult;
    this.fxAccount = fxAccount;
  }

  


  protected void setSyncResultSuccess() {
    syncResult.stats.numUpdates += 1;
  }

  



  protected void setSyncResultSoftError() {
    syncResult.stats.numUpdates += 1;
    syncResult.stats.numIoExceptions += 1;
  }

  



  protected void setSyncResultHardError() {
    syncResult.stats.numAuthExceptions += 1;
  }

  public void handleSuccess() {
    setSyncResultSuccess();
    latch.countDown();
  }

  public void handleError(Exception e) {
    setSyncResultSoftError();
    
    
    if (e instanceof TokenServerException) {
      
      State state = fxAccount.getState();
      if (state.getStateLabel() == StateLabel.Married) {
        Married married = (Married) state;
        fxAccount.setState(married.makeCohabitingState());
      }
    }
    latch.countDown();
  }

  













  public void handleCannotSync(State finalState) {
    setSyncResultSoftError();
    latch.countDown();
  }

  public void postponeSync(long millis) {
    if (millis > 0) {
      
      
      
      



    }
    setSyncResultSoftError();
    latch.countDown();
  }

  




  public void rejectSync() {
    latch.countDown();
  }
}