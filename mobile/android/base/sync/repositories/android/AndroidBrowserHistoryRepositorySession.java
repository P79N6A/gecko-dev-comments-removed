




































package org.mozilla.gecko.sync.repositories.android;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.mozilla.gecko.sync.repositories.NoGuidForIdException;
import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.ParentNotFoundException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.domain.HistoryRecord;
import org.mozilla.gecko.sync.repositories.domain.Record;

import android.content.Context;
import android.database.Cursor;

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
  protected Record[] doFetch(String[] guids) throws NoGuidForIdException,
      NullCursorException, ParentNotFoundException {
    return addVisitsToRecords(super.doFetch(guids));
  }
  
  @Override
  protected Record[] doFetchSince(long since) throws NoGuidForIdException, NullCursorException, ParentNotFoundException {
    return addVisitsToRecords(super.doFetchSince(since));
  }
  
  @Override
  protected Record[] doFetchAll() throws NullCursorException, NoGuidForIdException, ParentNotFoundException {
    return addVisitsToRecords(super.doFetchAll());
  }
  
  @SuppressWarnings("unchecked")
  private Record[] addVisitsToRecords(Record[] records) throws NullCursorException {
    AndroidBrowserHistoryDataExtender dataExtender = ((AndroidBrowserHistoryDataAccessor) dbHelper).getHistoryDataExtender();
    for(int i = 0; i < records.length; i++) {
      HistoryRecord hist = (HistoryRecord) records[i];
      Cursor visits = dataExtender.fetch(hist.guid);
      visits.moveToFirst();
      JSONArray visitsArray = RepoUtils.getJSONArrayFromCursor(visits, AndroidBrowserHistoryDataExtender.COL_VISITS);
      long missingRecords = hist.fennecVisitCount - visitsArray.size();
      
      
      if (missingRecords >= 1) {
        
        if (missingRecords > 1) {
          for (int j = 0; j < missingRecords -1; j++) {
            JSONObject fake = new JSONObject();
            
            
            
            fake.put(KEY_DATE, (long) hist.fennecDateVisited - (1+j));
            fake.put(KEY_TYPE, DEFAULT_VISIT_TYPE);
            visitsArray.add(fake);
          }
        }
        
        
        
        
        JSONObject real = new JSONObject();
        real.put(KEY_DATE, hist.fennecDateVisited);
        real.put(KEY_TYPE, DEFAULT_VISIT_TYPE);
        visitsArray.add(real);
      }
      hist.visits = visitsArray;
      records[i] = hist;
    }
    
    return records;
  }
}
