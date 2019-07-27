



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.net.MozResponse;





public interface ReadingListDeleteDelegate {
  void onSuccess(ReadingListRecordResponse response, ReadingListRecord record);
  void onPreconditionFailed(String guid, MozResponse response);
  void onRecordMissingOrDeleted(String guid, MozResponse response);
  void onFailure(Exception e);
  void onFailure(MozResponse response);
}
