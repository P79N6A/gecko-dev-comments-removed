



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.net.MozResponse;

public interface ReadingListRecordUploadDelegate {
  
  public void onBatchDone();

  
  public void onSuccess(ClientReadingListRecord up, ReadingListRecordResponse response, ServerReadingListRecord down);
  public void onConflict(ClientReadingListRecord up, ReadingListResponse response);
  public void onInvalidUpload(ClientReadingListRecord up, ReadingListResponse response);
  public void onBadRequest(ClientReadingListRecord up, MozResponse response);
  public void onFailure(ClientReadingListRecord up, Exception ex);
  public void onFailure(ClientReadingListRecord up, MozResponse response);
}
