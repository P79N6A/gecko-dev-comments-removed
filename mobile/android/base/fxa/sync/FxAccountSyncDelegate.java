



package org.mozilla.gecko.fxa.sync;

import java.util.concurrent.CountDownLatch;

import org.mozilla.gecko.fxa.login.State;

import android.content.SyncResult;

public class FxAccountSyncDelegate {
  protected final CountDownLatch latch;
  protected final SyncResult syncResult;

  public FxAccountSyncDelegate(CountDownLatch latch, SyncResult syncResult) {
    if (latch == null) {
      throw new IllegalArgumentException("latch must not be null");
    }
    if (syncResult == null) {
      throw new IllegalArgumentException("syncResult must not be null");
    }
    this.latch = latch;
    this.syncResult = syncResult;
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