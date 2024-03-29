



package org.mozilla.gecko.reading;






public interface ReadingListChangeAccumulator {
  void addDeletion(String guid);
  void addDeletion(ClientReadingListRecord record);

  
  void addChangedRecord(ClientReadingListRecord record);
  void addDownloadedRecord(ServerReadingListRecord down);
  void finish() throws Exception;
}
