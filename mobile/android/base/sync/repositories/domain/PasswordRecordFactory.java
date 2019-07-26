



package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.domain.PasswordRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

public class PasswordRecordFactory extends RecordFactory {
  @Override
  public Record createRecord(Record record) {
    PasswordRecord r = new PasswordRecord();
    r.initFromEnvelope((CryptoRecord) record);
    return r;
  }
}
