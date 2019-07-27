



package org.mozilla.gecko.reading;

import java.util.Collection;

public interface ReadingListSynchronizerDelegate {
  
  void onUnableToSync(Exception e);

  
  
  void onStatusUploadComplete(Collection<String> uploaded, Collection<String> failed);
  void onNewItemUploadComplete(Collection<String> uploaded, Collection<String> failed);
  void onDownloadComplete();
  void onModifiedUploadComplete();

  
  void onComplete();
}
