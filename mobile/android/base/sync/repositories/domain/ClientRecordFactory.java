



package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.repositories.RecordFactory;

public class ClientRecordFactory extends RecordFactory {

  @Override
  public Record createRecord(Record record) {
    ClientRecord r = new ClientRecord();
    r.initFromEnvelope((CryptoRecord) record);
    return r;
  }
}
