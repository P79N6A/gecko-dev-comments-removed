



package org.mozilla.gecko.sync.repositories;














































public interface StoreTracker {

  





  public boolean trackRecordForExclusion(String guid);

  





  public boolean isTrackedForExclusion(String guid);

  




  public boolean untrackStoredForExclusion(String guid);

  public RecordFilter getFilter();
}
