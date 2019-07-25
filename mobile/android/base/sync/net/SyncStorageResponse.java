




































package org.mozilla.gecko.sync.net;


import ch.boye.httpclientandroidlib.HttpResponse;

public class SyncStorageResponse extends SyncResponse {
  
  static final int SERVER_RESPONSE_OVER_QUOTA = 14;

  
  public enum Reason {
    SUCCESS,
    OVER_QUOTA,
    UNAUTHORIZED_OR_REASSIGNED,
    SERVICE_UNAVAILABLE,
    BAD_REQUEST,
    UNKNOWN
  }

  public SyncStorageResponse(HttpResponse res) {
    this.response = res;
  }

  



  public Reason reason() {
    switch (this.response.getStatusLine().getStatusCode()) {
    case 200:
      return Reason.SUCCESS;
    case 400:
      try {
        Object body = this.jsonBody();
        if (body instanceof Number) {
          if (((Number) body).intValue() == SERVER_RESPONSE_OVER_QUOTA) {
            return Reason.OVER_QUOTA;
          }
        }
      } catch (Exception e) {
      }
      return Reason.BAD_REQUEST;
    case 401:
      return Reason.UNAUTHORIZED_OR_REASSIGNED;
    case 503:
      return Reason.SERVICE_UNAVAILABLE;
    }
    return Reason.UNKNOWN;
  }

  

}
