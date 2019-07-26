


package org.mozilla.gecko.background.sync.helpers;

import java.util.HashMap;

import org.mozilla.gecko.sync.repositories.domain.Record;

public class ExpectFetchDelegate extends DefaultFetchDelegate {
  private HashMap<String, Record> expect = new HashMap<String, Record>();

  public ExpectFetchDelegate(Record[] records) {
    for(int i = 0; i < records.length; i++) {
      expect.put(records[i].guid, records[i]);
    }
  }

  @Override
  public void onFetchedRecord(Record record) {
    this.records.add(record);
  }

  @Override
  public void onFetchCompleted(final long fetchEnd) {
    super.onDone(this.records, this.expect, fetchEnd);
  }

  public Record recordAt(int i) {
    return this.records.get(i);
  }
}
