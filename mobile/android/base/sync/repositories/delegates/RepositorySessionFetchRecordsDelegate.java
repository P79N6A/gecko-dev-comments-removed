





































package org.mozilla.gecko.sync.repositories.delegates;

import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.domain.Record;

public interface RepositorySessionFetchRecordsDelegate {
  public void onFetchFailed(Exception ex, Record record);
  public void onFetchedRecord(Record record);

  








  public void onFetchCompleted(final long fetchEnd);

  
  
  public void onFetchSucceeded(Record[] records, final long fetchEnd);

  public RepositorySessionFetchRecordsDelegate deferredFetchDelegate(ExecutorService executor);
}
