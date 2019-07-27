



package org.mozilla.gecko.sync.net;

import org.mozilla.gecko.sync.Utils;

import ch.boye.httpclientandroidlib.HttpResponse;

public class SyncResponse extends MozResponse {
  public SyncResponse(HttpResponse res) {
    super(res);
  }

  



  public int weaveBackoffInSeconds() throws NumberFormatException {
    return this.getIntegerHeader("x-weave-backoff");
  }

  








  public int totalBackoffInSeconds(boolean includeRetryAfter) {
    int retryAfterInSeconds = -1;
    if (includeRetryAfter) {
      try {
        retryAfterInSeconds = retryAfterInSeconds();
      } catch (NumberFormatException e) {
      }
    }

    int weaveBackoffInSeconds = -1;
    try {
      weaveBackoffInSeconds = weaveBackoffInSeconds();
    } catch (NumberFormatException e) {
    }

    int backoffInSeconds = -1;
    try {
      backoffInSeconds = backoffInSeconds();
    } catch (NumberFormatException e) {
    }

    int totalBackoff = Math.max(retryAfterInSeconds, Math.max(backoffInSeconds, weaveBackoffInSeconds));
    if (totalBackoff < 0) {
      return -1;
    } else {
      return totalBackoff;
    }
  }

  



  public long totalBackoffInMilliseconds() {
    long totalBackoff = totalBackoffInSeconds(true);
    if (totalBackoff < 0) {
      return -1;
    } else {
      return 1000 * totalBackoff;
    }
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
