





































package org.mozilla.gecko.sync.repositories.android;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.Context;
import android.database.Cursor;
import android.util.Log;

public class AndroidBrowserHistoryRepositorySession extends AndroidBrowserRepositorySession {
  
  public static final String KEY_DATE = "date";
  public static final String KEY_TYPE = "type";
  public static final long DEFAULT_VISIT_TYPE = 1;

  public AndroidBrowserHistoryRepositorySession(Repository repository, Context context) {
    super(repository);
    dbHelper = new AndroidBrowserHistoryDataAccessor(context);
  }

  @Override
  protected Record recordFromMirrorCursor(Cursor cur) {
    return RepoUtils.historyFromMirrorCursor(cur);
  }

  @Override
  protected String buildRecordString(Record record) {
    HistoryRecord hist = (HistoryRecord) record;
    return hist.title + hist.histURI;
  }

  @Override
  protected Record transformRecord(Record record) throws NullCursorException {
    return addVisitsToRecord(record);
  }

  @SuppressWarnings("unchecked")
  private void addVisit(JSONArray visits, long date, long visitType) {
    JSONObject visit = new JSONObject();
    visit.put(KEY_DATE, date);               
    visit.put(KEY_TYPE, visitType);
    visits.add(visit);
  }

  private void addVisit(JSONArray visits, long date) {
    addVisit(visits, date, DEFAULT_VISIT_TYPE);
  }

  private AndroidBrowserHistoryDataExtender getDataExtender() {
    return ((AndroidBrowserHistoryDataAccessor) dbHelper).getHistoryDataExtender();
  }

  private JSONArray visitsForGUID(String guid) throws NullCursorException {
    Log.d(LOG_TAG, "Fetching visits for GUID " + guid);
    Cursor visits = getDataExtender().fetch(guid);
    try {
      if (!visits.moveToFirst()) {
        
        return new JSONArray();
      } else {
        return RepoUtils.getJSONArrayFromCursor(visits, AndroidBrowserHistoryDataExtender.COL_VISITS);
      }
    } finally {
      visits.close();
    }
  }

  private Record addVisitsToRecord(Record record) throws NullCursorException {
    Log.d(LOG_TAG, "Adding visits for GUID " + record.guid);
    HistoryRecord hist = (HistoryRecord) record;
    JSONArray visitsArray = visitsForGUID(hist.guid);
    long missingRecords = hist.fennecVisitCount - visitsArray.size();

    
    

    
    if (missingRecords > 0) {
      long fakes = missingRecords - 1;
      for (int j = 0; j < fakes; j++) {
        
        
        
        long fakeDate = (hist.fennecDateVisited - (1 + j)) * 1000;
        addVisit(visitsArray, fakeDate);
      }

      
      
      addVisit(visitsArray, hist.fennecDateVisited * 1000);
    }

    hist.visits = visitsArray;
    return hist;
  }
}
