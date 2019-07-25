




































package org.mozilla.gecko.sync.repositories.domain;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.repositories.RecordFactory;







public class BookmarkRecordFactory extends RecordFactory {

  @Override
  public Record createRecord(Record record) {
    BookmarkRecord r = new BookmarkRecord();
    r.initFromPayload((CryptoRecord) record);
    return r;
  }

}
