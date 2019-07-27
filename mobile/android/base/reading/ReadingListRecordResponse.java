



package org.mozilla.gecko.reading;

import java.io.IOException;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.NonObjectJSONException;

import ch.boye.httpclientandroidlib.HttpResponse;




public class ReadingListRecordResponse extends ReadingListResponse {
  @Override
  public boolean wasSuccessful() {
    final int code = getStatusCode();
    if (code == 200 || code == 201 || code == 204) {
      return true;
    }
    return super.wasSuccessful();
  }

  public static final ReadingListResponse.ResponseFactory<ReadingListRecordResponse> FACTORY = new ReadingListResponse.ResponseFactory<ReadingListRecordResponse>() {
    @Override
    public ReadingListRecordResponse getResponse(HttpResponse r) {
      return new ReadingListRecordResponse(r);
    }
  };

  public ReadingListRecordResponse(HttpResponse res) {
    super(res);
  }

  public ServerReadingListRecord getRecord() throws IllegalStateException, NonObjectJSONException, IOException, ParseException {
    return new ServerReadingListRecord(jsonObjectBody());
  }
}