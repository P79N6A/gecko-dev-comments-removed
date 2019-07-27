



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.net.MozResponse;












public interface ReadingListRecordDelegate {
  void onRecordReceived(ServerReadingListRecord record);
  void onComplete(ReadingListResponse response);
  void onFailure(MozResponse response);
  void onFailure(Exception error);
  void onRecordMissingOrDeleted(String guid, ReadingListResponse resp);
}
