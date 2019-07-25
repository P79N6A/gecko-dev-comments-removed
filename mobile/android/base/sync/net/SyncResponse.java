





































package org.mozilla.gecko.sync.net;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Scanner;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.Utils;

import android.util.Log;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.impl.cookie.DateParseException;
import ch.boye.httpclientandroidlib.impl.cookie.DateUtils;

public class SyncResponse {
  private static final String HEADER_RETRY_AFTER = "retry-after";
  private static final String LOG_TAG = "SyncResponse";

  protected HttpResponse response;

  public SyncResponse() {
    super();
  }

  public SyncResponse(HttpResponse res) {
    response = res;
  }

  public HttpResponse httpResponse() {
    return this.response;
  }

  public int getStatusCode() {
    return this.response.getStatusLine().getStatusCode();
  }

  public boolean wasSuccessful() {
    return this.getStatusCode() == 200;
  }

  private String body = null;
  public String body() throws IllegalStateException, IOException {
    if (body != null) {
      return body;
    }
    InputStreamReader is = new InputStreamReader(this.response.getEntity().getContent());
    
    body = new Scanner(is).useDelimiter("\\A").next();
    return body;
  }

  








  public Object jsonBody() throws IllegalStateException, IOException,
                          ParseException {
    if (body != null) {
      
      ExtendedJSONObject.parse(body);
    }
    HttpEntity entity = this.response.getEntity();
    if (entity == null) {
      return null;
    }
    InputStream content = entity.getContent();
    return ExtendedJSONObject.parse(content);
  }

  public ExtendedJSONObject jsonObjectBody() throws IllegalStateException,
                                            IOException, ParseException,
                                            NonObjectJSONException {
    Object body = this.jsonBody();
    if (body instanceof ExtendedJSONObject) {
      return (ExtendedJSONObject) body;
    }
    throw new NonObjectJSONException(body);
  }

  private boolean hasHeader(String h) {
    return this.response.containsHeader(h);
  }

  private static boolean missingHeader(String value) {
    return value == null ||
           value.trim().length() == 0;
  }

  private int getIntegerHeader(String h) throws NumberFormatException {
    if (this.hasHeader(h)) {
      Header header = this.response.getFirstHeader(h);
      String value  = header.getValue();
      if (missingHeader(value)) {
        Log.w(LOG_TAG, h + " header present but empty.");
        return -1;
      }
      return Integer.parseInt(value, 10);
    }
    return -1;
  }

  


  public int retryAfter() throws NumberFormatException {
    if (!this.hasHeader(HEADER_RETRY_AFTER)) {
      return -1;
    }

    Header header = this.response.getFirstHeader(HEADER_RETRY_AFTER);
    String retryAfter = header.getValue();
    if (missingHeader(retryAfter)) {
      Log.w(LOG_TAG, "Retry-After header present but empty.");
      return -1;
    }

    try {
      return Integer.parseInt(retryAfter, 10);
    } catch (NumberFormatException e) {
      
    }

    try {
      final long then = DateUtils.parseDate(retryAfter).getTime();
      final long now  = System.currentTimeMillis();
      return (int)((then - now) / 1000);     
    } catch (DateParseException e) {
      Log.w(LOG_TAG, "Retry-After header neither integer nor date: " + retryAfter);
      return -1;
    }
  }

  public int weaveBackoff() throws NumberFormatException {
    return this.getIntegerHeader("x-weave-backoff");
  }

  








  public long normalizedWeaveTimestamp() {
    String h = "x-weave-timestamp";
    if (!this.hasHeader(h)) {
      return -1;
    }

    return Utils.decimalSecondsToMilliseconds(this.response.getFirstHeader(h).getValue());
  }

  public int weaveRecords() throws NumberFormatException {
    return this.getIntegerHeader("x-weave-records");
  }

  public int weaveQuotaRemaining() throws NumberFormatException {
    return this.getIntegerHeader("x-weave-quota-remaining");
  }

  public String weaveAlert() {
    if (this.hasHeader("x-weave-alert")) {
      return this.response.getFirstHeader("x-weave-alert").getValue();
    }
    return null;
  }
}