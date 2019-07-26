



package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.repositories.RecordFactory;

public class TabsRecordFactory extends RecordFactory {
  @Override
  public Record createRecord(Record record) {
    TabsRecord r = new TabsRecord();
    r.initFromEnvelope((CryptoRecord) record);
    return r;
  }
}
