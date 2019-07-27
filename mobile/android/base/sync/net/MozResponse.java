



package org.mozilla.gecko.sync.net;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Scanner;

import org.json.simple.parser.ParseException;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonObjectJSONException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.impl.cookie.DateParseException;
import ch.boye.httpclientandroidlib.impl.cookie.DateUtils;

public class MozResponse {
  private static final String LOG_TAG = "MozResponse";

  private static final String HEADER_RETRY_AFTER = "retry-after";

  protected HttpResponse response;
  private String body = null;

  public HttpResponse httpResponse() {
    return this.response;
  }

  public int getStatusCode() {
    return this.response.getStatusLine().getStatusCode();
  }

  public boolean wasSuccessful() {
    return this.getStatusCode() == 200;
  }

  





  public Header getContentType() {
    HttpEntity entity = this.response.getEntity();
    if (entity == null) {
      return null;
    }
    return entity.getContentType();
  }

  private static boolean missingHeader(String value) {
    return value == null ||
           value.trim().length() == 0;
  }

  public String body() throws IllegalStateException, IOException {
    if (body != null) {
      return body;
    }
    InputStreamReader is = new InputStreamReader(this.response.getEntity().getContent());
    
    body = new Scanner(is).useDelimiter("\\A").next();
    return body;
  }

  









  public ExtendedJSONObject jsonObjectBody() throws IllegalStateException, IOException,
                                 ParseException, NonObjectJSONException {
    if (body != null) {
      
      return ExtendedJSONObject.parseJSONObject(body);
    }

    HttpEntity entity = this.response.getEntity();
    if (entity == null) {
      throw new IOException("no entity");
    }

    InputStream content = entity.getContent();
    try {
      Reader in = new BufferedReader(new InputStreamReader(content, "UTF-8"));
      return ExtendedJSONObject.parseJSONObject(in);
    } finally {
      content.close();
    }
  }

  protected boolean hasHeader(String h) {
    return this.response.containsHeader(h);
  }

  public MozResponse(HttpResponse res) {
    response = res;
  }

  private String getNonMissingHeader(String h) {
    if (!this.hasHeader(h)) {
      return null;
    }

    final Header header = this.response.getFirstHeader(h);
    final String value = header.getValue();
    if (missingHeader(value)) {
      Logger.warn(LOG_TAG, h + " header present but empty.");
      return null;
    }
    return value;
  }

  protected long getLongHeader(String h) throws NumberFormatException {
    final String value = getNonMissingHeader(h);
    if (value == null) {
      return -1L;
    }
    return Long.parseLong(value, 10);
  }

  protected int getIntegerHeader(String h) throws NumberFormatException {
    final String value = getNonMissingHeader(h);
    if (value == null) {
      return -1;
    }
    return Integer.parseInt(value, 10);
  }

  


  public int retryAfterInSeconds() throws NumberFormatException {
    final String retryAfter = getNonMissingHeader(HEADER_RETRY_AFTER);
    if (retryAfter == null) {
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
      Logger.warn(LOG_TAG, "Retry-After header neither integer nor date: " + retryAfter);
      return -1;
    }
  }

  



  public int backoffInSeconds() throws NumberFormatException {
    return this.getIntegerHeader("x-backoff");
  }
}
