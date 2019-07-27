



package org.mozilla.gecko.fxa.sync;

import java.util.concurrent.BlockingQueue;

import org.mozilla.gecko.fxa.login.State;

import android.content.SyncResult;

public class FxAccountSyncDelegate {
  public enum Result {
    Success,
    Error,
    Postponed,
    Rejected,
  }

  protected final BlockingQueue<Result> latch;
  protected final SyncResult syncResult;

  public FxAccountSyncDelegate(BlockingQueue<Result> latch, SyncResult syncResult) {
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
    latch.offer(Result.Success);
  }

  public void handleError(Exception e) {
    setSyncResultSoftError();
    latch.offer(Result.Error);
  }

  













  public void handleCannotSync(State finalState) {
    setSyncResultSoftError();
    latch.offer(Result.Error);
  }

  public void postponeSync(long millis) {
    if (millis > 0) {
      
      
      
      



    }
    setSyncResultSoftError();
    latch.offer(Result.Postponed);
  }

  




  public void rejectSync() {
    latch.offer(Result.Rejected);
  }
}
