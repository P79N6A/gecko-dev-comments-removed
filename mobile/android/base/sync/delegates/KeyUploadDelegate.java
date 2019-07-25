




































package org.mozilla.gecko.sync.delegates;

public interface KeyUploadDelegate {
  void onKeysUploaded();
  void onKeyUploadFailed(Exception e);
}
