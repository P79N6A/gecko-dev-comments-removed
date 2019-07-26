



package org.mozilla.gecko.sync.repositories.android;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;

public class AndroidBrowserHistoryDataAccessor extends
    AndroidBrowserRepositoryDataAccessor {

  private AndroidBrowserHistoryDataExtender dataExtender;

  public AndroidBrowserHistoryDataAccessor(Context context) {
    super(context);
    dataExtender = new AndroidBrowserHistoryDataExtender(context);
  }

  public AndroidBrowserHistoryDataExtender getHistoryDataExtender() {
    return dataExtender;
  }

  @Override
  protected Uri getUri() {
    return BrowserContractHelpers.HISTORY_CONTENT_URI;
  }

  @Override
  protected ContentValues getContentValues(Record record) {
    ContentValues cv = new ContentValues();
    HistoryRecord rec = (HistoryRecord) record;
    cv.put(BrowserContract.History.GUID, rec.guid);
    cv.put(BrowserContract.History.TITLE, rec.title);
    cv.put(BrowserContract.History.URL, rec.histURI);
    if (rec.visits != null) {
      JSONArray visits = rec.visits;
      long mostRecent = 0;
      for (int i = 0; i < visits.size(); i++) {
        JSONObject visit = (JSONObject) visits.get(i);
        long visitDate = (Long) visit
            .get(AndroidBrowserHistoryRepositorySession.KEY_DATE);
        if (visitDate > mostRecent) {
          mostRecent = visitDate;
        }
      }
      
      cv.put(BrowserContract.History.DATE_LAST_VISITED, mostRecent / 1000);
      cv.put(BrowserContract.History.VISITS, Long.toString(visits.size()));
    }
    return cv;
  }

  @Override
  protected String[] getAllColumns() {
    return BrowserContractHelpers.HistoryColumns;
  }

  @Override
  public Uri insert(Record record) {
    HistoryRecord rec = (HistoryRecord) record;
    Logger.debug(LOG_TAG, "Storing visits for " + record.guid);
    dataExtender.store(record.guid, rec.visits);
    Logger.debug(LOG_TAG, "Storing record " + record.guid);
    return super.insert(record);
  }

  @Override
  public void update(String oldGUID, Record newRecord) {
    HistoryRecord rec = (HistoryRecord) newRecord;
    String newGUID = newRecord.guid;
    Logger.debug(LOG_TAG, "Storing visits for " + newGUID + ", replacing " + oldGUID);
    dataExtender.delete(oldGUID);
    dataExtender.store(newGUID, rec.visits);
    super.update(oldGUID, newRecord);
  }

  @Override
  public int purgeGuid(String guid) {
    Logger.debug(LOG_TAG, "Purging record with " + guid);
    dataExtender.delete(guid);
    return super.purgeGuid(guid);
  }

  public void closeExtender() {
    dataExtender.close();
  }

  public static String[] GUID_AND_ID = new String[] { BrowserContract.History.GUID, BrowserContract.History._ID };

  













  public int bulkInsert(ArrayList<HistoryRecord> records) throws NullCursorException {
    if (records.isEmpty()) {
      Logger.debug(LOG_TAG, "No records to insert, returning.");
    }

    int size = records.size();
    ContentValues[] cvs = new ContentValues[size];
    String[] guids = new String[size];
    Map<String, Record> guidToRecord = new HashMap<String, Record>();
    int index = 0;
    for (Record record : records) {
      if (record.guid == null) {
        throw new IllegalArgumentException("Record with null GUID passed in to bulkInsert.");
      }
      cvs[index] = getContentValues(record);
      guids[index] = record.guid;
      guidToRecord.put(record.guid, record);
      index += 1;
    }

    
    int inserted = context.getContentResolver().bulkInsert(getUri(), cvs);
    if (inserted == size) {
      Logger.debug(LOG_TAG, "Inserted " + inserted + " records, as expected.");
    } else {
      Logger.debug(LOG_TAG, "Inserted " +
                   inserted + " records but expected " +
                   size     + " records; continuing to update visits.");
    }
    
    dataExtender.bulkInsert(records);
    return inserted;
  }
}
