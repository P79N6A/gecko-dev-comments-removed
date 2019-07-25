



package org.mozilla.gecko.sync.repositories;

import org.mozilla.gecko.sync.repositories.domain.Record;

public interface RecordFilter {
  public boolean excludeRecord(Record r);
}
