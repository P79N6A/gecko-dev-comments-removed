



package org.mozilla.gecko.reading;

import org.mozilla.gecko.sync.net.MozResponse;

public interface ReadingListWipeDelegate {
  void onSuccess(ReadingListStorageResponse response);
  void onPreconditionFailed(MozResponse response);
  void onFailure(Exception e);
  void onFailure(MozResponse response);
}
