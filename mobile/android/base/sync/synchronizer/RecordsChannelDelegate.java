



package org.mozilla.gecko.sync.synchronizer;

public interface RecordsChannelDelegate {
  public void onFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd);
  public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex);
  public void onFlowFetchFailed(RecordsChannel recordsChannel, Exception ex);
  public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex, String recordGuid);
  public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex);
}