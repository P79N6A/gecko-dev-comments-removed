




































package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.repositories.RecordFactory;







public class HistoryRecordFactory extends RecordFactory {

  @Override
  public Record createRecord(Record record) {
    HistoryRecord r = new HistoryRecord();
    r.initFromPayload((CryptoRecord) record);
    return r;
  }

}
