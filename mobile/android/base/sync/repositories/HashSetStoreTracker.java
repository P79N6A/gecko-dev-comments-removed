



package org.mozilla.gecko.sync.repositories;

import java.util.HashSet;

import org.mozilla.gecko.sync.repositories.domain.Record;

public class HashSetStoreTracker implements StoreTracker {

  
  
  
  
  private HashSet<String> guids;

  public HashSetStoreTracker() {
    guids = new HashSet<String>();
  }

  @Override
  public String toString() {
    return "#<Tracker: " + guids.size() + " guids tracked.>";
  }

  @Override
  public synchronized boolean trackRecordForExclusion(String guid) {
    return (guid != null) && guids.add(guid);
  }

  @Override
  public synchronized boolean isTrackedForExclusion(String guid) {
    return (guid != null) && guids.contains(guid);
  }

  @Override
  public synchronized boolean untrackStoredForExclusion(String guid) {
    return (guid != null) && guids.remove(guid);
  }

  @Override
  public RecordFilter getFilter() {
    if (guids.size() == 0) {
      return null;
    }
    return new RecordFilter() {
      @Override
      public boolean excludeRecord(Record r) {
        return isTrackedForExclusion(r.guid);
      }
    };
  }
}
