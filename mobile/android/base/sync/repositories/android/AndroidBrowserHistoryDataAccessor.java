




































package org.mozilla.gecko.sync.repositories.android;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;

public class AndroidBrowserHistoryDataAccessor extends AndroidBrowserRepositoryDataAccessor {

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
    return BrowserContract.History.CONTENT_URI;
  }

  @Override
  protected ContentValues getContentValues(Record record) {
    ContentValues cv = new ContentValues();
    HistoryRecord rec = (HistoryRecord) record;
    cv.put(BrowserContract.History.GUID,            rec.guid);
    cv.put(BrowserContract.History.DATE_MODIFIED,        rec.lastModified);
    cv.put(BrowserContract.History.TITLE,           rec.title);
    cv.put(BrowserContract.History.URL,        rec.histURI);
    if (rec.visits != null) {
      JSONArray visits = (JSONArray) rec.visits;
      long mostRecent = 0;
      for (int i = 0; i < visits.size(); i++) {
        JSONObject visit = (JSONObject) visits.get(i);
        long visitDate = (Long) visit.get(AndroidBrowserHistoryRepositorySession.KEY_DATE);
        if (visitDate > mostRecent) {
          mostRecent = visitDate;
        }
      }
      cv.put(BrowserContract.History.DATE_LAST_VISITED,    mostRecent);
    }
    return cv;
  }

  @Override
  protected String[] getAllColumns() {
    return BrowserContract.History.HistoryColumns;
  }
  
  @Override
  public Uri insert(Record record) {
    HistoryRecord rec = (HistoryRecord) record;
    dataExtender.store(record.guid, rec.visits);
    return super.insert(record);
  }  
  
  @Override
  protected void delete(String guid) {
    context.getContentResolver().delete(getUri(), BrowserContract.SyncColumns.GUID + " = '" + guid + "'", null);
    dataExtender.delete(guid);
  }

}