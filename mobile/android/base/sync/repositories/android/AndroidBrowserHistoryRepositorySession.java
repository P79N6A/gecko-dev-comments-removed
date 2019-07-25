



package org.mozilla.gecko.sync.repositories.android;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
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
  public void begin(RepositorySessionBeginDelegate delegate) throws InvalidSessionTransitionException {
    
    
    try {
      dbHelper.delete(BrowserContract.History.GUID + " IS NULL", null);
    } catch (Exception e) {
      
    }
    super.begin(delegate);
  }

  @Override
  protected Record retrieveDuringStore(Cursor cur) {
    return RepoUtils.historyFromMirrorCursor(cur);
  }

  @Override
  protected Record retrieveDuringFetch(Cursor cur) {
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

  private Record addVisitsToRecord(Record record) throws NullCursorException {
    Log.d(LOG_TAG, "Adding visits for GUID " + record.guid);
    HistoryRecord hist = (HistoryRecord) record;
    JSONArray visitsArray = getDataExtender().visitsForGUID(hist.guid);
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

  @Override
  protected Record prepareRecord(Record record) {
    return record;
  }
  
  @Override
  public void abort() {
    ((AndroidBrowserHistoryDataAccessor) dbHelper).closeExtender();
    super.abort();
  }

  @Override
  public void finish(final RepositorySessionFinishDelegate delegate) throws InactiveSessionException {
    ((AndroidBrowserHistoryDataAccessor) dbHelper).closeExtender();
    super.finish(delegate);
  }
}
